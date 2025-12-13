// =====================================================================================
//       Filename:  Streamer.h
//    Description:  Wrapper class for different streaming data sources (Async Version)
// =====================================================================================

#ifndef _STREAMER_INC_
#define _STREAMER_INC_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <spdlog/spdlog.h>

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
    struct StreamingEOF
    {
    };

    using Host = UniqType<std::string, struct Host_Tag>;
    using Port = UniqType<std::string, struct Port_Tag>;
    using APIKey = UniqType<std::string, struct API_Key_Tag>;
    using Prefix = UniqType<std::string, struct Prefix_Tag>;

    using UTC_TmPt_NanoSecs = std::chrono::utc_time<std::chrono::nanoseconds>;
    using TopOfBookList = std::vector<TopOfBookOpenAndLastClose>;

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
        std::string ticker_;
        std::string time_stamp_;
        UTC_TmPt_NanoSecs time_stamp_nanoseconds_utc_{};
        decimal::Decimal last_price_{-1};
        int32_t last_size_{-1};
        bool dark_pool_{false};
        EodMktStatus market_status_{EodMktStatus::e_unknown};
    };

    struct StreamerContext
    {
        std::condition_variable cv_ = {};
        bool done_ = false; // Flag to signal completion
        std::mutex mtx_ = {};
        std::queue<std::string> streamed_data_;
    };

    struct ExtractorContext
    {
        std::condition_variable cv_ = {};
        bool done_ = false; // Flag to signal completion
        std::mutex mtx_ = {};
        std::queue<PF_Data> extracted_data_ = {};
    };

    // ====================  LIFECYCLE     =======================================

    RemoteDataSource();
    virtual ~RemoteDataSource();
    RemoteDataSource(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix);

    RemoteDataSource(const RemoteDataSource &rhs) = delete;
    RemoteDataSource(RemoteDataSource &&rhs) = delete;

    // ====================  ACCESSORS     =======================================

    // Synchronous request (kept for non-streaming data)
    std::string RequestData(const std::string &request_string);

    virtual TopOfBookList GetTopOfBookAndLastClose() = 0;
    virtual std::vector<StockDataRecord> GetMostRecentTickerData(const std::string &symbol,
                                                                 std::chrono::year_month_day start_from,
                                                                 int how_many_previous, UseAdjusted use_adjusted,
                                                                 const US_MarketHolidays *holidays) = 0;
    virtual PF_Data ExtractStreamedData(const std::string &buffer) = 0;

    // ====================  MUTATORS      =======================================

    // Main entry point for the async loop
    void StreamData(bool *had_signal, StreamerContext &streamer_context);

    // Derived classes implement this to send subscription messages after connection
    virtual void OnConnected() = 0;

    // Derived classes implement this to clean up subscriptions
    virtual void StopStreaming(StreamerContext &streamer_context) = 0;

    void UseSymbols(const std::vector<std::string> &symbols);

    void RequestStop();
    void ConnectWS();
    void DisconnectWS();

protected:
    // ====================  ASYNC MECHANICS  ====================================

    // Call this from derived classes when subscription handshake is done
    void StartReadLoop();

    // Handlers
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_ssl_handshake(beast::error_code ec);
    void on_handshake(beast::error_code ec);
    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_timer(beast::error_code ec);
    void on_signal(beast::error_code ec, int signal_number);

    // ====================  DATA MEMBERS  =======================================

    net::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws_;
    int version = 11;

    // Async components
    boost::asio::steady_timer timer_{ioc_};
    boost::asio::signal_set signals_{ioc_};
    beast::flat_buffer buffer_;

    // Pointers to external context (valid only during StreamData execution)
    StreamerContext *context_ptr_ = nullptr;
    bool *had_signal_ptr_ = nullptr;

    std::vector<std::string> symbol_list_;
    std::string host_;
    std::string port_;
    std::string api_key_;
    std::string websocket_prefix_;
};

// Formatter specializations...
template <>
struct std::formatter<RemoteDataSource::PF_Data> : std::formatter<std::string>
{
    auto format(const RemoteDataSource::PF_Data &pdata, std::format_context &ctx) const
    {
        std::string record;
        std::format_to(std::back_inserter(record), "ticker: {}, price: {}, shares: {}, time: {}", pdata.ticker_,
                       pdata.last_price_, pdata.last_size_, pdata.time_stamp_nanoseconds_utc_);
        return formatter<std::string>::format(record, ctx);
    }
};

inline std::ostream &operator<<(std::ostream &os, const RemoteDataSource::PF_Data pf_data)
{
    std::cout << "ticker: " << pf_data.ticker_ << " price: " << pf_data.last_price_ << " shares: " << pf_data.last_size_
              << " time:" << pf_data.time_stamp_nanoseconds_utc_;
    return os;
}

#endif
