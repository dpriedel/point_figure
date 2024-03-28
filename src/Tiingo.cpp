// =====================================================================================
//
//       Filename:  Tiingo.cpp
//
//    Description:  Live stream ticker updates
//
//        Version:  1.0
//        Created:  08/06/2021 09:28:55 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)/
// =====================================================================================
// the guts of this code comes from the examples distributed by Boost.

#include "boost/static_assert.hpp"
#include <algorithm>
#include <chrono>
#include <exception>
#include <format>
#include <iostream>
#include <mutex>
#include <ranges>
#include <regex>
#include <spdlog/spdlog.h>
#include <string_view>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include "Tiingo.h"

using namespace std::string_literals;
using namespace std::chrono_literals;

//--------------------------------------------------------------------------------------
//       Class:  Tiingo
//      Method:  Tiingo
// Description:  constructor
//--------------------------------------------------------------------------------------

Tiingo::Tiingo(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix)
    : Streamer{host, port, api_key, prefix}
{
}  // -----  end of method Tiingo::Tiingo  (constructor)  -----

void Tiingo::StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
{
    StartStreaming();

    // This buffer will hold the incoming message
    // Each message contains data for a single transaction

    beast::flat_buffer buffer;

    while (ws_.is_open() && !(*had_signal))
    {
        try
        {
            buffer.clear();
            ws_.read(buffer);
            ws_.text(ws_.got_text());
            std::string buffer_content = beast::buffers_to_string(buffer.cdata());
            if (!buffer_content.empty())
            {
                const std::lock_guard<std::mutex> queue_lock(*data_mutex);
                streamed_data->push(std::move(buffer_content));
            }
        }
        catch (std::system_error& e)
        {
            // any system problems, we close the socket and force our way out.
            // on EOF, just cleanly shutdown. Let higher level code
            // decide what to do then.

            auto ec = e.code();
            if (ec.value() == boost::asio::error::eof)
            {
                spdlog::info("EOF on websocket read. Exiting streaming.");
                StopStreaming();
                throw StreamingEOF{};
            }
            spdlog::error(std::format("System error. Category: {}. Value: {}. Message: {}", ec.category().name(),
                                      ec.value(), ec.message()));
            *had_signal = true;
            ws_.close(websocket::close_code::going_away);
        }
        catch (std::exception& e)
        {
            spdlog::error(std::format("Problem processing steamed data. Message: {}", e.what()));

            if (std::string_view{e.what()}.starts_with("End of file") ||
                std::string_view{e.what()}.starts_with("End of stream"))
            {
                // I had expected to get a system error here. Hence the
                // catch block above.
                // Anyways, let's just shutdown the stream and exit.

                spdlog::info("EOF on websocket read. Exiting streaming.");
                StopStreaming();
                throw StreamingEOF{};
            }
            *had_signal = true;
            StopStreaming();
        }
        catch (...)
        {
            spdlog::error("Unknown problem processing steamed data.");
            *had_signal = true;
            StopStreaming();
        }
    }
    if (*had_signal)
    {
        // do a last check for data

        buffer.clear();
        ws_.read(buffer);
        std::string buffer_content = beast::buffers_to_string(buffer.cdata());
        if (!buffer_content.empty())
        {
            const std::lock_guard<std::mutex> queue_lock(*data_mutex);
            streamed_data->push(std::move(buffer_content));
        }
    }
    // if the websocket is closed on the server side or there is a timeout which in turn
    // will cause the websocket to be closed, let's set this flag so other processes which
    // may be watching it can know.

    StopStreaming();
    DisconnectWS();

}  // -----  end of method Tiingo::StreamData  -----

