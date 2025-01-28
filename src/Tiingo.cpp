// =====================================================================================
//
//       Filename:  Tiingo.cpp
//
//    Description:  Live stream ticker updates
//
//        Version:  3.0
//        Created:  2024-04-01 10:10 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)/
// =====================================================================================
// the guts of this code comes from the examples distributed by Boost.

#include <format>
#include <ranges>
#include <regex>
#include <sstream>

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
    : RemoteDataSource{host, port, api_key, prefix}
{
}  // -----  end of method Tiingo::Tiingo  (constructor)  -----

void Tiingo::StartStreaming()
{
    // we need to do our own connect/disconnect so we can handle any
    // interruptions in streaming.

    ConnectWS();

    Json::Value connection_request;
    connection_request["eventName"] = "subscribe";
    connection_request["authorization"] = api_key;
    connection_request["eventData"]["thresholdLevel"] = 6;
    Json::Value tickers(Json::arrayValue);
    for (const auto& symbol : symbol_list)
    {
        tickers.append(symbol);
    }

    connection_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // compact printing and string formatting
    const std::string connection_request_str = Json::writeString(builder, connection_request);
    //    std::cout << "Jsoncpp connection_request_str: " << connection_request_str << '\n';

    // Send the message
    ws.write(net::buffer(connection_request_str));

    // let's make sure we were successful

    beast::flat_buffer buffer;

    ws.read(buffer);
    ws.text(ws.got_text());
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

Tiingo::PF_Data Tiingo::ExtractStreamedData(const std::string& buffer)
{
    // std::cout << "\nraw buffer: " << buffer << std::endl;

    static const std::regex kNumericTradePrice{R"***(("data":\["(?:[^,]*,){2})([0-9]*\.[0-9]*)])***"};
    static const std::regex kQuotedTradePrice{R"***(("data":\[(?:[^,]*,){2})"([0-9]*\.[0-9]*)")***"};
    static const std::string kStringTradePrice{R"***($1"$2"])***"};
    const std::string zapped_buffer = std::regex_replace(buffer, kNumericTradePrice, kStringTradePrice);
    // std::cout << "\nzapped buffer: " << zapped_buffer << std::endl;

    JSONCPP_STRING err;
    Json::Value response;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    if (!reader->parse(zapped_buffer.data(), zapped_buffer.data() + zapped_buffer.size(), &response, &err))
    {
        throw std::runtime_error("Problem parsing tiingo response: "s + err);
    }

    PF_Data new_value;

    auto message_type = response["messageType"];
    if (strcmp(message_type.asCString(), "A") == 0)
    {
        auto data = response["data"];

        std::smatch m;
        if (bool found_it = std::regex_search(zapped_buffer, m, kQuotedTradePrice); !found_it)
        {
            std::cout << "can't find trade price in buffer: " << buffer << '\n';
        }
        else
        {
            new_value.subscription_id_ = subscription_id_;
            new_value.time_stamp_ = data[0].asCString();
            std::istringstream(new_value.time_stamp_) >>
                std::chrono::parse("%FT%T%Ez", new_value.time_stamp_nanoseconds_utc_);
            new_value.ticker_ = data[1].asCString();
            rng::for_each(new_value.ticker_, [](char& c) { c = std::toupper(c); });
            // std::cout << std::format("{}\n", new_value.time_stamp_nanoseconds_utc_);
            new_value.last_price_ = decimal::Decimal{m[2].str()};
            new_value.last_size_ = 100;  // not reported by new Tiingo IEX data so just use a standard number
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
    return new_value;
}  // -----  end of method Tiingo::ExtractData  -----

void Tiingo::StopStreaming()
{
    // we need to send the unsubscribe message in a separate connection.

    Json::Value disconnect_request;
    disconnect_request["eventName"] = "unsubscribe";
    disconnect_request["authorization"] = api_key;
    disconnect_request["eventData"]["subscriptionId"] = subscription_id_;
    Json::Value tickers(Json::arrayValue);
    for (const auto& symbol : symbol_list)
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

    auto tmp_host = host;
    auto tmp_port = port;

    auto const results = resolver.resolve(tmp_host, tmp_port);

    auto ep = net::connect(get_lowest_layer(ws), results);

    if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), tmp_host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
            "Failed to set SNI Hostname");
    }

    tmp_host += ':' + std::to_string(ep.port());

    ws.next_layer().handshake(ssl::stream_base::client);

    ws.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

    ws.handshake(tmp_host, websocket_prefix);

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

Tiingo::TopOfBookList Tiingo::GetTopOfBookAndLastClose()
{
    // using the REST API for iex.
    // via the base class common request method.
    // if any problems occur here, we'll just let beast throw an exception.

    std::string symbols;
    auto s = symbol_list.begin();
    symbols += *s;
    for (++s; s != symbol_list.end(); ++s)
    {
        symbols += ',';
        symbols += *s;
    }

    const std::string request_string =
        std::format("https://{}{}/?tickers={}&token={}&format=csv", host, "/iex", symbols, api_key);

    const auto data = RequestData(request_string);

    // now, parse our our csv data
    // <ticker>,<askPrice>,<askSize>,<bidPrice>,<bidSize>,<high>,<last>,<lastSize>,<lastSaleTimestamp>,<low>,<mid>,<open>,
    // <prevClose>,<quoteTimestamp>,<timestamp>,<tngoLast>,<volume>

    enum Fields
    {
        e_symbol_ = 0,
        e_timestamp = 14,
        e_open = 11,
        e_close = 6,
        e_previous_close = 12
    };

    TopOfBookList stock_data;

    const auto rows = split_string<std::string_view>(data, "\n");

    rng::for_each(rows | vws::drop(1),
                  [&stock_data](const auto row)
                  {
                      const auto fields = split_string<std::string_view>(row, ",");

                      // a few checks to try to catch any changes in response format

                      BOOST_ASSERT_MSG(fields.size() == 17,
                                       std::format("Missing 1 or more fields from response: '{}'. Expected 17. Got: {}",
                                                   row, fields.size())
                                           .c_str());

                      const auto tstmp = StringToUTCTimePoint("%FT%T%z", fields[e_timestamp]);

                      TopOfBookOpenAndLastClose new_data{.symbol_ = std::string{fields[e_symbol_]},
                                                         .time_stamp_nsecs_ = tstmp,
                                                         .open_ = sv2dec(fields[e_open]),
                                                         .last_ = sv2dec(fields[e_close]),
                                                         .previous_close_ = sv2dec(fields[e_previous_close])};

                      stock_data.push_back(new_data);
                  });
    return stock_data;
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

                      // a few checks to try to catch any changes in response format

                      BOOST_ASSERT_MSG(fields.size() == 13,
                                       std::format("Missing 1 or more fields from response: '{}'. Expected 13. Got: {}",
                                                   row, fields.size())
                                           .c_str());

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
        "https://{}/tiingo/daily/{}/prices?startDate={}&endDate={}&token={}&format={}&resampleFreq={}&sort={}", host,
        symbol, start_date, end_date, api_key, "csv", "daily", (sort_asc == UpOrDown::e_Up ? "date" : "-date"));

    return RequestData(request_string);

}  // -----  end of method Tiingo::GetTickerData  -----
