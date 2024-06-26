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

#include <date/date.h>  // for from_stream

#include <boost/assert.hpp>
#include <format>
#include <pqxx/pqxx>
#include <pqxx/stream_from.hxx>
#include <pqxx/transaction.hxx>
// #include <date/chrono_io.h>
#include <date/tz.h>
#include <spdlog/spdlog.h>

#include "PF_Chart.h"
#include "PointAndFigureDB.h"
#include "utilities.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_DB
//      Method:  PF_DB
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_DB::PF_DB(DB_Params db_params) : db_params_{std::move(db_params)}

{
    BOOST_ASSERT_MSG(!db_params_.host_name_.empty(), "Must provide 'db-host' to access PointAndFigure database.");
    BOOST_ASSERT_MSG(db_params_.port_number_ != -1, "Must provide 'db-port' to access PointAndFigure database.");
    BOOST_ASSERT_MSG(!db_params_.user_name_.empty(), "Must provide 'db-user' to access PointAndFigure database.");
    BOOST_ASSERT_MSG(!db_params_.db_name_.empty(), "Must provide 'db-name' to access PointAndFigure database.");
    BOOST_ASSERT_MSG(db_params_.PF_db_mode_ == "test" || db_params_.PF_db_mode_ == "live",
                     "'db-mode' must be 'test' or 'live' to access PointAndFigure database.");
}  // -----  end of method PF_DB::PF_DB  (constructor)  -----

std::vector<std::string> PF_DB::ListExchanges() const
{
    std::vector<std::string> exchanges;

    auto Row2Exchange = [](const auto& r) { return r[0].template as<std::string>(); };

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    std::string get_exchanges_cmd =
        std::format("SELECT DISTINCT(exchange) FROM new_stock_data.names_and_symbols ORDER BY exchange ASC",
                    db_params_.stock_db_data_source_);

    try
    {
        exchanges = RunSQLQueryUsingRows<std::string>(get_exchanges_cmd, Row2Exchange);
    }
    catch (const std::exception& e)
    {
        spdlog::error(std::format("Unable to load list of exchanges from database because: {}\n", e.what()));
    }
    return exchanges;
}  // -----  end of method PF_DB::ListExchanges  -----

std::vector<std::string> PF_DB::ListSymbolsOnExchange(std::string_view exchange,
                                                      std::string_view min_dollar_volume) const
{
    std::vector<std::string> symbols;

    auto Row2Symbol = [](const auto& r) { return std::string{std::get<0>(r)}; };

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    try
    {
        std::string get_symbols_cmd =
            std::format("SELECT * FROM new_stock_data.find_symbols_gte_min_dollar_volume({}, {})", c.quote(exchange),
                        c.quote(min_dollar_volume));
        symbols = RunSQLQueryUsingStream<std::string, std::string_view>(get_symbols_cmd, Row2Symbol);
    }
    catch (const std::exception& e)
    {
        spdlog::error(std::format("Unable to load list of symbols for exchange: {} because: {}\n", exchange, e.what()));
    }
    return symbols;
}  // -----  end of method PF_DB::ListSymbolsOnExchange  -----

Json::Value PF_DB::GetPFChartData(std::string_view file_name) const
{
    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::transaction trxn{c};

    auto retrieve_chart_data_cmd =
        std::format("SELECT chart_data FROM {}_point_and_figure.pf_charts WHERE file_name = {}", db_params_.PF_db_mode_,
                    trxn.quote(file_name));

    // it's possible we get no records so use this more general command
    auto results = trxn.exec(retrieve_chart_data_cmd);
    trxn.commit();

    if (results.empty())
    {
        return {};
    }
    auto the_data = results[0][0].as<std::string_view>();

    // TODO(dpriedel): ?? write a converter for pqxx library

    JSONCPP_STRING err;
    Json::Value chart_data;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (!reader->parse(the_data.data(), the_data.data() + the_data.size(), &chart_data, &err))
    {
        throw std::runtime_error(std::format("Problem parsing data from DB for file: {}.\n{}", file_name, err));
    }
    return chart_data;
}  // -----  end of method PF_DB::GetPFChartData  -----

