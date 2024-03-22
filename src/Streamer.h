// =====================================================================================
//
//       Filename:  Streamer.h
//
//    Description:  Wrapper class for different streaming data sources
//
//        Version:  1.0
//        Created:  03/20/2024 01:46:48 PM
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
template <class StreamSource>
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

    // ====================  LIFECYCLE     =======================================

    ~Streamer()
    {
        // need to disconnect if still connected.

        Disconnect();
    }
    Streamer(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix)
        : api_key_{api_key.get()},
          host_{host.get()},
          port_{port.get()},
          websocket_prefix_{prefix.get()},
          ctx_{ssl::context::tlsv12_client},
          resolver_{ioc_},
          ws_{ioc_, ctx_}
    {
    }

    Streamer(const Streamer& rhs) = delete;
    Streamer(Streamer&& rhs) = delete;

    // ====================  ACCESSORS     =======================================

    // ====================  MUTATORS      =======================================

    void Connect()
    {
        // the following code is taken from example in Boost documentation

        auto const results = resolver_.resolve(host_, port_);

        auto ep = net::connect(get_lowest_layer(ws_), results);

        if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
        {
            throw beast::system_error(
                beast::error_code(static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()),
                "Failed to set SNI Hostname");
        }

        auto host = host_ + ':' + std::to_string(ep.port());

        ws_.set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

        // Perform the SSL handshake
        ws_.next_layer().handshake(ssl::stream_base::client);

        // Perform the websocket handshake
        ws_.handshake(host, websocket_prefix_);
        BOOST_ASSERT_MSG(ws_.is_open(), "Unable to complete websocket connection.");

        beast::flat_buffer buffer;

        buffer.clear();
        ws_.read(buffer);
        ws_.text(ws_.got_text());
        std::string buffer_content = beast::buffers_to_string(buffer.cdata());
        BOOST_ASSERT_MSG(buffer_content.starts_with(R"***({"status_code":200,)***"),
                         std::format("Failed to get success code. Got: {}", buffer_content).c_str());
    }

    void Disconnect()
    {
        if (!ws_.is_open())
        {
            return;
        }
        try
        {
            // beast::close_socket(get_lowest_layer(ws_));
            ws_.close(websocket::close_code::normal);
        }
        catch (std::exception& e)
        {
            spdlog::error("Problem closing socket during disconnect: {}.", e.what());
        }
    }

    void UseSymbols(const std::vector<std::string>& symbols) {}
    // void StreamData(bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data);
    // void StartStreaming();
    // void StopStreaming();

    // ====================  OPERATORS     =======================================

    Streamer& operator=(const Streamer& rhs) = delete;
    Streamer& operator=(Streamer&& rhs) = delete;

   protected:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

   private:
    friend StreamSource;

    // ====================  METHODS       =======================================

    Streamer()  // constructor
        : ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
    {
    }

    // ====================  DATA MEMBERS  =======================================

    std::string host_;
    std::string port_;
    std::string api_key_;
    std::string websocket_prefix_;

    net::io_context ioc_;
    ssl::context ctx_;
    tcp::resolver resolver_;
    websocket::stream<beast::ssl_stream<tcp::socket>, false> ws_;

    std::vector<std::string> symbol_list_;

};  // ----------  end of template class Streamer  ----------

#endif  // ----- #ifndef _STREAMER_INC_  -----
