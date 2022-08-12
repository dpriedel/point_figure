// =====================================================================================
// 
//       Filename:  PF_CollectDataApp.cpp
// 
//    Description:  This program will compute Point & Figure data for the
//    				given input file.
// 
//        Version:  2.0
//        Created:  2021-07-20 11:50 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company: 
// 
// =====================================================================================


	/* This file is part of PF_CollectData. */

	/* PF_CollectData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* PF_CollectData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with PF_CollectData.  If not, see <http://www.gnu.org/licenses/>. */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <mutex>
#include <queue>
#include <sstream>

#include <map>
#include <string_view>
#include <thread>
#include <type_traits>

#include <fmt/ranges.h>

#include <range/v3/action/sort.hpp>
#include <range/v3/action/unique.hpp>
#include <range/v3/action/push_back.hpp>
#include <range/v3/algorithm/copy.hpp>
#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/cartesian_product.hpp>
#include <range/v3/view/chunk_by.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/for_each.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

#include <date/date.h>
#include <date/chrono_io.h>
#include <date/tz.h>

#include <pybind11/embed.h>
#include <pybind11/gil.h>

namespace py = pybind11;
using namespace py::literals;

#include "DDecQuad.h"
#include "PF_Chart.h"
#include "PF_CollectDataApp.h"
#include "PF_Column.h"
#include "PointAndFigureDB.h"
#include "Tiingo.h"
#include "utilities.h"

using namespace std::string_literals;
using namespace date::literals;
using namespace std::literals::chrono_literals;

bool PF_CollectDataApp::had_signal_ = false;

//--------------------------------------------------------------------------------------
//       Class:  PF_CollectDataApp
//      Method:  PF_CollectDataApp
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_CollectDataApp::PF_CollectDataApp (int argc, char* argv[])
    : argc_{argc}, argv_{argv}

{
}  // -----  end of method PF_CollectDataApp::PF_CollectDataApp  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_CollectDataApp
//      Method:  PF_CollectDataApp
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_CollectDataApp::PF_CollectDataApp (const std::vector<std::string>& tokens)
    : tokens_{tokens}
{
}  // -----  end of method PF_CollectDataApp::PF_CollectDataApp  (constructor)  -----


void PF_CollectDataApp::ConfigureLogging ()
{

    // we need to set log level if specified and also log file.

    if (! log_file_path_name_.empty())
    {
        // if we are running inside our test harness, logging may already by
        // running so we don't want to clobber it.
        // different tests may use different names.

        auto logger_name = log_file_path_name_.filename();
        logger_ = spdlog::get(logger_name);
        if (! logger_)
        {
            fs::path log_dir = log_file_path_name_.parent_path();
            if (! fs::exists(log_dir))
            {
                fs::create_directories(log_dir);
            }

            logger_ = spdlog::basic_logger_mt<spdlog::async_factory>(logger_name, log_file_path_name_.c_str());
            spdlog::set_default_logger(logger_);
        }
    }

    // we are running before 'CheckArgs' so we need to do a little editiing ourselves.

    const std::map<std::string, spdlog::level::level_enum> levels
    {
        {"none", spdlog::level::off},
        {"error", spdlog::level::err},
        {"information", spdlog::level::info},
        {"debug", spdlog::level::debug}
    };

    auto which_level = levels.find(logging_level_);
    if (which_level != levels.end())
    {
        spdlog::set_level(which_level->second);
    }

}		// -----  end of method PF_CollectDataApp::ConfigureLogging  ----- 

bool PF_CollectDataApp::Startup ()
{
    spdlog::info(fmt::format("\n\n*** Begin run {:%a, %b %d, %Y at %I:%M:%S %p %Z}  ***\n", std::chrono::system_clock::now()));
    bool result{true};
	try
	{	
		SetupProgramOptions();
        ParseProgramOptions(tokens_);
        ConfigureLogging();
		result = CheckArgs ();
	}
	catch(const std::exception& e)
	{
        spdlog::error(fmt::format("Problem in startup: {}\n", e.what()));
		//	we're outta here!

		this->Shutdown();
        result = false;
    }
	catch(...)
	{
        spdlog::error("Unexpectd problem during Starup processing\n");

		//	we're outta here!

		this->Shutdown();
        result = false;
	}
    return result;
}		// -----  end of method PF_CollectDataApp::Do_StartUp  -----


