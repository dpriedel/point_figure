// =====================================================================================
//
//       Filename:  Tiingo.h
//
//    Description:  class to live stream ticker updatas 
//
//        Version:  1.0
//        Created:  08/06/2021 09:26:57 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef  _TIINGO_INC_
#define  _TIINGO_INC_

#include <chrono>
#include <deque>
#include <format>
#include <mutex>
#include <queue>
#include <vector>
#include <sys/types.h>

#include <json/json.h>
#include <decimal.hh>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#pragma GCC diagnostic pop

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "utilities.h"

// =====================================================================================
//        Class:  Tiingo
//  Description:  live stream ticker updates -- look like a generator 
// =====================================================================================
#include <vector>
class Tiingo
{
public:

    struct PF_Data
    {
        std::string subscription_id_;
        std::string ticker_;                        // Ticker
        std::string time_stamp_;                    // Date
        int64_t time_stamp_nanoseconds_utc_;        // time_stamp
        decimal::Decimal last_price_;           // Last Price
        int32_t last_size_;                         // Last Size
    };
    
    using StreamedData = std::vector<PF_Data>;

    // ====================  LIFECYCLE     ======================================= 
    Tiingo ();                             // constructor 
    ~Tiingo ();
    Tiingo(const std::string& host, const std::string& port, const std::string& api_key);
    Tiingo (const std::string& host, const std::string& port, const std::string& prefix,
            const std::string& api_key, const std::vector<std::string>& symbols);

    Tiingo(const Tiingo& rhs) = delete;
    Tiingo(Tiingo&& rhs) = delete;

    // ====================  ACCESSORS     ======================================= 

    Json::Value GetTopOfBookAndLastClose();
    std::vector<StockDataRecord> GetMostRecentTickerData(const std::string& symbol, std::chrono::year_month_day start_from, int how_many_previous, UseAdjusted use_adjusted, const US_MarketHolidays* holidays=nullptr);

    StreamedData ExtractData(const std::string& buffer);

    // ====================  MUTATORS      ======================================= 

    void Connect();
    void Disconnect();
    void StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data);
    void StopStreaming(bool* had_signal);

    // ====================  OPERATORS     ======================================= 

    Tiingo& operator = (const Tiingo& rhs) = delete;
    Tiingo& operator = (Tiingo&& rhs) = delete;

protected:
    // ====================  METHODS       ======================================= 

    Json::Value GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date, std::chrono::year_month_day end_date, UpOrDown sort_asc);

    // ====================  DATA MEMBERS  ======================================= 

private:
    // ====================  METHODS       ======================================= 

    // ====================  DATA MEMBERS  ======================================= 

    std::vector<std::string> symbol_list_;
    std::string api_key_;
    std::string host_;
    std::string port_;
    std::string websocket_prefix_;
    std::string subscription_id_;
    int version_ = 11;

    net::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws_;
}; // -----  end of class Tiingo  ----- 

template <>
struct std::formatter<Tiingo::PF_Data> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Tiingo::PF_Data& pdata, std::format_context& ctx) const
    {
        std::string record;
        std::format_to(std::back_inserter(record), "ticker: {}, price: {}, shares: {}, time: {}", pdata.ticker_, pdata.last_price_.format("f"),
                       pdata.last_size_, pdata.time_stamp_);
        return formatter<std::string>::format(record, ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Tiingo::PF_Data pf_data)
{
    std::cout << "ticker: " << pf_data.ticker_ << " price: " << pf_data.last_price_ << " shares: " << pf_data.last_size_ << " time:" << pf_data.time_stamp_;
    return os;
}


#endif   // ----- #ifndef _TIINGO_INC_  ----- 