std::vector<PF_Chart> PF_DB::RetrieveAllEODChartsForSymbol(std::string_view symbol) const
{
    std::vector<PF_Chart> charts;

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::transaction trxn{c};

    auto retrieve_chart_data_cmd = std::format(
        "SELECT chart_data FROM {}_point_and_figure.pf_charts WHERE symbol = {} and file_name like '%_eod.json' ",
        db_params_.PF_db_mode_, trxn.quote(symbol));

    // it's possible we get no records so use this more general command
    auto results = trxn.exec(retrieve_chart_data_cmd);
    trxn.commit();

    if (results.empty())
    {
        return {};
    }

    for (const auto& row : results)
    {
        auto the_data = row[0].as<std::string_view>();

        JSONCPP_STRING err;
        Json::Value chart_data;

        Json::CharReaderBuilder builder;
        const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        if (!reader->parse(the_data.data(), the_data.data() + the_data.size(), &chart_data, &err))
        {
            throw std::runtime_error(std::format("Problem parsing data from DB for symbol: {}.\n{}", symbol, err));
        }
        PF_Chart retrieved_chart{chart_data};
        charts.push_back(std::move(retrieved_chart));
    }
    return charts;
}  // -----  end of method PF_DB::RetrieveAllEODChartsForSymbol  -----

void PF_DB::StorePFChartDataIntoDB(const PF_Chart& the_chart, std::string_view interval,
                                   std::string_view cvs_graphics_data) const
{
    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::work trxn{c};

    auto delete_existing_data_cmd =
        std::format("DELETE FROM {}_point_and_figure.pf_charts WHERE file_name = {}", db_params_.PF_db_mode_,
                    trxn.quote(the_chart.MakeChartFileName(interval, "json")));
    trxn.exec(delete_existing_data_cmd);

    auto json = the_chart.ToJSON();
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    std::string for_db = Json::writeString(wbuilder, json);

    // std::string direction_fld_name = db_params_.PF_db_mode_ + '_' + "current_direction";
    // std::string signal_fld_name = db_params_.PF_db_mode_ + '_' + "current_signal";

    const auto add_new_data_cmd = std::format(
        "INSERT INTO {}_point_and_figure.pf_charts ({}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {})"
        " VALUES({}, {}, {}, {}, 'e_{}', 'e_{}', {}, {}, {}, {}, 'e_{}', 'e_{}', '{}', '{}')",
        db_params_.PF_db_mode_, "symbol", "fname_box_size", "chart_box_size", "reversal_boxes", "box_type", "box_scale",
        "file_name", "first_date", "last_change_date", "last_checked_date", "current_direction", "current_signal",
        "chart_data", "cvs_graphics_data", trxn.quote(the_chart.GetSymbol()),
        trxn.quote(the_chart.GetFNameBoxSize().format("f")), trxn.quote(the_chart.GetChartBoxSize().format("f")),
        the_chart.GetReversalboxes(), json["boxes"]["box_type"].asString(), json["boxes"]["box_scale"].asString(),
        trxn.quote(the_chart.MakeChartFileName(interval, "json")),
        trxn.quote(std::format("{:%F %T%z}", the_chart.GetFirstTime())),
        trxn.quote(std::format("{:%F %T%z}", the_chart.GetLastChangeTime())),
        trxn.quote(std::format("{:%F %T%z}", the_chart.GetLastCheckedTime())), json["current_direction"].asString(),
        the_chart.GetCurrentSignal().value_or(PF_Signal{}).signal_type_, for_db, cvs_graphics_data);

    // std::cout << add_new_data_cmd << std::endl;
    trxn.exec(add_new_data_cmd);

    trxn.commit();
}  // -----  end of method PF_DB::StorePFChartDataIntoDB  -----

void PF_DB::UpdatePFChartDataInDB(const PF_Chart& the_chart, std::string_view interval,
                                  std::string_view cvs_graphics_data) const
{
    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::work trxn{c};

    auto json = the_chart.ToJSON();
    Json::StreamWriterBuilder wbuilder;
    wbuilder["indentation"] = "";
    std::string for_db = Json::writeString(wbuilder, json);

    const auto update_chart_data_cmd = std::format(
        "UPDATE {}_point_and_figure.pf_charts "
        "SET chart_data = '{}', cvs_graphics_data = '{}', last_change_date = {}, last_checked_date = {}, "
        "current_direction = 'e_{}', current_signal = 'e_{}' "
        "WHERE symbol = {} and file_name = {}",
        db_params_.PF_db_mode_, for_db, cvs_graphics_data,
        trxn.quote(std::format("{:%F %T%z}", the_chart.GetLastChangeTime())),
        trxn.quote(std::format("{:%F %T%z}", the_chart.GetLastCheckedTime())), json["current_direction"].asString(),
        the_chart.GetCurrentSignal().value_or(PF_Signal{}).signal_type_, trxn.quote(the_chart.GetSymbol()),
        trxn.quote(the_chart.MakeChartFileName(interval, "json")));

    //    std::cout << update_chart_data_cmd << std::endl;
    trxn.exec(update_chart_data_cmd);

    trxn.commit();
}  // -----  end of method PF_DB::UpdatePFChartDataInDB  -----