void Tiingo::StartStreaming()
{
    // we need to do our own connect/disconnect so we can handle any
    // interruptions in streaming.

    ConnectWS();

    Json::Value connection_request;
    connection_request["eventName"] = "subscribe";
    connection_request["authorization"] = api_key_;
    connection_request["eventData"]["thresholdLevel"] = 0;
    Json::Value tickers(Json::arrayValue);
    for (const auto& symbol : symbol_list_)
    {
        tickers.append(symbol);
    }

    connection_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // compact printing and string formatting
    const std::string connection_request_str = Json::writeString(builder, connection_request);
    //    std::cout << "Jsoncpp connection_request_str: " << connection_request_str << '\n';

    // Send the message
    ws_.write(net::buffer(connection_request_str));

    // let's make sure we were successful

    beast::flat_buffer buffer;

    ws_.read(buffer);
    ws_.text(ws_.got_text());
    std::string buffer_content = beast::buffers_to_string(buffer.cdata());
    // if (!buffer_content.empty())
    // {
    JSONCPP_STRING err;
    Json::Value response;

    Json::CharReaderBuilder builder2;
    const std::unique_ptr<Json::CharReader> reader(builder2.newCharReader());
    //    if (!reader->parse(buffer.data(), buffer.data() + buffer.size(), &response, nullptr))
    if (!reader->parse(buffer_content.data(), buffer_content.data() + buffer_content.size(), &response, &err))
    {
        throw std::runtime_error("Problem parsing tiingo response: "s + err);
    }

    std::string message_type = response["messageType"].asString();
    BOOST_ASSERT_MSG(message_type == "I", std::format("Expected message type of 'I'. Got: {}", message_type).c_str());
    int32_t code = response["response"]["code"].asInt();
    BOOST_ASSERT_MSG(code == 200, std::format("Expected success code of '200'. Got: {}", code).c_str());

    subscription_id_ = response["data"]["subscriptionId"].asString();
    // }
}  // -----  end of method Tiingo::StartStreaming  -----

Tiingo::PF_Data Tiingo::ExtractData(const std::string& buffer)
{
    // std::cout << "\nraw buffer: " << buffer << std::endl;

    static const std::regex kNumericTradePrice{R"***(("T",(?:[^,]*,){8})([0-9]*\.[0-9]*),)***"};
    static const std::regex kQuotedTradePrice{R"***("T",(?:[^,]*,){8}"([0-9]*\.[0-9]*)",)***"};
    static const std::string kStringTradePrice{R"***($1"$2",)***"};
    const std::string zapped_buffer = std::regex_replace(buffer, kNumericTradePrice, kStringTradePrice);
    // std::cout << "\nzapped buffer: " << zapped_buffer << std::endl;

    // will eventually need to use locks to access this I think.
    // for now, we just append data.
    JSONCPP_STRING err;
    Json::Value response;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    //    if (!reader->parse(buffer.data(), buffer.data() + buffer.size(), &response, nullptr))
    if (!reader->parse(zapped_buffer.data(), zapped_buffer.data() + zapped_buffer.size(), &response, &err))
    {
        throw std::runtime_error("Problem parsing tiingo response: "s + err);
    }
    //    std::cout << "\n\n jsoncpp parsed response: " << response << "\n\n";

    PF_Data pf_data;

    auto message_type = response["messageType"];
    if (message_type == "A")
    {
        auto data = response["data"];

        if (data[0] == "T")
        {
            std::smatch m;
            if (bool found_it = std::regex_search(zapped_buffer, m, kQuotedTradePrice); !found_it)
            {
                spdlog::error(std::format("can't find trade price in buffer: {}", zapped_buffer));
            }
            else
            {
                pf_data.subscription_id_ = subscription_id_;
                pf_data.time_stamp_ = data[1].asCString();
                pf_data.time_stamp_nanoseconds_utc_ = data[2].asInt64();
                pf_data.ticker_ = data[3].asCString();
                rng::for_each(pf_data.ticker_, [](char& c) { c = std::toupper(c); });
                pf_data.last_price_ = decimal::Decimal{m[1].str()};
                pf_data.last_size_ = data[10].asInt();
            }

            //        std::cout << "new data: " << pf_data_.back().ticker_ << " : " << pf_data_.back().last_price_
            //        <<
            //        '\n';
        }
    }
    // else if (message_type == "I")
    // {
    //     subscription_id_ = response["data"]["subscriptionId"].asString();
    //     //        std::cout << "json cpp subscription ID: " << subscription_id_ << '\n';
    // }
    else if (message_type == "H")
    {
        // heartbeat , just return
    }
    else
    {
        spdlog::error("unexpected message type.");
    }
    return pf_data;
}  // -----  end of method Tiingo::ExtractData  -----

