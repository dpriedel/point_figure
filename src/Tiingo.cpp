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

#include <algorithm>
#include <format>
#include <iostream>
#include <mutex>
#include <ranges>
#include <regex>
#include <string_view>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include "Tiingo.h"
#include "boost/beast/core/buffers_to_string.hpp"

#include "utilities.h"

using namespace std::string_literals;

//--------------------------------------------------------------------------------------
//       Class:  Tiingo
//      Method:  Tiingo
// Description:  constructor
//--------------------------------------------------------------------------------------

Tiingo::Tiingo()
    : ctx_{ssl::context::tlsv12_client},
      resolver_{ioc_},
      ws_{ioc_, ctx_} {}  // -----  end of method Tiingo::Tiingo  (constructor)  -----

Tiingo::~Tiingo()
{
    // need to disconnect if still connected.

    if (ws_.is_open())
    {
        Disconnect();
    }
}  // -----  end of method Tiingo::~Tiingo  -----

Tiingo::Tiingo(const std::string& host, const std::string& port, const std::string& api_key)
    : api_key_{api_key}, host_{host}, port_{port}, ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
{
}  // -----  end of method Tiingo::Tiingo  (constructor)  -----

Tiingo::Tiingo(const std::string& host, const std::string& port, const std::string& prefix, const std::string& api_key,
               const std::vector<std::string>& symbols)
    : symbol_list_{symbols},
      api_key_{api_key},
      host_{host},
      port_{port},
      websocket_prefix_{prefix},
      ctx_{ssl::context::tlsv12_client},
      resolver_{ioc_},
      ws_{ioc_, ctx_}
{
}  // -----  end of method Tiingo::Tiingo  (constructor)  -----

void Tiingo::Connect()
{
    // Look up the domain name
    auto const results = resolver_.resolve(host_, port_);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(get_lowest_layer(ws_), results);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
            "Failed to set SNI Hostname");
    }

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    auto host = host_ + ':' + std::to_string(ep.port());

    //    ws_.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

    // set timeout options so we don't hang forever if the
    // exchange is closed.

    beast::websocket::stream_base::timeout opt{
        std::chrono::seconds(30),  // handshake timeout
        std::chrono::seconds(20),  // idle timeout
        true                       // enable keep-alive pings
    };

    ws_.set_option(opt);

    // Perform the SSL handshake
    ws_.next_layer().handshake(ssl::stream_base::client);

    // Perform the websocket handshake
    ws_.handshake(host, websocket_prefix_);
    BOOST_ASSERT_MSG(ws_.is_open(), "Unable to complete websocket connection.");
}

void Tiingo::StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
{
    // put this here for now.
    // need to manually construct to get expected formate when serialized

    Json::Value connection_request;
    connection_request["eventName"] = "subscribe";
    connection_request["authorization"] = api_key_;
    connection_request["eventData"]["thresholdLevel"] = 5;
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

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    while (ws_.is_open())
    {
        buffer.clear();
        ws_.read(buffer);
        std::string buffer_content = beast::buffers_to_string(buffer.cdata());
        if (!buffer_content.empty())
        {
            const std::lock_guard<std::mutex> queue_lock(*data_mutex);
            streamed_data->push(std::move(buffer_content));
        }
        if (*had_signal == true)
        {
            StopStreaming();

            // do a last check for data

            buffer.clear();
            ws_.read(buffer);
            std::string buffer_content = beast::buffers_to_string(buffer.cdata());
            if (!buffer_content.empty())
            {
                const std::lock_guard<std::mutex> queue_lock(*data_mutex);
                streamed_data->push(std::move(buffer_content));
            }
            break;
        }
    }
    // if the websocket is closed on the server side or there is a timeout which in turn
    // will cause the websocket to be closed, let's set this flag so other processes which
    // may be watching it can know.

    if (!ws_.is_open())
    {
        *had_signal = true;
    }
}  // -----  end of method Tiingo::StreamData  -----

