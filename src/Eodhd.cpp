// =====================================================================================
//
//       Filename:  Eodhd.cpp
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
// =====================================================================================

#include "Eodhd.h"
#include "boost/beast/core/buffers_to_string.hpp"
#include <boost/regex.hpp>
#include <boost/regex/config.hpp>
#include <print>
#include <ranges>
#include <utility>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

using namespace std::string_view_literals;

Eodhd::Eodhd(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix)
    : RemoteDataSource{host, port, api_key, prefix}
{
}

void Eodhd::OnConnected()
{
    // Construct Subscription String
    std::string ticker_list;
    ticker_list = symbol_list_.front();
    rng::for_each(symbol_list_ | vws::drop(1), [&ticker_list](const auto &sym) {
        ticker_list += ", ";
        ticker_list += sym;
    });

    const auto subscribe_request_str = std::format(R"({{"action": "subscribe", "symbols": "{}"}})", ticker_list);

    // Async Write
    ws_.value().async_write(net::buffer(subscribe_request_str),
                            beast::bind_front_handler(&Eodhd::on_write_subscribe, this));
}

void Eodhd::on_write_subscribe(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        spdlog::error("Eodhd subscribe write failed: {}", ec.message());
        return;
    }

    // Async Read Response
    buffer_.clear();
    ws_.value().async_read(buffer_, beast::bind_front_handler(&Eodhd::on_read_subscribe, this));
}

void Eodhd::on_read_subscribe(beast::error_code ec, std::size_t bytes_transferred)
{
    if (ec)
    {
        spdlog::error("Eodhd subscribe read failed: {}", ec.message());
        return;
    }

    std::string buffer_content = beast::buffers_to_string(buffer_.cdata());

    // Check for success code
    if (!buffer_content.starts_with(R"***({"status_code":200,)***"))
    {
        spdlog::error("Failed to get success code. Got: {}", buffer_content);
        return;
    }

    spdlog::info("Eodhd Subscribed Successfully.");
    StartReadLoop();
}

