// =====================================================================================
//
//       Filename:  LiveStream.h
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

#ifndef  LIVESTREAM_INC
#define  LIVESTREAM_INC

#include <deque>
#include <string>
#include <vector>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>

#include <sys/types.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#include "DDecDouble.h"

// =====================================================================================
//        Class:  LiveStream
//  Description:  live stream ticker updates -- look like a generator 
// =====================================================================================
#include <vector>
class LiveStream
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
    LiveStream ();                             // constructor 
    ~LiveStream ();
    LiveStream (const std::string& host, const std::string& port, const std::string& prefix,
            const std::string& api_key, const std::string& symbols);

    LiveStream(const LiveStream& rhs) = delete;
    LiveStream(LiveStream&& rhs) = delete;

    // ====================  ACCESSORS     ======================================= 

    // ====================  MUTATORS      ======================================= 

    void Connect();
    void Disconnect();
    void StreamData(bool* time_to_stop);
    void StopStreaming();

    // ====================  OPERATORS     ======================================= 

    LiveStream& operator = (const LiveStream& rhs) = delete;
    LiveStream& operator = (LiveStream&& rhs) = delete;

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
}; // -----  end of class LiveStream  ----- 

#endif   // ----- #ifndef LIVESTREAM_INC  ----- 