void Tiingo::StopStreaming()
{
    // we need to send the unsubscribe message in a separate connection.

    Json::Value disconnect_request;
    disconnect_request["eventName"] = "unsubscribe";
    disconnect_request["authorization"] = api_key_;
    disconnect_request["eventData"]["subscriptionId"] = subscription_id_;
    Json::Value tickers(Json::arrayValue);
    for (const auto& symbol : symbol_list_)
    {
        tickers.append(symbol);
    }

    disconnect_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // compact printing and string formatting
    const std::string disconnect_request_str = Json::writeString(builder, disconnect_request);
    //    std::cout << "Jsoncpp disconnect_request_str: " << disconnect_request_str << '\n';

    // just grab the code from the example program

    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};

    tcp::resolver resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws{ioc, ctx};

    auto host = host_;
    auto port = port_;

    auto const results = resolver.resolve(host, port);

    auto ep = net::connect(get_lowest_layer(ws), results);

    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
            "Failed to set SNI Hostname");
    }

    host += ':' + std::to_string(ep.port());

    ws.next_layer().handshake(ssl::stream_base::client);

    ws.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

    ws.handshake(host, websocket_prefix_);

    try
    {
        ws.write(net::buffer(std::string(disconnect_request_str)));

        beast::flat_buffer buffer;

        ws.read(buffer);

        ws.close(websocket::close_code::normal);
    }
    catch (std::exception& e)
    {
        spdlog::error(std::format("Problem closing socket after clearing streaming symbols: {}", e.what()));
    }

    DisconnectWS();

    //    std::cout << beast::make_printable(buffer.data()) << std::endl;

}  // -----  end of method Tiingo::StopStreaming  -----