Eodhd::PF_Data Eodhd::ExtractStreamedData(const std::string &buffer)
{

    // response format is 'simple' so we'll use RegExes here too.

    // {"s":"TGT","p":141,"c":[14,37,41],"v":1,"dp":false,"ms":"open","t":1706109542329}

    // make referencing the submatch groups more readable

    enum GroupNumber : int
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

    // enum GroupNumber
    // {
    //     e_all = 0,
    //     e_ticker = 1,
    //     e_price = 2,
    //     e_volume = 3,
    //     e_mkt_status = 4,
    //     e_conditions = 5,
    //     e_dark_pool = 6,
    //     e_time = 7
    // };

    static const std::string kResponseString{
        R"***(\{"s":"(.*)","p":([.0-9]*),"c":(.*),"v":(.*),"dp":(false|true),"ms":"(open|closed|close|extended-hours)?","t":([.0-9]*)\})***"};
    // static const std::string kResponseString{
    //     R"***(\{"s":"(.*)","p":([.0-9]*),"v":(.*),"e":(.*),"c":(.*),"dp":(false|true),"t":([.0-9]*)\})***"};
    static const boost::regex kResponseRegex{kResponseString};

    boost::cmatch fields;
    const char *response_text = buffer.data();

    PF_Data new_value;

    if (bool matched_it = boost::regex_match(response_text, fields, kResponseRegex); matched_it)
    {
        std::string_view tmp_fld(response_text + fields.position(e_time), fields.length(std::to_underlying(e_time)));

        // EODHD provides timestamp with milliseconds resolution.
        new_value.time_stamp_ = tmp_fld;
        // make it into nanoseconds.
        new_value.time_stamp_.append("000000"sv);

        int64_t time_value{};
        if (auto [p, ec] = std::from_chars(new_value.time_stamp_.data(),
                                           new_value.time_stamp_.data() + new_value.time_stamp_.size(), time_value);
            ec != std::errc())
        {
            throw std::runtime_error(std::format("Problem converting transaction timestamp to int64: {}\n",
                                                 std::make_error_code(ec).message()));
        }
        new_value.time_stamp_nanoseconds_utc_ = UTC_TmPt_NanoSecs{std::chrono::nanoseconds{time_value}};

        new_value.ticker_ =
            std::string_view(response_text + fields.position(e_ticker), fields.length(std::to_underlying(e_ticker)));

        tmp_fld =
            std::string_view(response_text + fields.position(e_price), fields.length(std::to_underlying(e_price)));
        new_value.last_price_ = decimal::Decimal{std::string{tmp_fld}};

        tmp_fld =
            std::string_view(response_text + fields.position(e_volume), fields.length(std::to_underlying(e_volume)));
        if (auto [p, ec] = std::from_chars(tmp_fld.begin(), tmp_fld.end(), new_value.last_size_); ec != std::errc())
        {
            throw std::runtime_error(std::format("Problem converting transaction volume to int64: {}\n",
                                                 std::make_error_code(ec).message()));
        }

        tmp_fld = std::string_view(response_text + fields.position(e_dark_pool),
                                   fields.length(std::to_underlying(e_dark_pool)));
        new_value.dark_pool_ = tmp_fld == "true";

        tmp_fld = std::string_view(response_text + fields.position(e_mkt_status),
                                   fields.length(std::to_underlying(e_mkt_status)));

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
}

void Eodhd::StopStreaming(StreamerContext &streamer_context)
{

    // tell our extractor code we are really done

    {
        std::lock_guard<std::mutex> lock(streamer_context.mtx_);
        streamer_context.done_ = true;
    }
    streamer_context.cv_.notify_one();

    // we need to send the unsubscribe message in a separate connection.

    std::string ticker_list;
    ticker_list = symbol_list_.front();
    rng::for_each(symbol_list_ | vws::drop(1), [&ticker_list](const auto &sym) {
        ticker_list += ", ";
        ticker_list += sym;
    });

    const auto unsubscribe_request_str = std::format(R"({{"action": "unsubscribe", "symbols": "{}"}})", ticker_list);
    // std::cout << "Jsoncpp disconnect_request_str: " << disconnect_request_str << '\n';

    // just grab the code from the example program

    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};

    tcp::resolver resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>, false> tmp_ws{ioc, ctx};

    auto tmp_host = host_;
    auto tmp_port = port_;

    auto const results = resolver.resolve(tmp_host, tmp_port);

    auto ep = net::connect(get_lowest_layer(tmp_ws), results);

    if (!SSL_set_tlsext_host_name(tmp_ws.next_layer().native_handle(), tmp_host.c_str()))
    {
        throw beast::system_error(
            beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
            "Failed to set SNI Hostname");
    }

    tmp_host += ':' + std::to_string(ep.port());

    tmp_ws.next_layer().handshake(ssl::stream_base::client);

    tmp_ws.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

    tmp_ws.handshake(tmp_host, websocket_prefix_);

    try
    {
        tmp_ws.write(net::buffer(unsubscribe_request_str));

        beast::flat_buffer buffer;

        tmp_ws.read(buffer);

        // beast::close_socket(get_lowest_layer(ws));
        tmp_ws.close(websocket::close_code::normal);
        //        ws.close(websocket::close_code::normal);
    }
    catch (std::exception &e)
    {
        spdlog::error("Problem closing socket after clearing streaming symbols: {}.", e.what());
    }

    DisconnectWS();

    // *had_signal = true;
}