bool PF_CollectDataApp::CheckArgs ()
{
	//	let's get our input and output set up
	
	// we now have two possible sources for symbols. We need to be sure we have 1 of them.
	
	BOOST_ASSERT_MSG(! symbol_list_.empty() || ! symbol_list_i_.empty(), "Must provide either 1 or more '-s' values or 'symbol-list' list.");

	if (! symbol_list_i_.empty() && symbol_list_i_ != "*")
	{
		ranges::actions::push_back(symbol_list_, split_string<std::string>(symbol_list_i_, ','));
		symbol_list_ |= ranges::actions::sort | ranges::actions::unique;
	}

	// if symbol-list is '*' then we will generate a list of symbols from our source database.
	
	if (symbol_list_i_ == "*")
	{
		symbol_list_.clear();
	}
	else
	{
    	// now we want upper case symbols.

    	ranges::for_each(symbol_list_, [](auto& symbol) { ranges::for_each(symbol, [](char& c) { c = std::toupper(c); }); });
	}

    // now make sure we can find our data for input and output.

    BOOST_ASSERT_MSG(new_data_source_i == "file" || new_data_source_i == "streaming" || new_data_source_i == "database", fmt::format("New data source must be: 'file', 'streaming' or 'database': {}", new_data_source_i).c_str());
    new_data_source_ = new_data_source_i == "file" ? Source::e_file : new_data_source_i == "database" ? Source::e_DB : Source::e_streaming;
    
    BOOST_ASSERT_MSG(chart_data_source_i == "file" || chart_data_source_i == "database", fmt::format("Existing chart data source must be: 'file' or 'database': {}", chart_data_source_i).c_str());
    chart_data_source_ = chart_data_source_i == "file" ? Source::e_file : Source::e_DB;
    
    BOOST_ASSERT_MSG(destination_i == "file" || destination_i == "database", fmt::format("Data destination must be: 'file' or 'database': {}", destination_i).c_str());
    destination_ = destination_i == "file" ? Destination::e_file : Destination::e_DB;

    BOOST_ASSERT_MSG(mode_i == "load" || mode_i == "update" || mode_i == "daily-scan", fmt::format("Mode must be: 'load', 'update' or 'daily-scan': {}", mode_i).c_str());
    mode_ = mode_i == "load" ? Mode::e_load : mode_i == "update" ? Mode::e_update : Mode::e_daily_scan;

    // possibly empty if this is our first time or we are starting over

    if (new_data_source_ == Source::e_file)
    {
        // for file input, we always need to have our raw data inputs 

        BOOST_ASSERT_MSG(! new_data_input_directory_.empty(), "Must specify 'new-data-dir' when data source is 'file'.");
        BOOST_ASSERT_MSG(fs::exists(new_data_input_directory_), fmt::format("Can't find new data input directory: {}", new_data_input_directory_).c_str());

        BOOST_ASSERT_MSG(source_format_i == "csv" || source_format_i == "json", fmt::format("New data files must be: 'csv' or 'json': {}", source_format_i).c_str());
        source_format_ = source_format_i == "csv" ? SourceFormat::e_csv : SourceFormat::e_json;

        // if we are adding to existing data then we need to know where to find that data  

        if (mode_ == Mode::e_update && chart_data_source_ == Source::e_file)
        {
        	BOOST_ASSERT_MSG(! input_chart_directory_.empty(), "Must specify 'chart-data-dir' when data source is 'file' and mode is 'update'.");
        	BOOST_ASSERT_MSG(fs::exists(input_chart_directory_), fmt::format("Can't find new existing chart data directory: {}", input_chart_directory_).c_str());

    		// we could write out data to a separate location if we want 
    		// otherwise, use the charts directory. 

        	if (output_chart_directory_.empty())
        	{
        		output_chart_directory_ = input_chart_directory_;
        	}
        }
    }

    BOOST_ASSERT_MSG(graphics_format_i_ == "svg" || graphics_format_i_ == "csv", fmt::format("graphics-format must be either 'svg' or 'csv': {}", graphics_format_i_).c_str());
    graphics_format_ = graphics_format_i_ == "svg" ? GraphicsFormat::e_svg : GraphicsFormat::e_csv;
    
    if (destination_ == Destination::e_file)
    {
        BOOST_ASSERT_MSG(! output_chart_directory_.empty(), "Must specify 'output-chart-dir' when data destination is 'file'.");
    	if (! fs::exists(output_chart_directory_))
    	{
        	fs::create_directories(output_chart_directory_);
    	}

    	// we can share the charts directory if we must 

    	if (output_graphs_directory_.empty())
    	{
        	output_graphs_directory_ = output_chart_directory_;
    	}
	}

    if (destination_ == Destination::e_file || graphics_format_ == GraphicsFormat::e_svg)
    {
    	BOOST_ASSERT_MSG(! output_graphs_directory_.empty(), "Must specify 'output-graph-dir'.");
    	if (! fs::exists(output_graphs_directory_))
    	{
        	fs::create_directories(output_graphs_directory_);
    	}
    }

    if (new_data_source_ == Source::e_DB || destination_ == Destination::e_DB)
    {
        BOOST_ASSERT_MSG(! db_params_.host_name_.empty(), "Must provide 'db-host' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(db_params_.port_number_ != -1, "Must provide 'db-port' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(! db_params_.user_name_.empty(), "Must provide 'db-user' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(! db_params_.db_name_.empty(), "Must provide 'db-name' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(db_params_.db_mode_ == "test" || db_params_.db_mode_ == "live", "'db-mode' must be 'test' or 'live'.");
        if (new_data_source_ == Source::e_DB)
        {
			BOOST_ASSERT_MSG(! db_params_.db_data_source_.empty(), "'db-data-source' must be specified when load source is 'database'.");
		}
    }

    if (new_data_source_ != Source::e_DB && use_ATR_)
    {
        BOOST_ASSERT_MSG(! tiingo_api_key_.empty(), "Must specify api 'key' file when data source is 'streaming'.");
        BOOST_ASSERT_MSG(fs::exists(tiingo_api_key_), fmt::format("Can't find tiingo api key file: {}", tiingo_api_key_).c_str());
    }
    
    if (new_data_source_ == Source::e_DB)
    {
        BOOST_ASSERT_MSG(! begin_date_.empty(), "Must specify 'begin-date' when data source is 'database'.");
    }

    if (! exchange_.empty())
    {
    	BOOST_ASSERT_MSG(exchange_ == "AMEX" || exchange_ == "NYSE" || exchange_ == "NASDAQ", fmt::format("exchange: {} must be 'AMEX' or 'NYSE' or 'NASDAQ'.", exchange_).c_str());
    }
    
    BOOST_ASSERT_MSG(max_columns_for_graph_ >= -1, "max-graphic-cols must be >= -1.");

    BOOST_ASSERT_MSG(trend_lines_ == "no" || trend_lines_ == "data" || trend_lines_ == "angle", fmt::format("show-trend-lines must be: 'no' or 'data' or 'angle': {}", trend_lines_).c_str());

    const std::map<std::string, Interval> possible_intervals = {{"eod", Interval::e_eod}, {"live", Interval::e_live}, {"sec1", Interval::e_sec1}, {"sec5", Interval::e_sec5}, {"min1", Interval::e_min1}, {"min5", Interval::e_min5}};
    BOOST_ASSERT_MSG(possible_intervals.contains(interval_i), fmt::format("Interval must be: 'eod', 'live', 'sec1', 'sec5', 'min1', 'min5': {}", interval_i).c_str());
    interval_ = possible_intervals.find(interval_i)->second;
  
    // provide our default value here.

    if (scale_i_list_.empty())
    {
        scale_i_list_.emplace_back("linear");
    }

    // edit and translate from text to enums...

    ranges::for_each(scale_i_list_, [](const auto& scale) { BOOST_ASSERT_MSG(scale == "linear" || scale == "percent", fmt::format("Chart scale must be: 'linear' or 'percent': {}", scale).c_str()); });
    ranges::for_each(scale_i_list_, [this] (const auto& scale_i) { this->scale_list_.emplace_back(scale_i == "linear" ? Boxes::BoxScale::e_linear : Boxes::BoxScale::e_percent); });

    // we can compute whether boxes are fractions or intergers from input. This may be changed by the Boxes code later. 

    // generate PF_Chart type combinations from input params.

    auto params = ranges::views::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);
    ranges::for_each(params, [](const auto& x) {fmt::print("{}\n", x); });

	return true ;
}		// -----  end of method PF_CollectDataApp::Do_CheckArgs  -----

void PF_CollectDataApp::SetupProgramOptions ()
{
    newoptions_ = std::make_unique<po::options_description>();

	newoptions_->add_options()
		("help,h",											"produce help message")
		("symbol,s",			po::value<std::vector<std::string>>(&this->symbol_list_),	"name of symbol we are processing data for. Repeat for multiple symbols.")
		("symbol-list",			po::value<std::string>(&this->symbol_list_i_),	"Comma-delimited list of symbols to process OR '*' to use all symbols from the database.")
		("new-data-dir",		po::value<fs::path>(&this->new_data_input_directory_),	"name of directory containing files with new data for symbols we are using.")
		("chart-data-dir",		po::value<fs::path>(&this->input_chart_directory_),	"name of directory containing existing files with data for symbols we are using.")
		("destination",	    	po::value<std::string>(&this->destination_i)->default_value("file"),	"destination: send data to 'file' or 'database'. Default is 'file'.")
		("new-data-source",		po::value<std::string>(&this->new_data_source_i)->default_value("file"),	"source for new data: either 'file', 'streaming' or 'database'. Default is 'file'.")
		("chart-data-source",	po::value<std::string>(&this->chart_data_source_i)->default_value("file"),	"source for existing chart data: either 'file' or 'database'. Default is 'file'.")
		("source-format",		po::value<std::string>(&this->source_format_i)->default_value("csv"),	"source data format: either 'csv' or 'json'. Default is 'csv'.")
		("graphics-format",		po::value<std::string>(&this->graphics_format_i_)->default_value("svg"),	"Output graphics file format: either 'svg' or 'csv'. Default is 'svg'.")
		("mode,m",				po::value<std::string>(&this->mode_i)->default_value("load"),	"mode: either 'load' new data, 'update' existing data or 'daily-scan'. Default is 'load'.")
		("interval,i",			po::value<std::string>(&this->interval_i)->default_value("eod"),	"interval: 'eod', 'live', '1sec', '5sec', '1min', '5min'. Default is 'eod'.")
		("scale",				po::value<std::vector<std::string>>(&this->scale_i_list_),	"scale: 'linear', 'percent'. Default is 'linear'.")
		("price-fld-name",		po::value<std::string>(&this->price_fld_name_)->default_value("Close"),	"price-fld-name: which data field to use for price value. Default is 'Close'.")
		("exchange",			po::value<std::string>(&this->exchange_),	"exchange: use symbols from specified exchange. Possible values: 'AMEX', 'NYSE', 'NASDAQ' or none (use values from all exchanges). Default is: not specified.")
		("begin-date",			po::value<std::string>(&this->begin_date_),	"Start date for extracting data from database source.")
		("output-chart-dir",	po::value<fs::path>(&this->output_chart_directory_),	"output directory for chart [and graphic] files.")
		("output-graph-dir",	po::value<fs::path>(&this->output_graphs_directory_),	"name of output directory to write generated graphics to.")
		("boxsize,b",			po::value<std::vector<DprDecimal::DDecQuad>>(&this->box_size_list_)->required(),   	"box step size. 'n', 'm.n'")
		("reversal,r",			po::value<std::vector<int32_t>>(&this->reversal_boxes_list_)->required(),		"reversal size in number of boxes. Default is 2")
		("max-graphic-cols",	po::value<int32_t>(&this->max_columns_for_graph_)->default_value(-1),
									"maximum number of columns to show in graphic. Use -1 for ALL, 0 to keep existing value, if any, otherwise -1. >0 to specify how many columns.")
		("show-trend-lines",	po::value<std::string>(&this->trend_lines_)->default_value("no"),	"Show trend lines on graphic. Can be 'data' or 'angle'. Default is 'no'.")
		("log-path",            po::value<fs::path>(&log_file_path_name_),	"path name for log file.")
		("log-level,l",         po::value<std::string>(&logging_level_)->default_value("information"), "logging level. Must be 'none|error|information|debug'. Default is 'information'.")

        ("quote-host",          po::value<std::string>(&this->quote_host_name_)->default_value("api.tiingo.com"), "web site we download from. Default is 'api.tiingo.com'.")
        ("quote-port",          po::value<std::string>(&this->quote_host_port_)->default_value("443"), "Port number to use for web site. Default is '443'.")

        ("db-host",             po::value<std::string>(&this->db_params_.host_name_)->default_value("localhost"), "web location where database is running. Default is 'localhost'.")
        ("db-port",             po::value<int32_t>(&this->db_params_.port_number_)->default_value(5432), "Port number to use for database access. Default is '5432'.")
        ("db-user",             po::value<std::string>(&this->db_params_.user_name_), "Database user name.  Required if using database.")
        ("db-name",             po::value<std::string>(&this->db_params_.db_name_), "Name of database containing PF_Chart data. Required if using database.")
        ("db-mode",             po::value<std::string>(&this->db_params_.db_mode_)->default_value("test"), "'test' or 'live' schema to use. Default is 'test'.")
        ("db-data-source",      po::value<std::string>(&this->db_params_.db_data_source_)->default_value("stock_data.current_data"), "table containing symbol data. Default is 'stock_data.current_data'.")

        ("key",                 po::value<fs::path>(&this->tiingo_api_key_)->default_value("./tiingo_key.dat"), "Path to file containing tiingo api key. Default is './tiingo_key.dat'.")
		("use-ATR",             po::value<bool>(&use_ATR_)->default_value(false)->implicit_value(true), "compute Average True Value and use to compute box size for streaming.")
		;

}		// -----  end of method PF_CollectDataApp::Do_SetupPrograoptions_  -----


void PF_CollectDataApp::ParseProgramOptions (const std::vector<std::string>& tokens)
{
    if (tokens.empty())
    {
	    auto options = po::parse_command_line(argc_, argv_, *newoptions_);
        po::store(options, variablemap_);
        if (this->argc_ == 1 ||	variablemap_.count("help") == 1)
        {
            std::cout << *newoptions_ << "\n";
            throw std::runtime_error("\nExiting after 'help'.");
        }
    }
    else
    {
        auto options = po::command_line_parser(tokens).options(*newoptions_).run();
        po::store(options, variablemap_);
        if (variablemap_.count("help") == 1)
        {
            std::cout << *newoptions_ << "\n";
            throw std::runtime_error("\nExiting after 'help'.");
        }
    }
	po::notify(variablemap_);    
}		/* -----  end of method ExtractorApp::ParsePrograoptions_  ----- */


std::tuple<int, int, int> PF_CollectDataApp::Run()
{
    if (new_data_source_ != Source::e_DB)
    {
        api_key_ = LoadDataFileForUse(tiingo_api_key_);
        if (api_key_.ends_with('\n'))
        {
            api_key_.resize(api_key_.size() - 1);
        }
    }

    // TODO(dpriedel): this should be a program param... 

    number_of_days_history_for_ATR_ = 20;

    if (new_data_source_ == Source::e_streaming)
    {
        Run_Streaming();
        return {};
    }

	if (mode_ == Mode::e_daily_scan)
	{
		Run_DailyScan();
		return {};
	}

    if(new_data_source_ == Source::e_file)
    {
        if (mode_ == Mode::e_load)
        {
			Run_Load();
        }
        else if (mode_ == Mode::e_update)
        {
            Run_Update();
        }
    }
    else if(new_data_source_ == Source::e_DB)
    {
        if (mode_ == Mode::e_load)
        {
			Run_LoadFromDB();
        }
        else if (mode_ == Mode::e_update)
        {
            Run_UpdateFromDB();
        }
    }
	return {} ;
}		// -----  end of method PF_CollectDataApp::Do_Run  -----

void PF_CollectDataApp::Run_Load()
{
    auto params = ranges::views::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_,  scale_list_);

	// TODO(dpriedel): move file read out of loop and into a buffer
	
    for (const auto& val : params)
    {
        const auto& symbol = std::get<PF_Chart::e_symbol>(val);
        try
        {
            fs::path symbol_file_name = new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
            BOOST_ASSERT_MSG(fs::exists(symbol_file_name), fmt::format("Can't find data file: {} for symbol: {}.", symbol_file_name, symbol).c_str());
            // TODO(dpriedel): add json code
            BOOST_ASSERT_MSG(source_format_ == SourceFormat::e_csv, "JSON files are not yet supported for loading symbol data.");
            auto atr = use_ATR_ ? ComputeATRForChart(symbol) : 0.0;
            PF_Chart new_chart(val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_);
            AddPriceDataToExistingChartCSV(new_chart, symbol_file_name);
            charts_.emplace_back(std::make_pair(symbol, new_chart));
        }
        catch (const std::exception& e)
        {
			std::cout << "Unable to load data for symbol: " << symbol << " because: " << e.what() << std::endl;	
        }
    }
}		// -----  end of method PF_CollectDataApp::Run_Load  -----