Tiingo::StreamedData Tiingo::ExtractData(const std::string& buffer)
{
    //    std::cout << "\nraw buffer: " << buffer << '\n';

    const std::regex numeric_trade_price{R"***(("T",(?:[^,]*,){8})([0-9]*\.[0-9]*),)***"};
    const std::regex quoted_trade_price{R"***("T",(?:[^,]*,){8}"([0-9]*\.[0-9]*)",)***"};
    const std::string string_trade_price{R"***($1"$2",)***"};
    const std::string zapped_buffer = std::regex_replace(buffer, numeric_trade_price, string_trade_price);
    //    std::cout << "\nzapped buffer: " << zapped_buffer << '\n';

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

    StreamedData pf_data;

    auto message_type = response["messageType"];
    if (message_type == "A")
    {
        auto data = response["data"];

        if (data[0] != "T")
        {
            return pf_data;
        }
        // extract our data

        std::smatch m;
        if (bool found_it = std::regex_search(zapped_buffer, m, quoted_trade_price); !found_it)
        {
            std::cout << "can't find trade price in buffer: " << buffer << '\n';
        }
        else
        {
            PF_Data new_value;
            new_value.subscription_id_ = subscription_id_;
            new_value.time_stamp_ = data[1].asCString();
            new_value.time_stamp_nanoseconds_utc_ = data[2].asInt64();
            new_value.ticker_ = data[3].asCString();
            rng::for_each(new_value.ticker_, [](char& c) { c = std::toupper(c); });
            new_value.last_price_ = decimal::Decimal{m[1].str()};
            new_value.last_size_ = data[10].asInt();

            pf_data.push_back(std::move(new_value));
        }

        //        std::cout << "new data: " << pf_data_.back().ticker_ << " : " << pf_data_.back().last_price_ << '\n';
    }
    else if (message_type == "I")
    {
        subscription_id_ = response["data"]["subscriptionId"].asString();
        //        std::cout << "json cpp subscription ID: " << subscription_id_ << '\n';
        return pf_data;
    }
    else if (message_type == "H")
    {
        // heartbeat , just return

        return pf_data;
    }
    else
    {
        throw std::runtime_error("unexpected message type: "s + message_type.asCString());
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

    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        { req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro"); }));

    // set timeout options so we don't hang forever if the
    // exchange is closed.

    beast::websocket::stream_base::timeout opt{
        std::chrono::seconds(30),  // handshake timeout
        std::chrono::seconds(20),  // idle timeout
        true                       // enable keep-alive pings
    };

    ws.set_option(opt);

    ws.handshake(host, websocket_prefix_);

    ws.write(net::buffer(std::string(disconnect_request_str)));

    beast::flat_buffer buffer;

    ws.read(buffer);

    try
    {
        beast::close_socket(get_lowest_layer(ws));
        //        ws.close(websocket::close_code::normal);
    }
    catch (std::exception& e)
    {
        std::cout << "Problem closing socket after clearing streaming symbols."s + e.what() << '\n';
    }

    //    std::cout << beast::make_printable(buffer.data()) << std::endl;

}  // -----  end of method Tiingo::StopStreaming  -----

void Tiingo::Disconnect()
{
    beast::flat_buffer buffer;

    try
    {
        //        ws_.close(websocket::close_code::normal);
        beast::close_socket(get_lowest_layer(ws_));
    }
    catch (std::exception& e)
    {
        std::cout << "Problem closing socket after disconnect command."s + e.what() << '\n';
    }
}

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

    http::request<http::string_body> req{http::verb::get, request.c_str(), version_};
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

    const auto ticker_data = GetTickerData(symbol, business_days.second, business_days.first, UpOrDown::e_Down);

    return ConvertJSONPriceHistory(symbol, ticker_data, how_many_previous, use_adjusted);

}  // -----  end of method Tiingo::GetMostRecentTickerData  -----

Json::Value Tiingo::GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
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

    const std::string request = std::format(
        "https://{}/tiingo/daily/{}/prices?startDate={}&endDate={}&token={}&format={}&resampleFreq={}&sort={}", host_,
        symbol, start_date, end_date, api_key_, "json", "daily", (sort_asc == UpOrDown::e_Up ? "date" : "-date"));

    http::request<http::string_body> req{http::verb::get, request.c_str(), version_};
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

    const std::regex source{R"***("(open|high|low|close|adjOpen|adjHigh|adjLow|adjClose)":\s*([0-9]*\.[0-9]*))***"};
    const std::string dest{R"***("$1":"$2")***"};
    const std::string result1 = std::regex_replace(result, source, dest);

    // std::cout << "modified data: " << result1 << '\n';

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
}  // -----  end of method Tiingo::GetTickerData  -----
