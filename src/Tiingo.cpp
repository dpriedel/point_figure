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
//        License:  GNU General Public License -v3
//
// =====================================================================================

#include "Tiingo.h"
#include <boost/regex.hpp>
#include <format>
#include <ranges>
#include <sstream>

#include <json/json.h> // Ensure jsoncpp is available

namespace rng = std::ranges;
namespace vws = std::ranges::views;
using namespace std::string_literals;

Tiingo::Tiingo(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix)
    : RemoteDataSource{host, port, api_key, prefix}
{
}

void Tiingo::OnConnected()
{
    // Construct Subscription JSON
    Json::Value connection_request;
    connection_request["eventName"] = "subscribe";
    connection_request["authorization"] = api_key_;
    connection_request["eventData"]["thresholdLevel"] = 6;
    Json::Value tickers(Json::arrayValue);
    for (const auto &symbol : symbol_list_)
    {
        tickers.append(symbol);
    }
    connection_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const std::string connection_request_str = Json::writeString(builder, connection_request);

    // Async Write
    ws_.value().async_write(net::buffer(connection_request_str),
                            beast::bind_front_handler(&Tiingo::on_write_subscribe, this));
}

void Tiingo::on_write_subscribe(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        spdlog::error("Tiingo subscribe write failed: {}", ec.message());
        return;
    }

    // Async Read Response
    buffer_.clear();
    ws_.value().async_read(buffer_, beast::bind_front_handler(&Tiingo::on_read_subscribe, this));
}

void Tiingo::on_read_subscribe(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        spdlog::error("Tiingo subscribe read failed: {}", ec.message());
        return;
    }

    std::string buffer_content = beast::buffers_to_string(buffer_.cdata());
    JSONCPP_STRING err;
    Json::Value response;
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());

    if (!reader->parse(buffer_content.data(), buffer_content.data() + buffer_content.size(), &response, &err))
    {
        spdlog::error("Problem parsing tiingo response: {}", err);
        return;
    }

    std::string message_type = response["messageType"].asString();
    if (message_type != "I")
    {
        spdlog::error("Expected message type of 'I'. Got: {}", message_type);
        return;
    }

    subscription_id_ = response["data"]["subscriptionId"].asString();
    spdlog::info("Tiingo Subscribed. ID: {}", subscription_id_);

    // Success! Start the main read loop in base class
    StartReadLoop();
}

Tiingo::PF_Data Tiingo::ExtractStreamedData(const std::string &buffer)
{
    // Tiingo only provides 3 fields for its 'free' IEX feed
    // - nanoseconds timestamp as fully formatted text string
    // - symbol
    // - price as a 2-digit float.

    static const boost::regex kNumericTradePrice{R"***(("data":\["(?:[^,]*,){2})([0-9]*\.[0-9]*)])***"};
    static const boost::regex kQuotedTradePrice{R"***(("data":\[(?:[^,]*,){2})"([0-9]*\.[0-9]*)")***"};
    static const std::string kStringTradePrice{R"***($1"$2"])***"};
    const std::string zapped_buffer = boost::regex_replace(buffer, kNumericTradePrice, kStringTradePrice);

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

        boost::smatch m;
        if (bool found_it = boost::regex_search(zapped_buffer, m, kQuotedTradePrice); !found_it)
        {
            spdlog::error("can't find trade price in buffer: {}", buffer);
        }
        else
        {
            new_value.subscription_id_ = subscription_id_;
            new_value.time_stamp_ = data[0].asCString();
            std::istringstream(new_value.time_stamp_) >>
                std::chrono::parse("%FT%T%Ez", new_value.time_stamp_nanoseconds_utc_);
            new_value.ticker_ = data[1].asCString();
            rng::for_each(new_value.ticker_, [](char &c) { c = std::toupper(c); });
            new_value.last_price_ = decimal::Decimal{m[2].str()};
            new_value.last_size_ = 100; // not reported by new Tiingo IEX data so just use a standard number
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
}

