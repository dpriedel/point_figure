// =====================================================================================
//
//       Filename:  PointAndFigureDB.cpp
//
//    Description:  Encapsulate code for database access 
//
//        Version:  1.0
//        Created:  08/04/2022 10:05:18 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//   Organization:  
//
//   License:       The GPLv3 License (GPLv3)

// Copyright (c) 2022 David P. Riedel
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// =====================================================================================


#include <boost/assert.hpp>

#include <fmt/format.h>

#include <pqxx/pqxx>
#include <pqxx/stream_from.hxx>
#include <pqxx/transaction.hxx>

#include <spdlog/spdlog.h>

#include "PointAndFigureDB.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_DB
//      Method:  PF_DB
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_DB::PF_DB (const DB_Params& db_params)
    : db_params_{db_params}

{
        BOOST_ASSERT_MSG(! db_params_.host_name_.empty(), "Must provide 'db-host' to access PointAndFigure database.");
        BOOST_ASSERT_MSG(db_params_.port_number_ != -1, "Must provide 'db-port' to access PointAndFigure database.");
        BOOST_ASSERT_MSG(! db_params_.user_name_.empty(), "Must provide 'db-user' to access PointAndFigure database.");
        BOOST_ASSERT_MSG(! db_params_.db_name_.empty(), "Must provide 'db-name' to access PointAndFigure database.");
        BOOST_ASSERT_MSG(db_params_.db_mode_ == "test" || db_params_.db_mode_ == "live", "'db-mode' must be 'test' or 'live' to access PointAndFigure database.");
}  // -----  end of method PF_DB::PF_DB  (constructor)  ----- 


std::vector<std::string> PF_DB::ListExchanges () const
{
	std::vector<std::string> exchanges;

	try
	{
	    BOOST_ASSERT_MSG(! db_params_.db_data_source_.empty(), "'db-data-source' must be specified to access stock_data database.");

    	pqxx::connection c{fmt::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
		pqxx::nontransaction trxn{c};		// we are read-only for this work

		std::string get_exchanges_cmd = fmt::format("SELECT DISTINCT(exchange) FROM {} ORDER BY exchange ASC",
				db_params_.db_data_source_
				);
		for (auto [exchange] : trxn.stream<std::string_view>(get_exchanges_cmd))
		{
			exchanges.emplace_back(std::string{exchange}); 
		}
		trxn.commit();
    }
   	catch (const std::exception& e)
   	{
        spdlog::error(fmt::format("Unable to load list of exchanges from database because: {}\n", e.what()));
   	}
	return exchanges;
}		// -----  end of method PF_DB::ListExchanges  ----- 

std::vector<std::string> PF_DB::ListSymbolsOnExchange (std::string_view exchange) const
{
	std::vector<std::string> symbols;

	try
	{
	    BOOST_ASSERT_MSG(! db_params_.db_data_source_.empty(), "'db-data-source' must be specified to access stock_data database.");

    	pqxx::connection c{fmt::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
		pqxx::nontransaction trxn{c};		// we are read-only for this work

		std::string get_symbols_cmd = fmt::format("SELECT DISTINCT(symbol) FROM {} WHERE exchange = {} ORDER BY symbol ASC",
				db_params_.db_data_source_,
				trxn.quote(exchange)
				);
		for (auto [symbol] : trxn.stream<std::string_view>(get_symbols_cmd))
		{
			symbols.emplace_back(std::string{symbol}); 
		}
		trxn.commit();
    }
   	catch (const std::exception& e)
   	{
        spdlog::error(fmt::format("Unable to load list of symbols for exchange: {} because: {}\n", exchange, e.what()));
   	}
	return symbols;
}		// -----  end of method PF_DB::ListSymbolsOnExchange  ----- 