// ... [Includes GetTopOfBookAndLastClose and GetMostRecentTickerData implementation from original file] ...
RemoteDataSource::TopOfBookList Eodhd::GetTopOfBookAndLastClose()
{

    // this needs to be done differently for Eodhd since they don't provide
    // the function Tiingo does.
    // Eod does provide delayed by 15 to 20 minutes'live' API which
    // returns data for a 1-minute interval.
    // If you try this too soon after open, the values returned are zeros.
    // csv response looks like:
    // code,timestamp,gmtoffset,open,high,low,close,volume,previousClose,change,change_p
    //

    // NOTE: csv format result contains a header row
    // which we need to skip
    // <code>,<timestamp>,<gmtoffset>,<open>,<high>,<low>,<close>,<volume>,<previousClose>,<change>,<change_p>
    // so, let's just quickly parse them out.
    enum Fields
    {
        e_timestamp = 1,
        e_open = 3,
        e_close = 6,
        e_previous_close = 8
    };

    TopOfBookList stock_data;

    for (const auto &symbol : symbol_list_)
    {
        std::string request_string =
            std::format("https://{}/api/real-time/{}.US?api_token={}&fmt=csv", host_, symbol, api_key_);
        const auto tob_data = RequestData(request_string);

        const auto rows = split_string<std::string_view>(tob_data, "\n");
        const auto fields = split_string<std::string_view>(rows[1], ",");

        // a few checks to try to catch any changes in response format

        BOOST_ASSERT_MSG(
            fields.size() == 11,
            std::format("Missing 1 or more fields from response: '{}'. Expected 11. Got: {}", tob_data, fields.size())
                .c_str());

        BOOST_ASSERT_MSG(fields[e_timestamp] != "NA" && fields[e_open] != "NA",
                         std::format("No ToB data found for symbol: {}", symbol).c_str());
        std::println("ToB data: {}", rows[1]);
        std::string time_fld{fields[e_timestamp]};

        // EODHD provides the timestamp in seconds.  we need to convert to nanoseconds.

        time_fld.append("000000000");

        int64_t time_value{};
        if (auto [p, ec] = std::from_chars(&time_fld.front(), &time_fld.back(), time_value); ec != std::errc())
        {
            throw std::runtime_error(std::format("Problem converting transaction timestamp: {} to int64: {}\n",
                                                 time_fld, std::make_error_code(ec).message()));
        }
        std::chrono::seconds secs{time_value};
        TopOfBookOpenAndLastClose new_data{.symbol_ = symbol,
                                           .time_stamp_nsecs_ =
                                               std::chrono::utc_clock::time_point{std::chrono::nanoseconds{secs}},
                                           .open_ = sv2dec(fields[e_open]),
                                           .last_ = sv2dec(fields[e_close]),
                                           .previous_close_ = sv2dec(fields[e_previous_close])};

        stock_data.push_back(new_data);
    }
    return stock_data;
}
std::vector<StockDataRecord> Eodhd::GetMostRecentTickerData(const std::string &symbol,
                                                            std::chrono::year_month_day start_from,
                                                            int how_many_previous, UseAdjusted use_adjusted,
                                                            const US_MarketHolidays *holidays)
{
    // we need to do some date arithmetic so we can use our basic 'GetTickerData' method.

    auto business_days = ConstructeBusinessDayRange(start_from, how_many_previous, UpOrDown::e_Down, holidays);
    //    std::cout << "business days: " << business_days.first << " : " << business_days.second << '\n';

    // we reverse the dates because we worked backwards from our given starting point and
    // Eodhd needs the dates in ascending order.

    const std::string ticker_data = GetTickerData(symbol, business_days.second, business_days.first, UpOrDown::e_Down);

    // we get 1 or more rows of csv data
    // NOTE: csv format result contains a header row
    // which we need to skip
    // <date>,<open>,<high>,<low>,<close>,<adjusted close>,<volume>
    // so, let's just quickly parse them out.

    enum fields
    {
        e_date = 0,
        e_open = 1,
        e_high = 2,
        e_low = 3,
        e_close = 4,
        e_adj_close = 5,
        e_volume = 6
    };

    std::vector<StockDataRecord> stock_data;

    const auto rows = split_string<std::string_view>(ticker_data, "\n");
    BOOST_ASSERT_MSG(rows.size() > 1, std::format("No recent data returned for symbol: {}", symbol).c_str());

    rng::for_each(rows | vws::drop(1), [&stock_data, symbol, use_adjusted](const auto row) {
        const auto fields = split_string<std::string_view>(row, ",");

        // a few checks to try to catch any changes in response format

        BOOST_ASSERT_MSG(
            fields.size() == 7,
            std::format("Missing 1 or more fields from response: '{}'. Expected 7. Got: {}", row, fields.size())
                .c_str());

        StockDataRecord new_data{.date_ = std::string{fields[e_date]},
                                 .symbol_ = symbol,
                                 .open_ = sv2dec(fields[e_open]),
                                 .high_ = sv2dec(fields[e_high]),
                                 .low_ = sv2dec(fields[e_low]),
                                 .close_ = decimal::Decimal{sv2dec(
                                     use_adjusted == UseAdjusted::e_Yes ? fields[e_adj_close] : fields[e_close])}};
        stock_data.push_back(new_data);
    });

    return stock_data;
}
std::string Eodhd::GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                                 std::chrono::year_month_day end_date, UpOrDown sort_asc)
{
    // if any problems occur here, we'll just let beast throw an exception.
    const std::string request_string =
        std::format("https://{}/api/eod/{}.US?from={}&to={}&order={}&period=d&api_token={}&fmt=csv", host_, symbol,
                    start_date, end_date, (sort_asc == UpOrDown::e_Up ? "a" : "d"), api_key_);

    return RequestData(request_string);
}