void Tiingo::StopStreaming(StreamerContext &streamer_context)
{

    // tell our extractor code we are really done

    {
        std::lock_guard<std::mutex> lock(streamer_context.mtx_);
        streamer_context.done_ = true;
    }
    streamer_context.cv_.notify_one();

    // we need to send the unsubscribe message in a separate connection.

    Json::Value disconnect_request;
    disconnect_request["eventName"] = "unsubscribe";
    disconnect_request["authorization"] = api_key_;
    disconnect_request["eventData"]["subscriptionId"] = subscription_id_;
    Json::Value tickers(Json::arrayValue);
    for (const auto &symbol : symbol_list_)
    {
        tickers.append(symbol);
    }

    disconnect_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = ""; // compact printing and string formatting
    const std::string disconnect_request_str = Json::writeString(builder, disconnect_request);

    // just grab the code from the example program

    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};

    tcp::resolver resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws{ioc, ctx};

    auto tmp_host = host_;
    auto tmp_port = port_;

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

    ws.handshake(tmp_host, websocket_prefix_);

    try
    {
        ws.write(net::buffer(std::string(disconnect_request_str)));

        beast::flat_buffer buffer;

        ws.read(buffer);

        ws.close(websocket::close_code::normal);
    }
    catch (std::exception &e)
    {
        spdlog::error(std::format("Problem closing socket after clearing streaming symbols: {}", e.what()));
    }

    DisconnectWS();
}

// These methods use RequestData which is now synchronous and separate.
RemoteDataSource::TopOfBookList Tiingo::GetTopOfBookAndLastClose()
{

    // using the REST API for iex.
    // via the base class common request method.
    // if any problems occur here, we'll just let beast throw an exception.

    std::string symbols;
    auto s = symbol_list_.begin();
    symbols += *s;
    for (++s; s != symbol_list_.end(); ++s)
    {
        symbols += ',';
        symbols += *s;
    }

    const std::string request_string =
        std::format("https://{}{}/?tickers={}&token={}&format=csv", host_, websocket_prefix_, symbols, api_key_);

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
        e_previous_close = 12,
        e_tiingo_last = 15
    };

    TopOfBookList stock_data;

    const auto rows = split_string<std::string_view>(data, "\n");

    rng::for_each(rows | vws::drop(1), [&stock_data](const auto row) {
        const auto fields = split_string<std::string_view>(row, ",");

        // a few checks to try to catch any changes in response format

        BOOST_ASSERT_MSG(
            fields.size() == 17,
            std::format("Missing 1 or more fields from response: '{}'. Expected 17. Got: {}", row, fields.size())
                .c_str());

        // if we have any problems with this data, skip the row an try the next.
        // missing data here is not very important.
        try
        {
            const auto tstmp = StringToUTCTimePoint("%FT%T%z", fields[e_timestamp]);

            TopOfBookOpenAndLastClose new_data{.symbol_ = std::string{fields[e_symbol_]},
                                               .time_stamp_nsecs_ = tstmp,
                                               .open_ = sv2dec(fields[e_open]),
                                               .last_ = sv2dec(fields[e_tiingo_last]),
                                               .previous_close_ = sv2dec(fields[e_previous_close])};
            stock_data.push_back(new_data);
        }
        catch (const std::exception &e)
        {
            spdlog::error(e.what());
        }
    });
    return stock_data;
}
std::vector<StockDataRecord> Tiingo::GetMostRecentTickerData(const std::string &symbol,
                                                             std::chrono::year_month_day start_from,
                                                             int how_many_previous, UseAdjusted use_adjusted,
                                                             const US_MarketHolidays *holidays)
{
    // we need to do some date arithmetic so we can use our basic 'GetTickerData' method.

    auto business_days = ConstructeBusinessDayRange(start_from, how_many_previous, UpOrDown::e_Down, holidays);

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

    rng::for_each(rows | vws::drop(1), [&stock_data, symbol, use_adjusted](const auto row) {
        const auto fields = split_string<std::string_view>(row, ",");

        // a few checks to try to catch any changes in response format

        BOOST_ASSERT_MSG(
            fields.size() == 13,
            std::format("Missing 1 or more fields from response: '{}'. Expected 13. Got: {}", row, fields.size())
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

} // -----  end of method Tiingo::GetMostRecentTickerData  -----

std::string Tiingo::GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                                  std::chrono::year_month_day end_date, UpOrDown sort_asc)
{
    // if any problems occur here, we'll just let beast throw an exception.

    const std::string request_string = std::format(
        "https://{}/tiingo/daily/{}/prices?startDate={}&endDate={}&token={}&format={}&resampleFreq={}&sort={}", host_,
        symbol, start_date, end_date, api_key_, "csv", "daily", (sort_asc == UpOrDown::e_Up ? "date" : "-date"));

    return RequestData(request_string);

} // -----  end of method Tiingo::GetTickerData  -----
