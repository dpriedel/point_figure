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
/// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)/
// =====================================================================================
// the guts of this code comes from the examples distributed by Boost.

#include <string_view>

#include <json/json.h>

#include "LiveStream.h"
#include "boost/beast/core/buffers_to_string.hpp"

using namespace std::string_literals;

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
    
    if (ws_.is_open())
    {
        Disconnect();
    }
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
    auto host = host_ + ':' + std::to_string(ep.port());

    // Perform the SSL handshake
    ws_.next_layer().handshake(ssl::stream_base::client);

    // Perform the websocket handshake
    ws_.handshake(host, websocket_prefix_);


}
void LiveStream::StreamData(bool* time_to_stop)
{

    // put this here for now.
    // need to manually construct to get expected formate when serialized 

    Json::Value connection_request;
    connection_request["eventName"] = "subscribe";
    connection_request["authorization"] = api_key_;
    connection_request["eventData"]["thresholdLevel"] = 5;
    Json::Value tickers;
    for (const auto& symbol : symbol_list_)
    {
        tickers.append(symbol);
    }
    
    connection_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";        // compact printing and string formatting 
    const std::string connection_request_str = Json::writeString(builder, connection_request);
//    std::cout << "Jsoncpp connection_request_str: " << connection_request_str << '\n';

    // Send the message
    ws_.write(net::buffer(connection_request_str));

    // This buffer will hold the incoming message
    beast::flat_buffer buffer;

    while(true)
    {
        buffer.clear();
        ws_.read(buffer);
        std::string buffer_content = beast::buffers_to_string(buffer.cdata());
        // The make_printable() function helps print a ConstBufferSequence
//        std::cout << buffer_content << std::endl;
        ExtractData(buffer_content);
        if (*time_to_stop == true)
        {
            StopStreaming();
            break;
        }
    }
}

void LiveStream::ExtractData (const std::string& buffer)
{
    // will eventually need to use locks to access this I think.
    // for now, we just append data.
    JSONCPP_STRING err;
    Json::Value response;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
//    if (!reader->parse(buffer.data(), buffer.data() + buffer.size(), &response, nullptr))
    if (! reader->parse(buffer.data(), buffer.data() + buffer.size(), &response, &err))
    {
        throw std::runtime_error("Problem parsing tiingo response: "s + err);
    }
//    std::cout << "\n\n jsoncpp parsed response: " << response << "\n\n";

    auto message_type = response["messageType"];
    if (message_type == "A")
    {
        auto data = response["data"];

        if (data[0] != "T")
        {
            return;
        }
        // extract our data

        PF_Data new_value;
        new_value.subscription_id_ = subscription_id_;
        new_value.time_stamp_ = data[1].asString();
        new_value.time_stamp_seconds_ = data[2].asInt64();
        new_value.ticker_ = data[3].asString();
        new_value.last_price_ = DprDecimal::DDecDouble(data[9].asFloat(), 4);
        new_value.last_size_ = data[10].asInt();

        pf_data_.push_back(std::move(new_value));        

//        std::cout << "new data: " << pf_data_.back().ticker_ << " : " << pf_data_.back().last_price_ << '\n';
    }
    else if (message_type == "I")
    {
        subscription_id_ = response["data"]["subscriptionId"].asInt();
//        std::cout << "json cpp subscription ID: " << subscription_id_ << '\n';
        return;
    }
    else if (message_type == "H")
    {
        // heartbeat , just return

        return;
    }
    else
    {
        throw std::runtime_error("unexpected message type: "s + message_type.asString());
    }

}		// -----  end of method LiveStream::ExtractData  ----- 

void LiveStream::StopStreaming ()
{
    // we need to send the unsubscribe message in a separate connection.

    Json::Value disconnect_request;
    disconnect_request["eventName"] = "unsubscribe";
    disconnect_request["authorization"] = api_key_;
    disconnect_request["eventData"]["subscriptionId"] = subscription_id_;
    Json::Value tickers;
    for (const auto& symbol : symbol_list_)
    {
        tickers.append(symbol);
    }
    
    disconnect_request["eventData"]["tickers"] = tickers;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";        // compact printing and string formatting 
    const std::string disconnect_request_str = Json::writeString(builder, disconnect_request);
//    std::cout << "Jsoncpp disconnect_request_str: " << disconnect_request_str << '\n';

    // just grab the code from the example program 

    net::io_context ioc;
    ssl::context ctx{ssl::context::tlsv12_client};

    tcp::resolver resolver{ioc};
    websocket::stream<beast::ssl_stream<tcp::socket>> ws{ioc, ctx};

    auto host = host_;
    auto port = port_;

    auto const results = resolver.resolve(host, port);

    auto ep = net::connect(get_lowest_layer(ws), results);

    if(! SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
        throw beast::system_error(
            beast::error_code(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category()),
            "Failed to set SNI Hostname");

    host += ':' + std::to_string(ep.port());

    ws.next_layer().handshake(ssl::stream_base::client);

    ws.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req)
        {
            req.set(http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) +
                    " websocket-client-coro");
        }));

    ws.handshake(host, websocket_prefix_);

    ws.write(net::buffer(std::string(disconnect_request_str)));

    beast::flat_buffer buffer;

    ws.read(buffer);

    ws.close(websocket::close_code::normal);

//    std::cout << beast::make_printable(buffer.data()) << std::endl;
 
}		// -----  end of method LiveStream::StopStreaming  ----- 

void LiveStream::Disconnect()
{

    // Close the WebSocket connection
    ws_.close(websocket::close_code::normal);
}