void PF_DB::UpdateLastCheckedDateInChartsDB(std::string_view exchange, std::string_view last_checked_date) const
{
    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::work trxn{c};

    const auto update_last_checked_date_stmt = std::format(
        "UPDATE {}_point_and_figure.pf_charts AS t1 SET last_checked_date = {} FROM new_stock_data.names_and_symbols "
        "AS t2 WHERE t1.symbol = t2.symbol AND t2.exchange = {}",
        db_params_.PF_db_mode_, trxn.quote(last_checked_date), trxn.quote(exchange));

    trxn.exec(update_last_checked_date_stmt);
    trxn.commit();

}  // -----  end of method PF_Chart::UpdateLastCheckedDateInChartsDB  -----

// ===  FUNCTION  ======================================================================
//         Name:  RetrieveMostRecentStockDataRecordsFromDB
//  Description:  just run the supplied query and convert the results set in our format.
//  We expect to get back the following fields:
//      date, exchange, symbol, open, high, low, close
// =====================================================================================

std::vector<StockDataRecord> PF_DB::RetrieveMostRecentStockDataRecordsFromDB(std::string_view symbol,
                                                                             std::string_view begin_date,
                                                                             int32_t how_many) const
{
    auto Row2StockDataRecord = [](const auto& r)
    {
        return StockDataRecord{.date_ = std::string{r[0].template as<std::string_view>()},
                               .symbol_ = std::string{r[1].template as<std::string_view>()},
                               .open_ = decimal::Decimal{r[2].c_str()},
                               .high_ = decimal::Decimal{r[3].c_str()},
                               .low_ = decimal::Decimal{r[4].c_str()},
                               .close_ = decimal::Decimal{r[5].c_str()}};
    };

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    std::string get_records_cmd = std::format(
        "SELECT date, symbol, split_adj_open, split_adj_high, split_adj_low, split_adj_close FROM {} WHERE symbol = {} "
        "AND date <= {} ORDER BY date DESC LIMIT {}",
        db_params_.stock_db_data_source_, c.quote(symbol), c.quote(begin_date),
        how_many  // need an extra row for the algorithm
    );
    std::vector<StockDataRecord> records;
    try
    {
        BOOST_ASSERT_MSG(!db_params_.stock_db_data_source_.empty(),
                         "'db-data-source' must be specified to access stock_data database.");

        records = RunSQLQueryUsingRows<StockDataRecord>(get_records_cmd, Row2StockDataRecord);
    }
    catch (const std::exception& e)
    {
        spdlog::error(std::format("Unable to run query: {}\n\tbecause: {}\n", get_records_cmd, e.what()));
    }
    return records;
}  // -----  end of function PF_DB::RetrieveMostRecentStockDataRecordsFromDB   -----

std::vector<MultiSymbolDateCloseRecord> PF_DB::GetPriceDataForSymbolsInList(const std::vector<std::string>& symbol_list,
                                                                            std::string_view begin_date,
                                                                            std::string_view end_date,
                                                                            std::string_view price_fld_name,
                                                                            const char* date_format) const
{
    // we need to convert our list of symbols into a format that can be used in a SQL query.

    std::string query_list = "( '";
    auto syms = symbol_list.begin();
    query_list += *syms;
    for (++syms; syms != symbol_list.end(); ++syms)
    {
        query_list += "', '";
        query_list += *syms;
    }
    query_list += "' )";
    spdlog::debug(std::format("Retrieving closing prices for symbols in list: {}", query_list));

    PF_DB pf_db{db_params_};

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    // we need a place to keep the data we retrieve from the database.

    std::vector<MultiSymbolDateCloseRecord> db_data;

    std::istringstream time_stream;
    date::utc_time<std::chrono::utc_clock::duration> tp;

    // we know our database contains 'date's, but we need timepoints.
    // we'll handle that in the conversion routine below.

    auto Row2Closing = [date_format, &time_stream, &tp](const auto& r)
    {
        time_stream.clear();
        time_stream.str(std::string{std::get<1>(r)});
        date::from_stream(time_stream, date_format, tp);
        std::chrono::utc_time<std::chrono::utc_clock::duration> tp1{tp.time_since_epoch()};
        MultiSymbolDateCloseRecord new_data{
            .symbol_ = std::string{std::get<0>(r)}, .date_ = tp1, .close_ = decimal::Decimal{std::get<2>(r)}};
        return new_data;
    };

    try
    {
        // first, get ready to retrieve our data from DB.  Do this for all our symbols here.

        std::string date_range = end_date.empty()
                                     ? std::format("date >= {}", c.quote(begin_date))
                                     : std::format("date BETWEEN {} and {}", c.quote(begin_date), c.quote(end_date));

        std::string get_symbol_prices_cmd =
            std::format("SELECT symbol, date, {} FROM {} WHERE symbol in {} AND {} ORDER BY symbol, date ASC",
                        price_fld_name, db_params_.stock_db_data_source_, query_list, date_range);

        db_data =
            pf_db.RunSQLQueryUsingStream<MultiSymbolDateCloseRecord, std::string_view, std::string_view, const char*>(
                get_symbol_prices_cmd, Row2Closing);
        spdlog::debug(
            std::format("Done retrieving data for symbols in: {}. Got: {} rows.", query_list, db_data.size()));
    }
    catch (const std::exception& e)
    {
        spdlog::error(std::format("Unable to retrieve DB data from symbols in: {} because: {}.", query_list, e.what()));
    }

    return db_data;
}  // -----  end of method PF_DB::GetPriceDataForSymbolsInList  -----