Json::Value Tiingo::GetTopOfBookAndLastClose()
{
    // using the REST API for iex.

    // if any problems occur here, we'll just let beast throw an exception.

    tcp::resolver resolver(ioc_);
    beast::ssl_stream<beast::tcp_stream> stream(ioc_, ctx_);

    if (!SSL_set_tlsext_host_name(stream.native_handle(), host_.c_str()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    auto const results = resolver.resolve(host_, port_);
    beast::get_lowest_layer(stream).connect(results);
    stream.handshake(ssl::stream_base::client);

    // we use our custom formatter for year_month_day objects because converting to sys_days
    // and then formatting changes the date (becomes a day earlier) for some reason (time zone
    // related maybe?? )

    std::string symbols;
    auto s = symbol_list_.begin();
    symbols += *s;
    for (++s; s != symbol_list_.end(); ++s)
    {
        symbols += ',';
        symbols += *s;
    }

    const std::string request =
        std::format("https://{}{}/?tickers={}&token={}", host_, websocket_prefix_, symbols, api_key_);

    http::request<http::string_body> req{http::verb::get, request, version_};
    req.set(http::field::host, host_);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::write(stream, req);

    beast::flat_buffer buffer;

    http::response<http::string_body> res;

    http::read(stream, buffer, res);
    std::string result = res.body();

    // shutdown without causing a 'stream_truncated' error.

    beast::get_lowest_layer(stream).cancel();
    beast::get_lowest_layer(stream).close();

    //    std::cout << "raw data: " << result << '\n';

    // I need to convert some numeric fields to string fields so they
    // won't be converted to floats and give me a bunch of extra decimal digits.
    // These values are nicely rounded by Tiingo.

    const std::regex source{R"***("(open|prevClose|last)":([0-9]*\.[0-9]*))***"};
    const std::string dest{R"***("$1":"$2")***"};
    const std::string result1 = std::regex_replace(result, source, dest);

    //    std::cout << "modified data: " << result1 << '\n';

    // now, just convert to JSON

    JSONCPP_STRING err;
    Json::Value response;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(result1.data(), result1.data() + result1.size(), &response, &err))
    {
        throw std::runtime_error("Problem parsing tiingo response: "s + err);
    }
    return response;
}  // -----  end of method Tiingo::GetTopOfBookAndLastClose  -----

std::vector<StockDataRecord> Tiingo::GetMostRecentTickerData(const std::string& symbol,
                                                             std::chrono::year_month_day start_from,
                                                             int how_many_previous, UseAdjusted use_adjusted,
                                                             const US_MarketHolidays* holidays)
{
    // we need to do some date arithmetic so we can use our basic 'GetTickerData' method.

    auto business_days = ConstructeBusinessDayRange(start_from, how_many_previous, UpOrDown::e_Down, holidays);
    //    std::cout << "business days: " << business_days.first << " : " << business_days.second << '\n';

    // we reverse the dates because we worked backwards from our given starting point and
    // Tiingo needs the dates in ascending order.

    const std::string ticker_data = GetTickerData(symbol, business_days.second, business_days.first, UpOrDown::e_Down);

    // we get 1 or more rows of csv data
    // NOTE: csv format result contains a header row
    // which we need to skip
    // NOTE: 'odd' sequence of fields
    // <date>,<close>,<high>,<low>,<open>,<volume>,<adjClose>,<adjHigh>,<adjLow>,<adjOpen>,<adjVolume>,<divCash>,<splitFactor>
    // so, let's just quickly parse them out.

    // <date>,<close>,<high>,<low>,<open>,<volume>,<adjClose>,<adjHigh>,<adjLow>,<adjOpen>,<adjVolume>,<divCash>,<splitFactor>
    enum fields
    {
        e_date = 0,
        e_open = 4,
        e_high = 2,
        e_low = 3,
        e_close = 1,
        e_volume = 5,
        e_adj_open = 9,
        e_adj_high = 7,
        e_adj_low = 8,
        e_adj_close = 6,
        e_adj_volume = 10,
        e_dividend = 11,
        e_split = 12
    };

    std::vector<StockDataRecord> stock_data;

    const auto rows = split_string<std::string_view>(ticker_data, "\n");

    rng::for_each(rows | vws::drop(1),
                  [&stock_data, symbol, use_adjusted](const auto row)
                  {
                      const auto fields = split_string<std::string_view>(row, ",");

                      if (use_adjusted == UseAdjusted::e_No)
                      {
                          StockDataRecord new_data{.date_ = std::string{fields[e_date]},
                                                   .symbol_ = symbol,
                                                   .open_ = sv2dec(fields[e_open]),
                                                   .high_ = sv2dec(fields[e_high]),
                                                   .low_ = sv2dec(fields[e_low]),
                                                   .close_ = sv2dec(fields[e_close])};
                          stock_data.push_back(new_data);
                      }
                      else
                      {
                          StockDataRecord new_data{.date_ = std::string{fields[e_date]},
                                                   .symbol_ = symbol,
                                                   .open_ = sv2dec(fields[e_adj_open]),
                                                   .high_ = sv2dec(fields[e_adj_high]),
                                                   .low_ = sv2dec(fields[e_adj_low]),
                                                   .close_ = sv2dec(fields[e_adj_close])};
                          stock_data.push_back(new_data);
                      }
                  });

    return stock_data;

}  // -----  end of method Tiingo::GetMostRecentTickerData  -----

std::string Tiingo::GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                                  std::chrono::year_month_day end_date, UpOrDown sort_asc)
{
    // if any problems occur here, we'll just let beast throw an exception.

    const std::string request_string = std::format(
        "https://{}/tiingo/daily/{}/prices?startDate={}&endDate={}&token={}&format={}&resampleFreq={}&sort={}", host_,
        symbol, start_date, end_date, api_key_, "csv", "daily", (sort_asc == UpOrDown::e_Up ? "date" : "-date"));

    return RequestData(request_string);

}  // -----  end of method Tiingo::GetTickerData  -----