void PF_CollectDataApp::Run_LoadFromDB()
{
	// we can handle mass symbol loads because we do a symbol-at-a-time DB query so we don't get an impossible amount
	// of data back from the DB
	
	PF_DB pf_db{db_params_};
	
	if (symbol_list_i_ == "*")
	{
		symbol_list_ = pf_db.ListSymbolsOnExchange(exchange_);
	}
	// fmt::print("symbol list: {}\n", symbol_list_);

	pqxx::connection c{fmt::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";

    std::vector<DateCloseRecord> closing_prices;

    std::istringstream time_stream;
    date::utc_time<date::utc_clock::duration> tp;

    // we know our database contains 'date's, but we need timepoints.
    // we'll handle that in the conversion routine below.

    auto Row2Closing = [dt_format, &time_stream, &tp](const auto& r) {
        time_stream.clear();
        time_stream.str(std::string{std::get<0>(r)});
        date::from_stream(time_stream, dt_format, tp);
        DateCloseRecord new_data{.date_=tp, .close_=std::get<1>(r)};
        return new_data;
    };

	for (const auto& symbol : symbol_list_)
	{
		try
		{
			// first, get ready to retrieve our data from DB.  Do this once per symbol.

			std::string get_symbol_prices_cmd = fmt::format("SELECT date, {} FROM {} WHERE symbol = {} AND date >= {} ORDER BY date ASC",
					price_fld_name_,
					db_params_.db_data_source_,
					c.quote(symbol),
					c.quote(begin_date_)
					);

            closing_prices = pf_db.RunSQLQueryUsingStream<DateCloseRecord, std::string_view, std::string_view>(c, get_symbol_prices_cmd, Row2Closing);

			// fmt::print("done retrieving data for symbol: {}. Got {} rows.\n", symbol, db_data.size());

   	   	    // only need to compute this once per symbol also 
   	   	    auto atr = use_ATR_ ? ComputeATRForChartFromDB(symbol) : 0.0;

			// There could be thousands of symbols in the database so we don't want to generate combinations for
			// all of them at once.
    		// so, make a single element list for the call below and then generate the other combinations.

			std::vector<std::string> the_symbol{symbol};
    		auto params = ranges::views::cartesian_product(the_symbol, box_size_list_, reversal_boxes_list_, scale_list_);
			// ranges::for_each(params, [](const auto& x) {fmt::print("{}\n", x); });

    		for (const auto& val : params)
    		{
				PF_Chart new_chart(val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_);
				try
				{
					for (const auto& [new_date, new_price] : closing_prices)
					{
						// std::cout << "new value: " << new_price << "\t" << new_date << std::endl;
						new_chart.AddValue(new_price, date::clock_cast<date::utc_clock>(new_date));
					}
					charts_.emplace_back(std::make_pair(symbol, new_chart));
				}
   	    		catch (const std::exception& e)
   	    		{
					std::cout << "Unable to load data for symbol chart: " << new_chart.ChartName("") << " because: " << e.what() << std::endl;
   	    		}
   	        }
   	    }
   	    catch (const std::exception& e)
   	    {
			std::cout << "Unable to load data for symbol: " << symbol << " because: " << e.what() << std::endl;
   	    }
    }
}		// -----  end of method PF_CollectDataApp::Run_Load  -----

void PF_CollectDataApp::Run_Update()
{
    auto params = ranges::views::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    // look for existing data and load the saved JSON data if we have it.
    // then add the new data to the chart.

    for (const auto& val : params)
    {
        const auto& symbol = std::get<PF_Chart::e_symbol>(val);
        try
        {
            fs::path existing_data_file_name = input_chart_directory_ / PF_Chart::ChartName(val, "json");
            PF_Chart new_chart;
            if (fs::exists(existing_data_file_name))
            {
                new_chart = LoadAndParsePriceDataJSON(existing_data_file_name);
                if (max_columns_for_graph_ != 0)
                {
                	new_chart.SetMaxGraphicColumns(max_columns_for_graph_);
                }
            }
            else
            {
                // no existing data to update, so make a new chart

                auto atr = use_ATR_ ? ComputeATRForChart(symbol) : 0.0;
				new_chart = PF_Chart(val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_);
            }
            fs::path update_file_name = new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
            BOOST_ASSERT_MSG(fs::exists(update_file_name), fmt::format("Can't find data file for symbol: {} for update.", update_file_name).c_str());
            // TODO(dpriedel): add json code
            BOOST_ASSERT_MSG(source_format_ == SourceFormat::e_csv, "JSON files are not yet supported for updating symbol data.");
            AddPriceDataToExistingChartCSV(new_chart, update_file_name);
            charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
        }
        catch (const std::exception& e)
        {
			std::cout << "Unable to update data for symbol: " << symbol << " because: " << e.what() << std::endl;	
        }
    }
}		// -----  end of method PF_CollectDataApp::Run_Update  -----

void PF_CollectDataApp::Run_UpdateFromDB()
{
	// if we are down this path then the expectation is that we are processing a 'short' list of symbols and a 'recent'
	// date so we will be collecting a 'small' amount of data from the DB.  In this case, we can do it all up front

	auto db_data = GetPriceDataForSymbolList();
	// ranges::for_each(db_data, [](const auto& xx) {fmt::print("{}, {}, {}\n", xx.symbol, xx.tp, xx.price); });

    // look for existing data and load the saved JSON data if we have it.
    // then add the new data to the chart.

    for (const auto& symbol : symbol_list_)
    {
		// fmt::print("symbol: {}\n", symbol);
		std::vector<std::string> the_symbol{symbol};
    	auto params = ranges::views::cartesian_product(the_symbol, box_size_list_, reversal_boxes_list_, scale_list_);

        auto data_for_symbol = ranges::views::chunk_by([](const auto& a, const auto& b) { return a.symbol_ == b.symbol_; });

    	for (const auto& val : params)
    	{
        	try
        	{
            	PF_Chart new_chart;
            	if (chart_data_source_ == Source::e_file)
            	{
            		fs::path existing_data_file_name = input_chart_directory_ / PF_Chart::ChartName(val, "json");
            		if (fs::exists(existing_data_file_name))
            		{
                		new_chart = LoadAndParsePriceDataJSON(existing_data_file_name);
                		if (max_columns_for_graph_ != 0)
                		{
                			new_chart.SetMaxGraphicColumns(max_columns_for_graph_);
                		}
            		}
            	}
            	else		// should only be database here 
            	{
					new_chart = PF_Chart::MakeChartFromDB(db_params_, val);
            	}
            	if (new_chart.IsEmpty())
            	{
                	// no existing data to update, so make a new chart

                	auto atr = use_ATR_ ? ComputeATRForChartFromDB(symbol) : 0.0;
					new_chart = PF_Chart(val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_);
            	}

                // our data from the DB is grouped by symbol and we've split it into sub-ranges by symbol above.
                // now, grab the sub-range for each symbol.

                for (const auto& symbol_data : db_data | data_for_symbol | ranges::views::filter([&symbol](const auto& rec_rng){ return rec_rng[0].symbol_ == symbol; }))
                {
                    ranges::for_each(symbol_data, [&new_chart](const auto& row) { new_chart.AddValue(row.close_, row.date_); });
                }
            	// AddPriceDataToExistingChartCSV(new_chart, update_file_name);
            	charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
        	}
        	catch (const std::exception& e)
        	{
				std::cout << "Unable to update data for symbol: " << symbol << " because: " << e.what() << std::endl;
        	}
    	}
    }
}		// -----  end of method PF_CollectDataApp::Run_UpdateFromDB  -----

std::vector<MultiSymbolDateCloseRecord> PF_CollectDataApp::GetPriceDataForSymbolList () const
{
	// we need to convert our list of symbols into a format that can be used in a SQL query.
	
	std::string query_list = "( '";
	auto syms = symbol_list_.begin();
	query_list += *syms;
	for (++syms; syms != symbol_list_.end(); ++syms)
	{
		query_list += "', '";
		query_list += *syms;
	}
	query_list += "' )";
	std::cout << query_list << std::endl;

    PF_DB pf_db{db_params_};
	
	pqxx::connection c{fmt::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

	// we need a place to keep the data we retrieve from the database.
	
    std::vector<MultiSymbolDateCloseRecord> db_data;

    std::istringstream time_stream;
    date::utc_time<date::utc_clock::duration> tp;

    const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";

    // we know our database contains 'date's, but we need timepoints.
    // we'll handle that in the conversion routine below.

    auto Row2Closing = [dt_format, &time_stream, &tp](const auto& r) {
        time_stream.clear();
        time_stream.str(std::string{std::get<1>(r)});
        date::from_stream(time_stream, dt_format, tp);
        MultiSymbolDateCloseRecord new_data{.symbol_=std::string{std::get<0>(r)},.date_=tp, .close_=std::get<2>(r)};
        return new_data;
    };

	try
	{
		// first, get ready to retrieve our data from DB.  Do this for all our symbols here.

		std::string get_symbol_prices_cmd = fmt::format("SELECT symbol, date, {} FROM {} WHERE symbol in {} AND date >= {} ORDER BY symbol, date ASC",
				price_fld_name_,
				db_params_.db_data_source_,
				query_list,
				c.quote(begin_date_)
				);

        db_data = pf_db.RunSQLQueryUsingStream<MultiSymbolDateCloseRecord, std::string_view, std::string_view, std::string_view>(c, get_symbol_prices_cmd, Row2Closing);
		std::cout << "done retrieving data for symbols: " << query_list << " got: " << db_data.size() << " rows." << std::endl;
   	}
   	catch (const std::exception& e)
   	{
		std::cout << "Unable to load data for symbols: " << query_list << " because: " << e.what() << std::endl;
   	}

	return db_data;
}		// -----  end of method PF_CollectDataApp::GetDataForSymbolList  ----- 

void PF_CollectDataApp::Run_Streaming()
{
    auto params = ranges::views::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    auto current_local_time = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    auto market_status = GetUS_MarketStatus(std::string_view{date::current_zone()->name()}, current_local_time.get_local_time());

    if (market_status != US_MarketStatus::e_NotOpenYet && market_status != US_MarketStatus::e_OpenForTrading)
    {
        std::cout << "Market not open for trading now so we can't stream quotes.\n";
        return;
    }

    if (market_status == US_MarketStatus::e_NotOpenYet)
    {
        std::cout << "Market not open for trading YET so we'll wait." << std::endl;
    }

    for (const auto& val : params)
    {
        const auto& symbol = std::get<PF_Chart::e_symbol>(val);
        auto atr = use_ATR_ ? ComputeATRForChart(symbol) : 0.0;
        PF_Chart new_chart(val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_);
        charts_.emplace_back(std::make_pair(symbol, new_chart));
    }
    // let's stream !
    
    PrimeChartsForStreaming();
    CollectStreamingData();
}

void    PF_CollectDataApp::AddPriceDataToExistingChartCSV(PF_Chart& new_chart, const fs::path& update_file_name) const
{
    const std::string file_content = LoadDataFileForUse(update_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content, '\n');
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ',');
    BOOST_ASSERT_MSG(date_column.has_value(), fmt::format("Can't find 'date' field in header record: {}.", header_record).c_str());
    
    auto close_column = FindColumnIndex(header_record, price_fld_name_, ',');
    BOOST_ASSERT_MSG(close_column.has_value(), fmt::format("Can't find price field: {} in header record: {}.", price_fld_name_, header_record).c_str());

    ranges::for_each(symbol_data_records | ranges::views::drop(1), [this, &new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record)
        {
            const auto fields = split_string<std::string_view> (record, ',');
            const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";
            new_chart.AddValue(DprDecimal::DDecQuad(fields[close_col]), StringToUTCTimePoint(dt_format, fields[date_col]));
        });

}		// -----  end of method PF_CollectDataApp::AddPriceDataToExistingChartCSV  ----- 

 PF_Chart PF_CollectDataApp::LoadAndParsePriceDataJSON (const fs::path& symbol_file_name) 
{
    const std::string file_content = LoadDataFileForUse(symbol_file_name);

    JSONCPP_STRING err;
    Json::Value saved_data;

    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    if (! reader->parse(file_content.data(), file_content.data() + file_content.size(), &saved_data, &err))
    {
        throw std::runtime_error("Problem parsing test data file: "s + err);
    }

    PF_Chart new_chart{saved_data};
    return new_chart;
}		// -----  end of method PF_CollectDataApp::LoadAndParsePriceDataJSON  ----- 

std::optional<int> PF_CollectDataApp::FindColumnIndex (std::string_view header, std::string_view column_name, char delim) 
{
    auto fields = rng_split_string<std::string_view>(header, delim);
    auto do_compare([&column_name](const auto& field_name)
    {
        // need case insensitive compare
        // found this on StackOverflow (but modified for my use)
        // (https://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c)

        if (column_name.size() != field_name.size())
        {
            return false;
        }
        return ranges::equal(column_name, field_name, [](unsigned char a, unsigned char b) { return tolower(a) == tolower(b); });
    });

    if (auto found_it = ranges::find_if(fields, do_compare); found_it != ranges::end(fields))
    {
        return ranges::distance(ranges::begin(fields), found_it);
    }
    return {};

}		// -----  end of method PF_CollectDataApp::FindColumnIndex  ----- 


DprDecimal::DDecQuad PF_CollectDataApp::ComputeATRForChart (const std::string& symbol) const
{
    Tiingo history_getter{quote_host_name_, quote_host_port_, api_key_};

    // we need to start from yesterday since we won't get history data for today 
    // since we are doing this while the market is open

    date::year_month_day today{--floor<date::days>(std::chrono::system_clock::now())};

    // use our new holidays capability 
    // we look backwards here. so add an extra year in case we are near New Years.
    
    auto holidays = MakeHolidayList(today.year());
    ranges::copy(MakeHolidayList(--(today.year())), std::back_inserter(holidays));

    const auto history = history_getter.GetMostRecentTickerData(symbol, today, number_of_days_history_for_ATR_ + 1, UseAdjusted::e_Yes, &holidays);

    auto atr = ComputeATR(symbol, history, number_of_days_history_for_ATR_);

    return atr;
}		// -----  end of method PF_CollectDataApp::ComputeBoxSizeUsingATR  ----- 

DprDecimal::DDecQuad PF_CollectDataApp::ComputeATRForChartFromDB (const std::string& symbol) const
{
    auto today = date::year_month_day{floor<date::days>(std::chrono::system_clock::now())};
//    date::year which_year = today.year();
//    auto holidays = MakeHolidayList(which_year);
//    ranges::copy(MakeHolidayList(--which_year), std::back_inserter(holidays));
//    
//    auto business_days = ConstructeBusinessDayRange(today, 2, UpOrDown::e_Down, &holidays);

    // 'business_days.second' will be the prior business day for DB search.
    // BUT, I expect the DB will only have data for trading days, so it will automatically 
    // skip weekends for me.
	std::string get_ATR_info_cmd = fmt::format("SELECT date, exchange, symbol, open_p, high, low, close_p FROM {} WHERE symbol = '{}' AND date <= '{}' ORDER BY date DESC LIMIT {}",
			db_params_.db_data_source_,
			symbol,
            today,
			number_of_days_history_for_ATR_ + 1		// need an extra row for the algorithm 
			);

    PF_DB the_db{db_params_};

    DprDecimal::DDecQuad atr{};
	try
	{
		auto price_data = the_db.RetrieveMostRecentStockDataRecordsFromDB(symbol, today, number_of_days_history_for_ATR_ + 1);
        atr = ComputeATR(symbol, price_data, number_of_days_history_for_ATR_);
    }
   	catch (const std::exception& e)
   	{
        fmt::print("Unable to comput ATR from DB for: '{}' because: {}.\n", symbol, e.what());
   	}

    return atr;
}		// -----  end of method PF_CollectDataApp::ComputeATRUsingDB  ----- 

void PF_CollectDataApp::PrimeChartsForStreaming ()
{
    // for streaming, we want to retrieve the previous day's close and, if the markets 
    // are already open, the day's open.  We do this to capture 'gaps' and to set 
    // the direction at little sooner.

    auto today = date::year_month_day{floor<date::days>(std::chrono::system_clock::now())};
    date::year which_year = today.year();
    auto holidays = MakeHolidayList(which_year);
    ranges::copy(MakeHolidayList(--which_year), std::back_inserter(holidays));
    
    auto current_local_time = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    auto market_status = GetUS_MarketStatus(std::string_view{date::current_zone()->name()}, current_local_time.get_local_time());

    Tiingo history_getter{"api.tiingo.com", "443", "/iex", api_key_, symbol_list_};

    if (market_status == US_MarketStatus::e_NotOpenYet)
    {
        for (auto& [symbol, chart] : charts_)
        {
            auto history = history_getter.GetMostRecentTickerData(symbol, today, 2, price_fld_name_.starts_with("adj") ? UseAdjusted::e_Yes : UseAdjusted::e_No, &holidays);
            chart.AddValue(history[0].close_, date::clock_cast<date::utc_clock>(current_local_time.get_sys_time()));
        }
    }
    else if (market_status == US_MarketStatus::e_OpenForTrading)
    {
        auto history = history_getter.GetTopOfBookAndLastClose();
        for (const auto& e : history)
        {
            const std::string ticker = e["ticker"].asString();
            const std::string tstmp = e["timestamp"].asString();
            const auto quote_time_stamp = StringToUTCTimePoint("%FT%T%z", tstmp);
            const auto close_time_stamp = date::clock_cast<date::utc_clock>(GetUS_MarketOpenTime(today).get_sys_time() - std::chrono::seconds{60});
            const auto open_time_stamp = date::clock_cast<date::utc_clock>(GetUS_MarketOpenTime(today).get_sys_time());

            try{
				ranges::for_each(charts_ | ranges::views::filter([&ticker] (auto& symbol_and_chart) { return symbol_and_chart.first == ticker; }),
						[&] (auto& symbol_and_chart)
						{
						symbol_and_chart.second.AddValue(DprDecimal::DDecQuad{e["prevClose"].asString()}, close_time_stamp);
						symbol_and_chart.second.AddValue(DprDecimal::DDecQuad{e["open"].asString()}, open_time_stamp);
						symbol_and_chart.second.AddValue(DprDecimal::DDecQuad{e["last"].asString()}, quote_time_stamp);
						});
            }
            catch (const std::exception& e)
            {
				std::cout << "Problem initializing streaming data for symbol: " << ticker << " because: " << e.what() << std::endl;
            }
        }
    }
}		// -----  end of method PF_CollectDataApp::PrimeChartsForStreaming  ----- 

void PF_CollectDataApp::CollectStreamingData ()
{
    // we're going to use 2 threads here -- a producer thread which collects streamed 
    // data from Tiingo and a consummer thread which will take that data, decode it 
    // and load it into appropriate charts. 
    // Processing continues until interrupted. 

    // we've added a third thread to manage a countdown timer which will interrupt our 
    // producer thread at market close.  Otherwise, the producer thread will hang forever 
    // or until interrupted.

    // since this code can potentially run for hours on end 
    // it's a good idea to provide a way to break into this processing and shut it down cleanly.
    // so, a little bit of C...(taken from "Advanced Unix Programming" by Warren W. Gay, p. 317)

    // ok, get ready to handle keyboard interrupts, if any.

    struct sigaction sa_old{};
    struct sigaction sa_new{};

    sa_new.sa_handler = PF_CollectDataApp::HandleSignal;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
    sigaction(SIGINT, &sa_new, &sa_old);

    PF_CollectDataApp::had_signal_= false;

    Tiingo quotes{quote_host_name_, quote_host_port_, "/iex", api_key_, symbol_list_};
    quotes.Connect();

    // if we are here then we already know that the US market is open for trading.

    auto today = date::year_month_day{floor<std::chrono::days>(std::chrono::system_clock::now())};
    // add a couple minutes for padding
    auto local_market_close = date::zoned_seconds(date::current_zone(), GetUS_MarketCloseTime(today).get_sys_time() + 2min);

    std::mutex data_mutex;
    std::queue<std::string> streamed_data;

    py::gil_scoped_release gil{};

    auto timer_task = std::async(std::launch::async, &PF_CollectDataApp::WaitForTimer, local_market_close);
    auto streaming_task = std::async(std::launch::async, &Tiingo::StreamData, &quotes, &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);
    auto processing_task = std::async(std::launch::async, &PF_CollectDataApp::ProcessStreamedData, this, &quotes, &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);

    streaming_task.get();
    processing_task.get();
    timer_task.get();

    quotes.Disconnect();

    // make a last check to be sure we  didn't leave any data unprocessed 

    ProcessStreamedData(&quotes, &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);

}		// -----  end of method PF_CollectDataApp::CollectStreamingData  ----- 

void PF_CollectDataApp::ProcessStreamedData (Tiingo* quotes, const bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
{
//    py::gil_scoped_acquire gil{};
    while(true)
    {
        if (! streamed_data->empty())
        {
            std::string new_data;
            {
                const std::lock_guard<std::mutex> queue_lock(*data_mutex);
                new_data = streamed_data->front();
                streamed_data->pop();
            }
            const auto pf_data = quotes->ExtractData(new_data);

            // keep track of what needs to be updated 

            std::vector<PF_Chart*> need_to_update_graph;

            // since we can have multiple charts for each symbol, we need to pass the new value
            // to all appropriate charts so we find all the charts for each symbol and give each a 
            // chance at the new data.

            for (const auto& new_value: pf_data)
            {
                ranges::for_each(charts_ | ranges::views::filter([&new_value] (const auto& symbol_and_chart) { return symbol_and_chart.first == new_value.ticker_; }),
                    [&] (auto& symbol_and_chart)
                    {
                        auto chart_changed = symbol_and_chart.second.AddValue(new_value.last_price_, PF_Column::TmPt{std::chrono::nanoseconds{new_value.time_stamp_nanoseconds_utc_}});
                        if (chart_changed != PF_Column::Status::e_ignored)
                        {
                            need_to_update_graph.push_back(&symbol_and_chart.second);
                        }
                    });
            }

            // we could have multiple chart updates for any given symbol but we only
            // want to update files and graphic once per symbol.

            need_to_update_graph |= ranges::actions::sort | ranges::actions::unique;

            for (const PF_Chart* chart : need_to_update_graph)
            {
                py::gil_scoped_acquire gil{};
                fs::path graph_file_path = output_graphs_directory_ / (chart->ChartName("svg"));
                chart->ConstructChartGraphAndWriteToFile(graph_file_path, trend_lines_, PF_Chart::X_AxisFormat::e_show_time);

                fs::path chart_file_path = output_chart_directory_ / (chart->ChartName("json"));
                chart->ConvertChartToJsonAndWriteToFile(chart_file_path);
            }
        }
        else
        {
            std::this_thread::sleep_for(2ms);
        }
        if (streamed_data->empty() && *had_signal)
        {
            break;
        }
    }
}		// -----  end of method PF_CollectDataApp::ProcessStreamedData  ----- 

void PF_CollectDataApp::Run_DailyScan()
{

}		// -----  end of method PF_CollectDataApp::Run_DailyScan  ----- 

void PF_CollectDataApp::Shutdown ()
{
    if (destination_ == Destination::e_file)
    {
        for (const auto& [symbol, chart] : charts_)
        {
            try
            {
				fs::path output_file_name = output_chart_directory_ / chart.ChartName("json"); 
				chart.ConvertChartToJsonAndWriteToFile(output_file_name);

            	if (graphics_format_ == GraphicsFormat::e_svg)
            	{
					fs::path graph_file_path = output_graphs_directory_ / (chart.ChartName("svg"));
					chart.ConstructChartGraphAndWriteToFile(graph_file_path, trend_lines_, interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time : PF_Chart::X_AxisFormat::e_show_date);
            	}
            	else
            	{
					fs::path graph_file_path = output_graphs_directory_ / (chart.ChartName("csv"));
					chart.ConvertChartToTableAndWriteToFile(graph_file_path, interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time : PF_Chart::X_AxisFormat::e_show_date);
            	}
            }
			catch(const std::exception& e)
			{
        		spdlog::error(fmt::format("Problem in shutdown: {} for chart: {}.\nTrying to complete shutdown.", e.what(), chart.ChartName("")));
    		}
        }
    }
    else
    {
		PF_DB pf_db{db_params_};
        for (const auto& [symbol, chart] : charts_)
        {
			try
			{
				if (graphics_format_ == GraphicsFormat::e_svg)
				{
					fs::path graph_file_path = output_graphs_directory_ / (chart.ChartName("svg"));
					chart.ConstructChartGraphAndWriteToFile(graph_file_path, trend_lines_, interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time : PF_Chart::X_AxisFormat::e_show_date);
				}
				chart.StoreChartInChartsDB(pf_db, interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time : PF_Chart::X_AxisFormat::e_show_date, true);
			}
			catch(const std::exception& e)
			{
        		spdlog::error(fmt::format("Problem storing data in DB in shutdown: {} for chart: {}.\nTrying to complete shutdown.", e.what(), chart.ChartName("")));
			}
        }
    }

    spdlog::info(fmt::format("\n\n*** End run {:%a, %b %d, %Y at %I:%M:%S %p %Z} ***\n", std::chrono::system_clock::now()));
}       // -----  end of method PF_CollectDataApp::Shutdown  -----

void PF_CollectDataApp::WaitForTimer (const date::zoned_seconds& stop_at)
{
    while(true)
    {
        // if the user has signaled time to leave, then do it 

        if (PF_CollectDataApp::had_signal_)
        {
            std::cout << "\n*** User interrupted. ***" << std::endl;
            break;
        }

        const date::zoned_seconds now = date::zoned_seconds(date::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        if (now.get_sys_time() < stop_at.get_sys_time())
        {
            std::this_thread::sleep_for(1min);
        }
        else
        {
            std::cout << "\n*** Timer expired. ***" << std::endl;
            PF_CollectDataApp::had_signal_ = true;
            break;
        }
    }
}		// -----  end of method PF_CollectDataApp::WaitForTimer  ----- 


void PF_CollectDataApp::HandleSignal(int signal)

{
    // only thing we need to do

    PF_CollectDataApp::had_signal_ = true;

}		/* -----  end of method PF_CollectDataApp::HandleSignal  ----- */