std::vector<MultiSymbolDateCloseRecord> PF_DB::GetPriceDataForSymbolsOnExchange(
    std::string_view exchange, std::string_view begin_date, std::string_view end_date, std::string_view price_fld_name,
    const char* date_format, std::string_view min_dollar_volume) const
{
    PF_DB pf_db{db_params_};

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    // we need a place to keep the data we retrieve from the database.

    std::vector<MultiSymbolDateCloseRecord> db_data;

    std::istringstream time_stream;
    date::utc_time<std::chrono::utc_clock::duration> tp;

    // we know our database contains 'date's, but we need timepoints.
    // we'll handle that in the conversion routine below.

    auto Row2Closing = [date_format, &time_stream, &tp](const auto& r)
    {
        time_stream.clear();
        time_stream.str(std::string{std::get<1>(r)});
        date::from_stream(time_stream, date_format, tp);
        std::chrono::utc_time<std::chrono::utc_clock::duration> tp1{tp.time_since_epoch()};
        MultiSymbolDateCloseRecord new_data{
            .symbol_ = std::string{std::get<0>(r)}, .date_ = tp1, .close_ = decimal::Decimal{std::get<2>(r)}};
        return new_data;
    };

    try
    {
        std::string date_range = end_date.empty()
                                     ? std::format("date >= {}", c.quote(begin_date))
                                     : std::format("date BETWEEN {} and {}", c.quote(begin_date), c.quote(end_date));

        // first, get ready to retrieve our data from DB.  Do this for all our symbols here.
        //
        std::string get_symbol_prices_cmd = std::format(
            "SELECT symbol, date, {} FROM {} WHERE {} AND symbol IN (SELECT * FROM "
            "new_stock_data.find_symbols_gte_min_dollar_volume({}, {})) ORDER BY symbol ASC, date ASC",
            price_fld_name, db_params_.stock_db_data_source_, date_range, c.quote(exchange),
            c.quote(min_dollar_volume));

        db_data =
            pf_db.RunSQLQueryUsingStream<MultiSymbolDateCloseRecord, std::string_view, std::string_view, const char*>(
                get_symbol_prices_cmd, Row2Closing);
        spdlog::debug(
            std::format("Done retrieving data for symbols on exchange: {}. Got: {} rows.", exchange, db_data.size()));
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            std::format("Unable to retrieve DB data from symbols on exchange: {} because: {}.", exchange, e.what()));
    }

    return db_data;
}  // -----  end of method PF_DB::GetPriceDataForSymbolsInList  -----

decimal::Decimal PF_DB::ComputePriceRangeForSymbolFromDB(std::string_view symbol, std::string_view begin_date,
                                                         std::string_view end_date) const
{
    // BUT, I expect the DB will only have data for trading days, so it will
    // automatically skip weekends for me.

    // set up a DB connection so query arguments can be properly quoted.
    PF_DB the_db{db_params_};
    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    std::string get_price_range_cmd = std::format(
        "SELECT (MAX(split_adj_close) - MIN(split_adj_close)) AS range FROM {} "
        "WHERE date BETWEEN {} AND {} AND symbol = {}",
        db_params_.stock_db_data_source_, c.quote(begin_date), c.quote(end_date), c.quote(symbol));

    c.close();

    decimal::Decimal price_range;

    auto Row2Range = [](const auto& r) { return decimal::Decimal{r[0].template as<const char*>()}; };

    try
    {
        price_range = the_db.RunSQLQueryUsingRows<decimal::Decimal>(get_price_range_cmd, Row2Range)[0];
        spdlog::debug(std::format("Price range query: {}. Result: {}\n", get_price_range_cmd, price_range.format("f")));
    }
    catch (const std::exception& e)
    {
        spdlog::error(
            std::format("Unable to compute closing price range from DB "
                        "for: '{}' because: {}.\n",
                        symbol, e.what()));
    }

    return price_range;
}  // -----  end of method PF_DB::ComputeRangeForChartFromDB -----
