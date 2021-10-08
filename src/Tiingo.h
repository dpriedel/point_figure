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

#include <date/date.h>
#include <deque>
#include <string>
#include <vector>
#include <sys/types.h>

#include <date/julian.h>
#include <json/json.h>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "DDecDouble.h"

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
        int64_t time_stamp_seconds_;                // Nanoseconds
        DprDecimal::DDecDouble last_price_;         // Last Price
        int32_t last_size_;                         // Last Size
    };

    // ====================  LIFECYCLE     ======================================= 
    Tiingo ();                             // constructor 
    ~Tiingo ();
    Tiingo (const std::string& host, const std::string& port, const std::string& prefix,
            const std::string& api_key, const std::string& symbols);

    Tiingo(const Tiingo& rhs) = delete;
    Tiingo(Tiingo&& rhs) = delete;

    // ====================  ACCESSORS     ======================================= 

    [[nodiscard]] std::deque<PF_Data>::const_iterator begin() const { return pf_data_.begin(); }
    [[nodiscard]] std::deque<PF_Data>::const_iterator end() const { return pf_data_.end(); }

    [[nodiscard]] bool empty() const { return pf_data_.empty(); }

    Json::Value GetTickerData(std::string_view symbol, date::year_month_day start_date, date::year_month_day end_date, bool sort_asc);
    Json::Value GetMostRecentTickerData(std::string_view symbol, date::year_month_day start_from, int how_many_previous);

    // ====================  MUTATORS      ======================================= 

    void Connect();
    void Disconnect();
    void StreamData(bool* time_to_stop);
    void StopStreaming();

    // ====================  OPERATORS     ======================================= 

    Tiingo& operator = (const Tiingo& rhs) = delete;
    Tiingo& operator = (Tiingo&& rhs) = delete;

protected:
    // ====================  METHODS       ======================================= 

    // ====================  DATA MEMBERS  ======================================= 

private:
    // ====================  METHODS       ======================================= 

    void ExtractData(const std::string& buffer);

    // ====================  DATA MEMBERS  ======================================= 

    std::deque<PF_Data> pf_data_;
    std::vector<std::string> symbol_list_;
    std::string api_key_;
    std::string host_;
    std::string port_;
    std::string websocket_prefix_;
    int32_t subscription_id_;

    net::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>> ws_;
}; // -----  end of class Tiingo  ----- 

inline std::ostream& operator<<(std::ostream& os, const Tiingo::PF_Data pf_data)
{
    std::cout << "ticker: " << pf_data.ticker_ << " price: " << pf_data.last_price_ << " shares: " << pf_data.last_size_ << " time:" << pf_data.time_stamp_;
    return os;
}


#endif   // ----- #ifndef _TIINGO_INC_  ----- 
