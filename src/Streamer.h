// =====================================================================================
//
//       Filename:  Streamer.h
//
//    Description:  Wrapper class for different streaming data sources
//
//        Version:  2.0
//        Created:  2024-04-01 09:28 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef _STREAMER_INC_
#define _STREAMER_INC_

#include <chrono>
#include <queue>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <spdlog/spdlog.h>

// #pragma GCC diagnostic pop

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "Uniqueifier.h"
#include "utilities.h"

// =====================================================================================
//        Class:  Streamer
//  Description:  wrapper for streamed data sources
// =====================================================================================

class RemoteDataSource
{
public:
    // use this as exception class to flag specific streaming error
    // so we can catch it

    struct StreamingEOF
    {
    };

    // some custom types to quiet clang-tidy warnings

    using Host = UniqType<std::string, struct Host_Tag>;
    using Port = UniqType<std::string, struct Port_Tag>;
    using APIKey = UniqType<std::string, struct API_Key_Tag>;
    using Prefix = UniqType<std::string, struct Prefix_Tag>;

    using UTC_TmPt_NanoSecs = std::chrono::utc_time<std::chrono::nanoseconds>;

    using TopOfBookList = std::vector<TopOfBookOpenAndLastClose>;

    // consolidated, unified supeset of streamed data from all sources.

    enum class EodMktStatus : int32_t
    {
        e_unknown,
        e_open,
        e_closed,
        e_extended_hours
    };

    struct PF_Data
    {
        std::string subscription_id_;
        std::string ticker_;                             // Ticker
        std::string time_stamp_;                         // Date
        UTC_TmPt_NanoSecs time_stamp_nanoseconds_utc_{}; // time_stamp
        decimal::Decimal last_price_{-1};                // Last Price
        int32_t last_size_{-1};                          // Last Size
        bool dark_pool_{false};
        EodMktStatus market_status_{EodMktStatus::e_unknown};
    };

    // ====================  LIFECYCLE     =======================================

    RemoteDataSource(); // constructor

    virtual ~RemoteDataSource();

    RemoteDataSource(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix);

    RemoteDataSource(const RemoteDataSource &rhs) = delete;
    RemoteDataSource(RemoteDataSource &&rhs) = delete;

    // ====================  ACCESSORS     =======================================

    std::string RequestData(const std::string &request_string);

    virtual TopOfBookList GetTopOfBookAndLastClose() = 0;
    virtual std::vector<StockDataRecord> GetMostRecentTickerData(const std::string &symbol,
                                                                 std::chrono::year_month_day start_from,
                                                                 int how_many_previous, UseAdjusted use_adjusted,
                                                                 const US_MarketHolidays *holidays) = 0;
    virtual PF_Data ExtractStreamedData(const std::string &buffer) = 0;

    // ====================  MUTATORS      =======================================

    void ConnectWS();
    void DisconnectWS();
    void StreamData(bool *had_signal, std::mutex *data_mutex, std::queue<std::string> *streamed_data);

    virtual void StartStreaming() = 0;
    virtual void StopStreaming() = 0;

    // for streaming or other data retrievals

    void UseSymbols(const std::vector<std::string> &symbols);

    // ====================  OPERATORS     =======================================

    RemoteDataSource &operator=(const RemoteDataSource &rhs) = delete;
    RemoteDataSource &operator=(RemoteDataSource &&rhs) = delete;

protected:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

    net::io_context ioc;
    ssl::context ctx;
    tcp::resolver resolver;
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws;
    int version = 11;

    std::vector<std::string> symbol_list;

    std::string host;
    std::string port;
    std::string api_key;
    std::string websocket_prefix;

private:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

}; // ----------  end of template class Streamer  ----------

template <> struct std::formatter<RemoteDataSource::PF_Data> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const RemoteDataSource::PF_Data &pdata, std::format_context &ctx) const
    {
        std::string record;
        std::format_to(std::back_inserter(record), "ticker: {}, price: {}, shares: {}, time: {}", pdata.ticker_,
                       pdata.last_price_, pdata.last_size_, pdata.time_stamp_);
        return formatter<std::string>::format(record, ctx);
    }
};

inline std::ostream &operator<<(std::ostream &os, const RemoteDataSource::PF_Data pf_data)
{
    std::cout << "ticker: " << pf_data.ticker_ << " price: " << pf_data.last_price_ << " shares: " << pf_data.last_size_
              << " time:" << pf_data.time_stamp_;
    return os;
}

#endif // ----- #ifndef _STREAMER_INC_  -----
