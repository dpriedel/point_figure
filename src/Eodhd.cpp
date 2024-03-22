// =====================================================================================
//
//       Filename:  Eodhd.cpp
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

#include <algorithm>
#include <charconv>
#include <exception>
#include <format>
#include <iostream>
#include <mutex>
#include <ranges>
#include <regex>
#include <string_view>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include "Eodhd.h"
#include "boost/beast/core/buffers_to_string.hpp"

using namespace std::string_literals;
using namespace std::chrono_literals;

//--------------------------------------------------------------------------------------
//       Class:  Eodhd
//      Method:  Eodhd
// Description:  constructor
//--------------------------------------------------------------------------------------
Eodhd::Eodhd(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix)
    : Streamer{host, port, api_key, prefix}
{
}  // -----  end of method Eodhd::Eodhd  (constructor)  -----

void Eodhd::StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
{
    StartStreaming();

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    while (ws_.is_open() && !*had_signal)
    {
        try
        {
            buffer.consume(buffer.size());
            ws_.read(buffer);
            ws_.text(ws_.got_text());
            std::string buffer_content = beast::buffers_to_string(buffer.cdata());
            if (!buffer_content.empty())
            {
                // std::cout << buffer_content << std::endl;
                // ExtractData(buffer_content);
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
                ioc_.restart();
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
                ioc_.restart();
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
        // StopStreaming(had_signal);

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
    Disconnect();

    // *had_signal = true;

}  // -----  end of method Eodhd::StreamData  -----

void Eodhd::StartStreaming()
{
    // we need to do our own connect/disconnect so we can handle any
    // interruptions in streaming.

    Connect();

    // message formats are 'simple' so let's just use RegExes to work with them.

    std::string ticker_list;
    ticker_list = symbol_list_.front();
    rng::for_each(symbol_list_ | vws::drop(1),
                  [&ticker_list](const auto& sym)
                  {
                      ticker_list += ", ";
                      ticker_list += sym;
                  });

    // NOTE: format requires '{{' and '}}' escaping for braces which are to be included in format output string.

    const auto subscribe_request_str = std::format(R"({{"action": "subscribe", "symbols": "{}"}})", ticker_list);

    // Send the message
    ws_.write(net::buffer(subscribe_request_str));

}  // -----  end of method Eodhd::StartStreaming  -----

Eodhd::PF_Data Eodhd::ExtractData(const std::string& buffer)
{
    // response format is 'simple' so we'll use RegExes here too.

    // {"s":"TGT","p":141,"c":[14,37,41],"v":1,"dp":false,"ms":"open","t":1706109542329}

    // std::cout << "\nraw buffer: " << buffer << std::endl;

    // make referencing the submatch groups more readable

    enum GroupNumber
    {
        e_all = 0,
        e_ticker = 1,
        e_price = 2,
        e_conditions = 3,
        e_volume = 4,
        e_dark_pool = 5,
        e_mkt_status = 6,
        e_time = 7
    };

    static const std::string kResponseString{
        R"***(\{"s":"(.*)","p":([.0-9]*),"c":(.*),"v":(.*),"dp":(false|true),"ms":"(open|closed|extended-hours)","t":([.0-9]*)\})***"};
    static const std::regex kResponseRegex{kResponseString};

    std::cmatch fields;
    const char* response_text = buffer.data();

    PF_Data new_value;

    if (bool matched_it = std::regex_match(response_text, fields, kResponseRegex); matched_it)
    {
        std::string_view tmp_fld{response_text + fields.position(e_time), static_cast<size_t>(fields.length(e_time))};
        int64_t time_value{};
        if (auto [p, ec] = std::from_chars(tmp_fld.begin(), tmp_fld.end(), time_value); ec != std::errc())
        {
            throw std::runtime_error(
                std::format("Problem converting transaction time to int64: {}\n", std::make_error_code(ec).message()));
        }
        std::chrono::milliseconds ms{time_value};
        new_value.time_stamp_nanoseconds_utc_ = TmPt{std::chrono::duration_cast<std::chrono::nanoseconds>(ms)};

        new_value.ticker_ =
            std::string_view{response_text + fields.position(e_ticker), static_cast<size_t>(fields.length(e_ticker))};

        tmp_fld = {response_text + fields.position(e_price), static_cast<size_t>(fields.length(e_price))};
        new_value.last_price_ = decimal::Decimal{std::string{tmp_fld}};

        tmp_fld = {response_text + fields.position(e_volume), static_cast<size_t>(fields.length(e_volume))};
        if (auto [p, ec] = std::from_chars(tmp_fld.begin(), tmp_fld.end(), new_value.last_size_); ec != std::errc())
        {
            throw std::runtime_error(std::format("Problem converting transaction volume to int64: {}\n",
                                                 std::make_error_code(ec).message()));
        }

        tmp_fld = {response_text + fields.position(e_dark_pool), static_cast<size_t>(fields.length(e_dark_pool))};
        new_value.dark_pool_ = tmp_fld == "true";

        tmp_fld = {response_text + fields.position(e_mkt_status), static_cast<size_t>(fields.length(e_mkt_status))};

        if (tmp_fld == "open")
        {
            new_value.market_status_ = EodMktStatus::e_open;
        }
        else if (tmp_fld == "closed")
        {
            new_value.market_status_ = EodMktStatus::e_closed;
        }
        else if (tmp_fld == "extended-hours")
        {
            new_value.market_status_ = EodMktStatus::e_extended_hours;
        }
        else
        {
            new_value.market_status_ = EodMktStatus::e_unknown;
        }
    }
    else
    {
        spdlog::error(std::format("can't parse transaction buffer: ->{}<-", buffer));
    }

    return new_value;
}  // -----  end of method Eodhd::ExtractData  -----

void Eodhd::StopStreaming()
{
    // we need to send the unsubscribe message in a separate connection.

    std::string ticker_list;
    ticker_list = symbol_list_.front();
    rng::for_each(symbol_list_ | vws::drop(1),
                  [&ticker_list](const auto& sym)
                  {
                      ticker_list += ", ";
                      ticker_list += sym;
                  });

    const auto unsubscribe_request_str = std::format(R"({{"action": "unsubscribe", "symbols": "{}"}})", ticker_list);
    // std::cout << "Jsoncpp disconnect_request_str: " << disconnect_request_str << '\n';

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
        ws.write(net::buffer(unsubscribe_request_str));

        beast::flat_buffer buffer;

        ws.read(buffer);

        // beast::close_socket(get_lowest_layer(ws));
        ws.close(websocket::close_code::normal);
        //        ws.close(websocket::close_code::normal);
    }
    catch (std::exception& e)
    {
        spdlog::error("Problem closing socket after clearing streaming symbols: {}.", e.what());
    }

    Disconnect();

    // *had_signal = true;

}  // -----  end of method Eodhd::StopStreaming  -----

Json::Value Eodhd::GetTopOfBookAndLastClose()
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

    // I need to convert some numeric fields to string fields so they
    // won't be converted to floats and give me a bunch of extra decimal digits.
    // These values are nicely rounded by Eodhd.

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
}  // -----  end of method Eodhd::GetTopOfBookAndLastClose  -----

std::vector<StockDataRecord> Eodhd::GetMostRecentTickerData(const std::string& symbol,
                                                            std::chrono::year_month_day start_from,
                                                            int how_many_previous, UseAdjusted use_adjusted,
                                                            const US_MarketHolidays* holidays)
{
    // we need to do some date arithmetic so we can use our basic 'GetTickerData' method.

    auto business_days = ConstructeBusinessDayRange(start_from, how_many_previous, UpOrDown::e_Down, holidays);
    //    std::cout << "business days: " << business_days.first << " : " << business_days.second << '\n';

    // we reverse the dates because we worked backwards from our given starting point and
    // Eodhd needs the dates in ascending order.

    const auto ticker_data = GetTickerData(symbol, business_days.second, business_days.first, UpOrDown::e_Down);

    return ConvertJSONPriceHistory(symbol, ticker_data, how_many_previous, use_adjusted);

}  // -----  end of method Eodhd::GetMostRecentTickerData  -----

Json::Value Eodhd::GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                                 std::chrono::year_month_day end_date, UpOrDown sort_asc)
{
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

    // const std::string request = std::format(
    //     "https://{}/tiingo/daily/{}/prices?startDate={}&endDate={}&token={}&format={}&resampleFreq={}&sort={}",
    //     host_, symbol, start_date, end_date, api_key_, "json", "daily", (sort_asc == UpOrDown::e_Up ? "date" :
    //     "-date"));

    const std::string request =
        std::format("https://{}/api/eod/{}.US?from={}&to={}&order={}&period=d&api_token={}&fmt=json", host_, symbol,
                    start_date, end_date, (sort_asc == UpOrDown::e_Up ? "a" : "d"), api_key_);
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

    // I need to convert some numeric fields to string fields so they
    // won't be converted to floats and give me a bunch of extra decimal digits.
    // These values are nicely rounded by Eodhd.

    const std::regex source{R"***("(open|high|low|close|adjusted_close)":\s*([0-9]*(?:\.[0-9]*)?))***"};
    const std::string dest{R"***("$1":"$2")***"};
    const std::string result1 = std::regex_replace(result, source, dest);

    // now, just convert to JSON

    JSONCPP_STRING err;
    Json::Value response;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(result1.data(), result1.data() + result1.size(), &response, &err))
    {
        throw std::runtime_error("Problem parsing Eodhd response: "s + err);
    }
    return response;
}  // -----  end of method Eodhd::GetTickerData  -----
