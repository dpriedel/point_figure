// =====================================================================================
//
//       Filename:  LiveStream.cpp
//
//    Description:  Live stream ticker updates 
//
//        Version:  1.0
//        Created:  08/06/2021 09:28:55 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#include <string_view>

#include "LiveStream.h"


//  let's do a little 'template normal' programming again

// function to split a string on a delimiter and return a vector of items.
// use concepts to restrict to strings and string_views.

template<typename T>
inline std::vector<T> split_string(std::string_view string_data, char delim)
    requires std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>
{
    std::vector<T> results;
	for (auto it = 0; it != T::npos; ++it)
	{
		auto pos = string_data.find(delim, it);
        if (pos != T::npos)
        {
    		results.emplace_back(string_data.substr(it, pos - it));
        }
        else
        {
    		results.emplace_back(string_data.substr(it));
            break;
        }
		it = pos;
	}
    return results;
}


//--------------------------------------------------------------------------------------
//       Class:  LiveStream
//      Method:  LiveStream
// Description:  constructor
//--------------------------------------------------------------------------------------

LiveStream::LiveStream ()
    : ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
{
}  // -----  end of method LiveStream::LiveStream  (constructor)  ----- 

LiveStream::~LiveStream ()
{
    // need to disconnect if still connected.
}		// -----  end of method LiveStream::~LiveStream  ----- 

LiveStream::LiveStream (const std::string& host, const std::string& port, const std::string& prefix,
            const std::string& api_key, const std::string& symbols)
    : api_key_{api_key}, host_{host}, port_{port}, websocket_prefix_{prefix},
        ctx_{ssl::context::tlsv12_client}, resolver_{ioc_}, ws_{ioc_, ctx_}
{
    // symbols is a string of one or more symbols to monitor.
    // If more than 1 symbol, list is coma delimited.

    symbol_list_ = split_string<std::string>(symbols, ',');

}  // -----  end of method LiveStream::LiveStream  (constructor)  ----- 

void LiveStream::Connect()
{
    // Look up the domain name
    auto const results = resolver_.resolve(host_, port_);

    // Make the connection on the IP address we get from a lookup
    auto ep = net::connect(get_lowest_layer(ws_), results);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(! SSL_set_tlsext_host_name(ws_.next_layer().native_handle(), host_.c_str()))
        throw beast::system_error(
            beast::error_code(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()),
            "Failed to set SNI Hostname");

    // Update the host_ string. This will provide the value of the
    // Host HTTP header during the WebSocket handshake.
    // See https://tools.ietf.org/html/rfc7230#section-5.4
    host_ += ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    ws_.next_layer().handshake(ssl::stream_base::client);


}
void LiveStream::StreamData(bool* time_to_stop)
{

    // put this here for now.
    // need to manually construct to get expected formate when serialized 
    boost::json::array tickers = {"spy", "uso"};
    for (const auto& symbol : symbol_list_)
    {
        tickers.push_back(boost::json::value(symbol));
    }
    boost::json::object event_data;
    event_data["thresholdLevel"] = 5;
    event_data["tickers"] = tickers;

    boost::json::object obj;
    obj["eventName"] = "subscribe";
    obj["authorization"] = api_key_;
    obj["eventData"] = event_data;

    // std::cout << "obj: " << boost::json::serialize(obj) << '\n';

    std::string const  text = boost::json::serialize(obj);

    // Perform the websocket handshake
    ws_.handshake(host_, websocket_prefix_);

    // Send the message
    ws_.write(net::buffer(text));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;
    
    while(! *time_to_stop)
    {
        // Read a message into our buffer
        ws_.read(buffer);
        // The make_printable() function helps print a ConstBufferSequence
        std::cout << beast::make_printable(buffer.data()) << std::endl;
    }
}

void LiveStream::ExtractData (const beast::flat_buffer& buffer)
{
    // will eventually need to use locks to access this I think.
    // for now, we just append data.



}		// -----  end of method LiveStream::ExtractData  ----- 

void LiveStream::Disconnect()
{

    // Close the WebSocket connection
    ws_.close(websocket::close_code::normal);
    // catch(std::exception const& e)
    // {
    //     std::cerr << "Error: " << e.what() << std::endl;
    // }
}
