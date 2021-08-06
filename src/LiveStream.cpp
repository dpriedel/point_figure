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
LiveStream::LiveStream (const std::string& host, const std::string& port, const std::string& prefix,
            const std::string& symbols)
    : host_{host}, port_{port}, websocket_prefix_{prefix}
{
    // symbols is a string of one or more symbols to monitor.
    // If more than 1 symbol, list is coma delimited.

    symbol_list_ = split_string<std::string>(symbols, ',');

}  // -----  end of method LiveStream::LiveStream  (constructor)  ----- 

