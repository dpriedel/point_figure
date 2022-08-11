// =====================================================================================
//
//       Filename:  PointAndFigureDB.h
//
//    Description: Encapsulate code for database access 
//
//        Version:  1.0
//        Created:  08/04/2022 10:03:19 AM
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


#ifndef  _POINTANDFIGUREDB_INC_
#define  _POINTANDFIGUREDB_INC_

#include <string>
#include <string_view>
#include <vector>

#include <json/json.h>

#include <pqxx/pqxx>
#include <pqxx/stream_from>

class PF_Chart;

#include "utilities.h"

// =====================================================================================
//        Class:  PF_DB
//  Description:  Code needed to work with stock and PF_Chart data stored in DB
// =====================================================================================
class PF_DB
{
public:

    // keep our database related parms together

    struct DB_Params
    {
	    std::string user_name_;
	    std::string db_name_;
	    std::string host_name_ = "localhost";
	    std::string db_mode_ = "test";
	    std::string db_data_source_;
	    int32_t port_number_ = 5432;
    };

	// ====================  LIFECYCLE     ======================================= 
	PF_DB () = default;                             // constructor 
    PF_DB(const DB_Params& db_params);
	// ====================  ACCESSORS     ======================================= 

    std::vector<std::string> ListExchanges() const;
    std::vector<std::string> ListSymbolsOnExchange(std::string_view exchange) const;

    Json::Value GetPFChartData(const std::string file_name) const;
    void StorePFChartDataIntoDB(const PF_Chart& the_chart, const std::string& cvs_graphics_data) const;
    
    std::vector<StockDataRecord> RetrieveStockDataRecordsFromDB (const std::string& query_cmd) const;

    template<typename T>
    std::vector<T> RunSQLQueryUsingRows(const std::string& query_cmd, const auto& converter) const;

    template<typename T, typename ...Vals>
    std::vector<T> RunSQLQueryUsingStream(const std::string& query_cmd, const auto& converter) const;

	// ====================  MUTATORS      ======================================= 

	// ====================  OPERATORS     ======================================= 

protected:
	// ====================  METHODS       ======================================= 

	// ====================  DATA MEMBERS  ======================================= 

private:
	// ====================  METHODS       ======================================= 

	// ====================  DATA MEMBERS  ======================================= 

    DB_Params db_params_;

}; // -----  end of class PF_DB  ----- 

template<typename T>
std::vector<T> PF_DB::RunSQLQueryUsingRows(const std::string& query_cmd, const auto& converter) const
{
	pqxx::connection c{fmt::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
	pqxx::nontransaction trxn{c};		// we are read-only for this work

	auto results = trxn.exec(query_cmd);
	trxn.commit();

    std::vector<T> data;
    for (const auto& row: results)
    {
        T new_data = converter(row);
        data.push_back(std::move(new_data));
    }
	return data;
}

template<typename T, typename ...Vals>
std::vector<T> PF_DB::RunSQLQueryUsingStream(const std::string& query_cmd, const auto& converter) const
{
	pqxx::connection c{fmt::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
	pqxx::nontransaction trxn{c};		// we are read-only for this work

    std::vector<T> data;

    auto stream = pqxx::stream_from::query(trxn, query_cmd);
    std::tuple<Vals...> row;
    while (stream >> row)
    {
        T new_data = converter(row);
        data.push_back(std::move(new_data));
    }
    stream.complete();
	trxn.commit();

    return data;
}
#endif   // ----- #ifndef _POINTANDFIGUREDB_INC_  ----- 
