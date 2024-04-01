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

#include <cctype>
#include <chrono>
#include <queue>
#include <ranges>
#include <vector>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

#include <spdlog/spdlog.h>

namespace rng = std::ranges;

// #pragma GCC diagnostic pop

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace http = beast::http;            // from <boost/beast/http.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

#include "Uniqueifier.h"
#include "utilities.h"

// =====================================================================================
//        Class:  Streamer
//  Description:  wrapper for streamed data sources
// =====================================================================================

class Streamer
{
   public:
    struct StreamingEOF
    {
    };

    using Host = UniqType<std::string, struct Host_Tag>;
    using Port = UniqType<std::string, struct Port_Tag>;
    using APIKey = UniqType<std::string, struct API_Key_Tag>;
    using Prefix = UniqType<std::string, struct Prefix_Tag>;

    using TmPt = std::chrono::utc_time<std::chrono::utc_clock::duration>;

    using TopOfBookList = std::vector<TopOfBookOpenAndLastClose>;

    // ====================  LIFECYCLE     =======================================

    Streamer();  // constructor

    virtual ~Streamer();

    Streamer(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix);

    Streamer(const Streamer& rhs) = delete;
    Streamer(Streamer&& rhs) = delete;

    // ====================  ACCESSORS     =======================================

    // ====================  MUTATORS      =======================================

    void ConnectWS();
    void DisconnectWS();
    void StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data);

    virtual void StartStreaming() = 0;
    virtual void StopStreaming() = 0;

    // for streaming or other data retrievals

    void UseSymbols(const std::vector<std::string>& symbols);

    std::string RequestData(const std::string& request_string);

    // ====================  OPERATORS     =======================================

    Streamer& operator=(const Streamer& rhs) = delete;
    Streamer& operator=(Streamer&& rhs) = delete;

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

};  // ----------  end of template class Streamer  ----------

#endif  // ----- #ifndef _STREAMER_INC_  -----
