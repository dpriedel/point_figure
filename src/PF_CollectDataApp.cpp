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
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <queue>
#include <ranges>
#include <sstream>
#include <string_view>
#include <thread>
#include <type_traits>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <date/chrono_io.h>
#include <date/tz.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

// #include <pybind11/embed.h>
// #include <pybind11/gil.h>
#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>

#include <range/v3/range/conversion.hpp>

// namespace py = pybind11;

// using namespace py::literals;

// #include "DDecQuad.h"
#include "ConstructChartGraphic.h"
#include "Eodhd.h"
#include "PF_Chart.h"
#include "PF_CollectDataApp.h"
#include "PF_Column.h"
#include "PointAndFigureDB.h"
#include "Tiingo.h"
#include "utilities.h"

using decimal::Decimal;

using namespace std::string_literals;
using namespace std::chrono_literals;

bool PF_CollectDataApp::had_signal_ = false;

// code from "The C++ Programming Language" 4th Edition. p. 1243.

template <typename T>
int wait_for_any(std::vector<std::future<T>> &vf, std::chrono::steady_clock::duration d)
// return index of ready future
// if no future is ready, wait for d before trying again
{
    while (true)
    {
        for (int i = 0; i != vf.size(); ++i)
        {
            if (!vf[i].valid())
            {
                continue;
            }
            switch (vf[i].wait_for(0s))
            {
                case std::future_status::ready:
                    return i;

                case std::future_status::timeout:
                    break;

                case std::future_status::deferred:
                    throw std::runtime_error("wait_for_all(): deferred future");
            }
        }
        std::this_thread::sleep_for(d);
    }
}

//--------------------------------------------------------------------------------------
//       Class:  PF_CollectDataApp
//      Method:  PF_CollectDataApp
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_CollectDataApp::PF_CollectDataApp(int argc, char *argv[])
    : argc_{argc}, argv_{argv} {}  // -----  end of method PF_CollectDataApp::PF_CollectDataApp  (constructor)
                                   // -----

//--------------------------------------------------------------------------------------
//       Class:  PF_CollectDataApp
//      Method:  PF_CollectDataApp
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_CollectDataApp::PF_CollectDataApp(const std::vector<std::string> &tokens)
    : tokens_{tokens} {}  // -----  end of method PF_CollectDataApp::PF_CollectDataApp  (constructor)
                          // -----

void PF_CollectDataApp::ConfigureLogging()
{
    // we need to set log level if specified and also log file.

    if (!log_file_path_name_.empty())
    {
        // if we are running inside our test harness, logging may already by
        // running so we don't want to clobber it.
        // different tests may use different names.

        auto logger_name = log_file_path_name_.filename();
        logger_ = spdlog::get(logger_name);
        if (!logger_)
        {
            fs::path log_dir = log_file_path_name_.parent_path();
            if (!fs::exists(log_dir))
            {
                fs::create_directories(log_dir);
            }

            logger_ = spdlog::basic_logger_mt<spdlog::async_factory>(logger_name, log_file_path_name_.c_str());
            spdlog::set_default_logger(logger_);
        }
    }

    // we are running before 'CheckArgs' so we need to do a little editiing
    // ourselves.

    const std::map<std::string, spdlog::level::level_enum> levels{{"none", spdlog::level::off},
                                                                  {"error", spdlog::level::err},
                                                                  {"information", spdlog::level::info},
                                                                  {"debug", spdlog::level::debug}};

    auto which_level = levels.find(logging_level_);
    BOOST_ASSERT_MSG(
        which_level != levels.end(),
        std::format("log-level: {} must be 1 of 'none', 'error', 'information', 'debug'.", logging_level_).c_str());

    spdlog::set_level(which_level->second);

}  // -----  end of method PF_CollectDataApp::ConfigureLogging  -----

bool PF_CollectDataApp::Startup()
{
    constexpr const char *time_fmt = "\n\n*** Begin run {:%a, %b %d, %Y at %I:%M:%S %p %Z}  ***\n";
    spdlog::info(std::format("\n\n*** Starting run {} ***\n",
                             std::chrono::current_zone()->to_local(std::chrono::system_clock::now())));
    bool result{true};
    try
    {
        SetupProgramOptions();
        ParseProgramOptions(tokens_);
        ConfigureLogging();
        result = CheckArgs();
    }
    catch (const std::exception &e)
    {
        spdlog::error(std::format("Problem in startup: {}\n", e.what()));
        //	we're outta here!

        //		this->Shutdown();
        result = false;
    }
    catch (...)
    {
        spdlog::error("Unexpectd problem during Starup processing\n");

        //	we're outta here!

        //		this->Shutdown();
        result = false;
    }
    return result;
}  // -----  end of method PF_CollectDataApp::Do_StartUp  -----

bool PF_CollectDataApp::CheckArgs()
{
    //	an easy check first

    BOOST_ASSERT_MSG(!(use_ATR_ && use_min_max_), "\nCan not use both ATR and MinMax for computing box size.");
    boxsize_source_ = (use_ATR_       ? BoxsizeSource::e_from_ATR
                       : use_min_max_ ? BoxsizeSource::e_from_MinMax
                                      : BoxsizeSource::e_from_args);

    //	let's get our input and output set up

    BOOST_ASSERT_MSG(mode_i_ == "load" || mode_i_ == "update" || mode_i_ == "daily-scan",
                     std::format("\nMode must be: 'load', 'update' or 'daily-scan': {}", mode_i_).c_str());
    mode_ = mode_i_ == "load" ? Mode::e_load : mode_i_ == "update" ? Mode::e_update : Mode::e_daily_scan;

    // now make sure we can find our data for input and output.

    BOOST_ASSERT_MSG(
        new_data_source_i_ == "file" || new_data_source_i_ == "streaming" || new_data_source_i_ == "database",
        std::format("\nNew data source must be: 'file', 'streaming' or 'database': {}", new_data_source_i_).c_str());
    new_data_source_ = new_data_source_i_ == "file"       ? Source::e_file
                       : new_data_source_i_ == "database" ? Source::e_DB
                                                          : Source::e_streaming;

    BOOST_ASSERT_MSG(
        chart_data_source_i_ == "file" || chart_data_source_i_ == "database",
        std::format("\nExisting chart data source must be: 'file' or 'database': {}", chart_data_source_i_).c_str());
    chart_data_source_ = chart_data_source_i_ == "file" ? Source::e_file : Source::e_DB;

    BOOST_ASSERT_MSG(destination_i_ == "file" || destination_i_ == "database",
                     std::format("\nData destination must be: 'file' or 'database': {}", destination_i_).c_str());
    destination_ = destination_i_ == "file" ? Destination::e_file : Destination::e_DB;

    if (mode_ == Mode::e_daily_scan || new_data_source_ == Source::e_DB)
    {
        // set up exchange list early.

        if (!exchange_list_i_.empty())
        {
            rng::for_each(split_string<std::string>(exchange_list_i_, ","),
                          [this](const auto xchng) { exchange_list_.push_back(xchng); });
            rng::for_each(exchange_list_,
                          [](auto &xchng) { rng::for_each(xchng, [](char &c) { c = std::toupper(c); }); });
            rng::sort(exchange_list_);
            const auto [first, last] = rng::unique(exchange_list_);
            exchange_list_.erase(first, last);

            const std::vector<std::string> exchanges{"AMEX",    "BATS",    "NASDAQ", "NMFQS", "NYSE", "OTCCE",
                                                     "OTCGREY", "OTCMKTS", "OTCQB",  "OTCQX", "PINK", "US"};
            rng::for_each(exchange_list_,
                          [&exchanges](const auto &xchng)
                          {
                              BOOST_ASSERT_MSG(
                                  std::ranges::find(exchanges, xchng) != exchanges.end(),
                                  fmt::format("\nexchange: {} must be one of: {}.\n", xchng, exchanges).c_str());
                          });
            spdlog::debug(fmt::format("exchanges for scan and bulk load: {}\n", exchange_list_));
        }
    }

    // validate our dates, if any

    if (!begin_date_.empty())
    {
        auto ymd = StringToDateYMD("%F", begin_date_);
    }
    if (!end_date_.empty())
    {
        auto ymd = StringToDateYMD("%F", end_date_);
    }
    else
    {
        // default to 'today'
        std::chrono::year_month_day today{--floor<std::chrono::days>(std::chrono::system_clock::now())};
        end_date_ = std::format("{:%Y-%m-%d}", today);
    }

    // do daily-scan edits upfront because we only need a couple

    if (mode_ == Mode::e_daily_scan)
    {
        BOOST_ASSERT_MSG(!db_params_.host_name_.empty(), "\nMust provide 'db-host' when mode is 'daily-scan'.");
        BOOST_ASSERT_MSG(db_params_.port_number_ != -1, "\nMust provide 'db-port' when mode is 'daily-scan'.");
        BOOST_ASSERT_MSG(!db_params_.user_name_.empty(), "\nMust provide 'db-user' when mode is 'daily-scan'.");
        BOOST_ASSERT_MSG(!db_params_.db_name_.empty(), "\nMust provide 'db-name' when mode is 'daily-scan'.");
        BOOST_ASSERT_MSG(db_params_.PF_db_mode_ == "test" || db_params_.PF_db_mode_ == "live",
                         "\n'db-mode' must be 'test' or 'live'.");
        BOOST_ASSERT_MSG(!db_params_.stock_db_data_source_.empty(),
                         "\n'db-data-source' must be specified when mode is 'daily-scan'.");

        // setup or list exchanges to use.

        BOOST_ASSERT_MSG(!begin_date_.empty(), "\nMust specify 'begin-date' when mode is 'daily-scan'.");

        // we need this

        new_data_source_ = Source::e_DB;

        // this is what we want

        graphics_format_ = GraphicsFormat::e_csv;
        return true;
    }

    // do these tests ourselves instead of specifying 'required' on Setup.
    // this is to avoid having to provide unnecessary arguments for daily scan
    // processing.

    BOOST_ASSERT_MSG(!box_size_i_list_.empty(), "\nMust provide at least 1 'boxsize' parameter.");
    BOOST_ASSERT_MSG(!reversal_boxes_list_.empty(), "\nMust provide at least 1 'reversal' parameter.");

    // this is a hack because the Decimal class has limited streams support

    std::ranges::for_each(box_size_i_list_, [this](const auto &b) { this->box_size_list_.emplace_back(Decimal{b}); });

    // we now have two possible sources for symbols. We need to be sure we have
    // 1 of them.

    BOOST_ASSERT_MSG(symbol_list_i_ != "*", "\n'*' is no longer valid for symbol-list. Use 'ALL' instead.");

    if (!symbol_list_i_.empty() && symbol_list_i_ != "ALL")
    {
        rng::for_each(split_string<std::string>(symbol_list_i_, ","),
                      [this](const auto sym) { symbol_list_.push_back(sym); });
        rng::sort(symbol_list_);
        const auto [first, last] = rng::unique(symbol_list_);
        symbol_list_.erase(first, last);
    }

    // if symbol-list is 'ALL' then we will generate a list of symbols from our
    // source database.

    if (symbol_list_i_ == "ALL")
    {
        symbol_list_.clear();
    }
    else
    {
        rng::for_each(symbol_list_, [](auto &symbol) { rng::for_each(symbol, [](char &c) { c = std::toupper(c); }); });
    }

    BOOST_ASSERT_MSG(!symbol_list_.empty() || !symbol_list_i_.empty() || !exchange_list_i_.empty(),
                     "\nMust provide either 1 or more '-s' values or 'symbol-list' or 'exchange-list' list.");

    if (use_min_max_)
    {
        BOOST_ASSERT_MSG(mode_ == Mode::e_load && new_data_source_ == Source::e_DB,
                         "\nMinMax is only available for loads using the DB as a source");
    }

    // possibly empty if this is our first time or we are starting over

    if (new_data_source_ == Source::e_file)
    {
        // for file input, we always need to have our raw data inputs

        BOOST_ASSERT_MSG(!new_data_input_directory_.empty(),
                         "\nMust specify 'new-data-dir' when data source is 'file'.");
        BOOST_ASSERT_MSG(fs::exists(new_data_input_directory_),
                         std::format("\nCan't find new data input directory: {}", new_data_input_directory_).c_str());

        BOOST_ASSERT_MSG(source_format_i_ == "csv" || source_format_i_ == "json",
                         std::format("\nNew data files must be: 'csv' or 'json': {}", source_format_i_).c_str());
        source_format_ = source_format_i_ == "csv" ? SourceFormat::e_csv : SourceFormat::e_json;

        // if we are adding to existing data then we need to know where to find
        // that data

        if (mode_ == Mode::e_update && chart_data_source_ == Source::e_file)
        {
            BOOST_ASSERT_MSG(!input_chart_directory_.empty(),
                             "\nMust specify 'chart-data-dir' when data source is "
                             "'file' and mode is 'update'.");
            BOOST_ASSERT_MSG(
                fs::exists(input_chart_directory_),
                std::format("\nCan't find new existing chart data directory: {}", input_chart_directory_).c_str());

            // we could write out data to a separate location if we want
            // otherwise, use the charts directory.

            if (output_chart_directory_.empty())
            {
                output_chart_directory_ = input_chart_directory_;
            }
        }
    }

    BOOST_ASSERT_MSG(graphics_format_i_ == "svg" || graphics_format_i_ == "csv",
                     std::format("\ngraphics-format must be either 'svg' or 'csv': {}", graphics_format_i_).c_str());
    graphics_format_ = graphics_format_i_ == "svg" ? GraphicsFormat::e_svg : GraphicsFormat::e_csv;

    if (destination_ == Destination::e_file)
    {
        BOOST_ASSERT_MSG(!output_chart_directory_.empty(),
                         "\nMust specify 'output-chart-dir' when data destination is 'file'.");
        if (!fs::exists(output_chart_directory_))
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
        BOOST_ASSERT_MSG(!output_graphs_directory_.empty(), "\nMust specify 'output-graph-dir'.");
        if (!fs::exists(output_graphs_directory_))
        {
            fs::create_directories(output_graphs_directory_);
        }
    }

    if (new_data_source_ == Source::e_DB || destination_ == Destination::e_DB)
    {
        BOOST_ASSERT_MSG(!db_params_.host_name_.empty(),
                         "\nMust provide 'db-host' when data source or destination "
                         "is 'database'.");
        BOOST_ASSERT_MSG(db_params_.port_number_ != -1,
                         "\nMust provide 'db-port' when data source or destination "
                         "is 'database'.");
        BOOST_ASSERT_MSG(!db_params_.user_name_.empty(),
                         "\nMust provide 'db-user' when data source or destination "
                         "is 'database'.");
        BOOST_ASSERT_MSG(!db_params_.db_name_.empty(),
                         "M\nust provide 'db-name' when data source or destination "
                         "is 'database'.");
        BOOST_ASSERT_MSG(db_params_.PF_db_mode_ == "test" || db_params_.PF_db_mode_ == "live",
                         "\n'db-mode' must be 'test' or 'live'.");
        if (new_data_source_ == Source::e_DB)
        {
            BOOST_ASSERT_MSG(!db_params_.stock_db_data_source_.empty(),
                             "\n'db-data-source' must be specified when load "
                             "source is 'database'.");
        }
    }

    if (new_data_source_ != Source::e_DB && use_ATR_)
    {
        BOOST_ASSERT_MSG(!tiingo_api_key_.empty(), "\nMust specify api 'key' file when data source is 'streaming'.");
        BOOST_ASSERT_MSG(fs::exists(tiingo_api_key_),
                         std::format("\nCan't find tiingo api key file: {}", tiingo_api_key_).c_str());
    }

    if (new_data_source_ == Source::e_DB)
    {
        BOOST_ASSERT_MSG(!begin_date_.empty(), "\nMust specify 'begin-date' when data source is 'database'.");
    }

    if (new_data_source_ == Source::e_streaming)
    {
        if (streaming_source_i_ == "Eodhd")
        {
            streaming_data_source_ = StreamingSource::e_Eodhd;
            BOOST_ASSERT_MSG(!eodhd_api_key_.empty(),
                             "\nMust specify 'eodhd-key' file when streaming data source is 'Eodhd'.");
            BOOST_ASSERT_MSG(fs::exists(eodhd_api_key_),
                             std::format("\nCan't find Eodhd api key file: {}", eodhd_api_key_).c_str());
        }
        else if (streaming_source_i_ == "Tiingo")
        {
            streaming_data_source_ = StreamingSource::e_Tiingo;
            BOOST_ASSERT_MSG(!tiingo_api_key_.empty(),
                             "\nMust specify 'tiingo-key' file when streaming data source is 'Tiingo'.");
            BOOST_ASSERT_MSG(fs::exists(tiingo_api_key_),
                             std::format("\nCan't find tiingo api key file: {}", tiingo_api_key_).c_str());
        }
    }

    BOOST_ASSERT_MSG(max_columns_for_graph_ >= -1, "\nmax-graphic-cols must be >= -1.");

    BOOST_ASSERT_MSG(trend_lines_ == "no" || trend_lines_ == "data" || trend_lines_ == "angle",
                     std::format("\nshow-trend-lines must be: 'no' or 'data' or 'angle': {}", trend_lines_).c_str());

    const std::map<std::string, Interval> possible_intervals = {{"eod", Interval::e_eod},   {"live", Interval::e_live},
                                                                {"sec1", Interval::e_sec1}, {"sec5", Interval::e_sec5},
                                                                {"min1", Interval::e_min1}, {"min5", Interval::e_min5}};
    auto which_interval = possible_intervals.find(interval_i_);
    BOOST_ASSERT_MSG(which_interval != possible_intervals.end(),
                     std::format("\nInterval must be: 'eod', 'live', 'sec1', "
                                 "'sec5', 'min1', 'min5': {}",
                                 interval_i_)
                         .c_str());
    interval_ = which_interval->second;

    // provide our default value here.

    if (scale_i_list_.empty())
    {
        scale_i_list_.emplace_back("linear");
    }

    // edit and translate from text to enums...

    rng::for_each(scale_i_list_,
                  [](const auto &scale)
                  {
                      BOOST_ASSERT_MSG(scale == "linear" || scale == "percent",
                                       std::format("\nChart scale must be: 'linear' or 'percent': {}", scale).c_str());
                  });
    rng::for_each(scale_i_list_, [this](const auto &scale_i)
                  { this->scale_list_.emplace_back(scale_i == "linear" ? BoxScale::e_Linear : BoxScale::e_Percent); });

    // we can compute whether boxes are fractions or intergers from input. This
    // may be changed by the Boxes code later.

    // generate PF_Chart type combinations from input params.

    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);
    rng::for_each(params,
                  [](const auto &x)
                  {
                      std::cout << std::format("{}\t{}\t{}\t{}\n", std::get<0>(x), std::get<1>(x).format("f"),
                                               std::get<2>(x), std::get<3>(x));
                  });
    std::cout << std::endl;  // force a flush

    return true;
}  // -----  end of method PF_CollectDataApp::Do_CheckArgs  -----

// clang-format off

void PF_CollectDataApp::SetupProgramOptions ()
{
    newoptions_ = std::make_unique<po::options_description>();

	newoptions_->add_options()
		("help,h",											"produce help message")
		("symbol,s",			po::value<std::vector<std::string>>(&this->symbol_list_),	"name of symbol we are processing data for. Repeat for multiple symbols.")
		("symbol-list",			po::value<std::string>(&this->symbol_list_i_),	"Comma-delimited list of symbols to process OR 'ALL' to use all symbols from the specified exchange.")
		("new-data-dir",		po::value<fs::path>(&this->new_data_input_directory_),	"name of directory containing files with new data for symbols we are using.")
		("chart-data-dir",		po::value<fs::path>(&this->input_chart_directory_),	"name of directory containing existing files with data for symbols we are using.")
		("destination",	    	po::value<std::string>(&this->destination_i_)->default_value("file"),	"destination: send data to 'file' or 'database'. Default is 'file'.")
		("new-data-source",		po::value<std::string>(&this->new_data_source_i_)->default_value("file"),	"source for new data: either 'file', 'streaming' or 'database'. Default is 'file'.")
		("chart-data-source",	po::value<std::string>(&this->chart_data_source_i_)->default_value("file"),	"source for existing chart data: either 'file' or 'database'. Default is 'file'.")
		("source-format",		po::value<std::string>(&this->source_format_i_)->default_value("csv"),	"source data format: either 'csv' or 'json'. Default is 'csv'.")
		("graphics-format",		po::value<std::string>(&this->graphics_format_i_)->default_value("svg"),	"Output graphics file format: either 'svg' or 'csv'. Default is 'svg'.")
		("mode,m",				po::value<std::string>(&this->mode_i_)->default_value("load"),	"mode: either 'load' new data, 'update' existing data or 'daily-scan'. Default is 'load'.")
		("interval,i",			po::value<std::string>(&this->interval_i_)->default_value("eod"),	"interval: 'eod', 'live', '1sec', '5sec', '1min', '5min'. Default is 'eod'.")
		("scale",				po::value<std::vector<std::string>>(&this->scale_i_list_),	"scale: 'linear', 'percent'. Default is 'linear'.")
		("price-fld-name",		po::value<std::string>(&this->price_fld_name_)->default_value("Close"),	"price-fld-name: which data field to use for price value. Default is 'Close'.")

		("exchange-list",		po::value<std::string>(&this->exchange_list_i_),	"exchange-list: use symbols from specified exchange(s) for daily-scan and bulk loads from database. Default is: not specified.")
		("min-dollar-volume",	po::value<std::string>(&this->min_dollar_volume_)->default_value("100000"),	"Minimum dollar volue price for a symbol to filter small stocks from daily-scan and bulk loads. Default is $5.00")
		// ("min-close-volume",    po::value<int64_t>(&this->min_close_volume_)->default_value(100'000),	"Minimum closing volume for a symbol to filter small stocks from daily-scan and bulk loads. Default is 100'000")

		("begin-date",			po::value<std::string>(&this->begin_date_),	"Start date for extracting data from database source.")
		("end-date",			po::value<std::string>(&this->end_date_),	"Stop date for extracting data from database source. Default is 'today'.")
		("output-chart-dir",	po::value<fs::path>(&this->output_chart_directory_),	"output directory for chart [and graphic] files.")
		("output-graph-dir",	po::value<fs::path>(&this->output_graphs_directory_),	"name of output directory to write generated graphics to.")
		("boxsize,b",			po::value<std::vector<std::string>>(&this->box_size_i_list_),   	"box step size. 'n', 'm.n'")
		("reversal,r",			po::value<std::vector<int32_t>>(&this->reversal_boxes_list_),		"reversal size in number of boxes.")
		("max-graphic-cols",	po::value<int32_t>(&this->max_columns_for_graph_)->default_value(-1),
									"maximum number of columns to show in graphic. Use -1 for ALL, 0 to keep existing value, if any, otherwise -1. >0 to specify how many columns.")
		("show-trend-lines",	po::value<std::string>(&this->trend_lines_)->default_value("no"),	"Show trend lines on graphic. Can be 'data' or 'angle'. Default is 'no'.")
		("log-path",            po::value<fs::path>(&log_file_path_name_),	"path name for log file.")
		("log-level,l",         po::value<std::string>(&logging_level_)->default_value("information"), "logging level. Must be 'none|error|information|debug'. Default is 'information'.")

        ("streaming-host",      po::value<std::string>(&this->streaming_host_name_)->default_value("ws.eodhistoricaldata.com"), "web site we stream from. Default is 'ws.eodhistoricaldata.com'.")
        ("quote-host",          po::value<std::string>(&this->quote_host_name_)->default_value("api.tiingo.com"), "web site we download from. Default is 'api.tiingo.com'.")
        ("quote-port",          po::value<std::string>(&this->quote_host_port_)->default_value("443"), "Port number to use for web site. Default is '443'.")

        ("db-host",             po::value<std::string>(&this->db_params_.host_name_)->default_value("localhost"), "web location where database is running. Default is 'localhost'.")
        ("db-port",             po::value<int32_t>(&this->db_params_.port_number_)->default_value(5432), "Port number to use for database access. Default is '5432'.")
        ("db-user",             po::value<std::string>(&this->db_params_.user_name_), "Database user name.  Required if using database.")
        ("db-name",             po::value<std::string>(&this->db_params_.db_name_), "Name of database containing PF_Chart data. Required if using database.")
        ("db-mode",             po::value<std::string>(&this->db_params_.PF_db_mode_)->default_value("test"), "'test' or 'live' schema to use. Default is 'test'.")
        ("stock-db-data-source",      po::value<std::string>(&this->db_params_.stock_db_data_source_)->default_value("new_stock_data.current_data"), "table containing symbol data. Default is 'new_stock_data.current_data'.")
        ("streaming-data-source",     po::value<std::string>(&this->streaming_source_i_)->default_value("Eodhd"), "Name of streaming quotes data source. Default is 'Eodhd'.")

        ("tiingo-key",          po::value<fs::path>(&this->tiingo_api_key_)->default_value("./tiingo_key.dat"), "Path to file containing tiingo api key. Default is './tiingo_key.dat'.")
        ("eodhd-key",           po::value<fs::path>(&this->eodhd_api_key_)->default_value("./Eodhd_key.dat"), "Path to file containing Eodhd api key. Default is './Eodhd_key.dat'.")
		("use-ATR",             po::value<bool>(&use_ATR_)->default_value(false)->implicit_value(true), "compute Average True Value and use to compute box size for streaming.")
		("use-MinMax",          po::value<bool>(&use_min_max_)->default_value(false)->implicit_value(true), "compute boxsize using price range from DB then apply specified fraction.")
		;

}		// -----  end of method PF_CollectDataApp::Do_SetupPrograoptions_  -----

// clang-format on

void PF_CollectDataApp::ParseProgramOptions(const std::vector<std::string> &tokens)
{
    if (tokens.empty())
    {
        auto options = po::parse_command_line(argc_, argv_, *newoptions_);
        po::store(options, variablemap_);
        if (this->argc_ == 1 || variablemap_.count("help") == 1)
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

    // std::print("\nRuntime parameters:\n");
    //    for (const auto& [param, value] : variablemap_)
    //    {
    // 	// std::cout << "param: " << param << " value: " <<
    // value.as<std::string>() << '\n'; 	std::print("param: {}. value:
    // .\n", param);
    //    }
} /* -----  end of method ExtractorApp::ParsePrograoptions_  ----- */

std::tuple<int, int, int> PF_CollectDataApp::Run()
{
    if (new_data_source_ != Source::e_DB)
    {
        if (streaming_data_source_ == StreamingSource::e_Tiingo)
        {
            std::ifstream key_file(tiingo_api_key_);
            key_file >> api_key_Tiingo_;
        }
        else if (streaming_data_source_ == StreamingSource::e_Eodhd)
        {
            std::ifstream key_file(eodhd_api_key_);
            key_file >> api_key_Eodhd_;
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
        auto results = Run_DailyScan();
        return results;
    }

    if (new_data_source_ == Source::e_file)
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
    else if (new_data_source_ == Source::e_DB)
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
    return {};
}  // -----  end of method PF_CollectDataApp::Do_Run  -----

void PF_CollectDataApp::Run_Load()
{
    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    // TODO(dpriedel): move file read out of loop and into a buffer

    for (const auto &val : params)
    {
        const auto &symbol = std::get<PF_Chart::e_symbol>(val);
        try
        {
            fs::path symbol_file_name =
                new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
            BOOST_ASSERT_MSG(
                fs::exists(symbol_file_name),
                std::format("\nCan't find data file: {} for symbol: {}.", symbol_file_name, symbol).c_str());
            // TODO(dpriedel): add json code
            BOOST_ASSERT_MSG(source_format_ == SourceFormat::e_csv,
                             "\nJSON files are not yet supported for loading symbol data.");
            auto atr = use_ATR_ ? ComputeATRForChart(symbol) : 0;
            PF_Chart new_chart;
            if (use_ATR_)
            {
                new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
            }
            else
            {
                new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
            }
            AddPriceDataToExistingChartCSV(new_chart, symbol_file_name);
            charts_.emplace_back(std::make_pair(symbol, new_chart));
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Unable to load data for symbol: {} from file because: {}.", symbol, e.what()));
        }
    }
}  // -----  end of method PF_CollectDataApp::Run_Load  -----

std::tuple<int, int, int> PF_CollectDataApp::Run_LoadFromDB()
{
    int32_t total_symbols_processed = 0;
    int32_t total_charts_processed = 0;
    int32_t total_charts_updated = 0;

    // we can handle mass symbol loads because we do a symbol-at-a-time DB query
    // so we don't get an impossible amount of data back from the DB

    if (symbol_list_i_ == "ALL")
    {
        PF_DB pf_db{db_params_};

        exchange_list_ = pf_db.ListExchanges();

        // eliminate some exchanges we don't want to process

        auto dont_use = [](const auto &xchng) { return xchng == "NMFQS" || xchng == "INDX" || xchng == "US"; };
        const auto [first, last] = rng::remove_if(exchange_list_, dont_use);
        exchange_list_.erase(first, last);
        spdlog::debug(fmt::format("exchanges for scan: {}\n", exchange_list_));

        for (const auto &xchng : exchange_list_)
        {
            spdlog::info(std::format("Building charts for symbols on xchng: {} with minimum dollar volume >= {}.",
                                     xchng, min_dollar_volume_));

            auto symbol_list = pf_db.ListSymbolsOnExchange(xchng, min_dollar_volume_);
            const auto counts = ProcessSymbolsFromDB(symbol_list);
            total_symbols_processed += std::get<0>(counts);
            total_charts_processed += std::get<1>(counts);
            total_charts_updated += std::get<2>(counts);
            spdlog::info(
                std::format("Exchange: {}. Symbols: {}. Charts scanned: {}. Charts built: "
                            "{}.",
                            xchng, std::get<0>(counts), std::get<1>(counts), std::get<2>(counts)));
        }
    }
    else
    {
        const auto counts = ProcessSymbolsFromDB(symbol_list_);
        total_symbols_processed += std::get<0>(counts);
        total_charts_processed += std::get<1>(counts);
        total_charts_updated += std::get<2>(counts);
    }
    // std::print("symbol list: {}\n", symbol_list_);

    spdlog::info(
        std::format("Total symbols: {}. Total charts generated: {}. Total charts built: "
                    "{}.",
                    total_symbols_processed, total_charts_processed, total_charts_updated));

    return {total_symbols_processed, total_charts_processed, total_charts_updated};
}  // -----  end of method PF_CollectDataApp::Run_Load  -----

std::tuple<int, int, int> PF_CollectDataApp::ProcessSymbolsFromDB(const std::vector<std::string> &symbol_list)
{
    int32_t total_symbols_processed = 0;
    int32_t total_charts_processed = 0;
    int32_t total_charts_updated = 0;

    PF_DB pf_db{db_params_};

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};

    const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";

    std::istringstream time_stream;
    date::utc_time<std::chrono::utc_clock::duration> tp;

    // we know our database contains 'date's, but we need timepoints.
    // we'll handle that in the conversion routine below.

    auto Row2Closing = [dt_format, &time_stream, &tp](const auto &r)
    {
        time_stream.clear();
        time_stream.str(std::string{std::get<0>(r)});
        date::from_stream(time_stream, dt_format, tp);
        std::chrono::utc_time<std::chrono::utc_clock::duration> tp1{tp.time_since_epoch()};
        DateCloseRecord new_data{.date_ = tp1, .close_ = Decimal{std::get<1>(r)}};
        return new_data;
    };

    for (const auto &symbol : symbol_list)
    {
        ++total_symbols_processed;

        try
        {
            // first, get ready to retrieve our data from DB.  Do this once per
            // symbol.

            std::string get_symbol_prices_cmd = std::format(
                "SELECT date, {} FROM {} WHERE symbol = {} AND date >= "
                "{} ORDER BY date ASC",
                price_fld_name_, db_params_.stock_db_data_source_, c.quote(symbol), c.quote(begin_date_));

            const auto closing_prices = pf_db.RunSQLQueryUsingStream<DateCloseRecord, std::string_view, const char *>(
                get_symbol_prices_cmd, Row2Closing);

            // only need to compute this once per symbol also
            auto atr_or_range = use_ATR_       ? ComputeATRForChartFromDB(symbol)
                                : use_min_max_ ? pf_db.ComputePriceRangeForSymbolFromDB(symbol, begin_date_, end_date_)
                                               : 0;

            // There could be thousands of symbols in the database so we don't
            // want to generate combinations for all of them at once. so, make a
            // single element list for the call below and then generate the
            // other combinations.

            std::vector<std::string> the_symbol{symbol};
            auto params = vws::cartesian_product(the_symbol, box_size_list_, reversal_boxes_list_, scale_list_);
            // ranges::for_each(params, [](const auto& x) {std::print("{}\n",
            // x); });

            for (const auto &val : params)
            {
                PF_Chart new_chart;
                if (use_ATR_ || use_min_max_)
                {
                    new_chart = PF_Chart{atr_or_range, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
                else
                {
                    new_chart = PF_Chart{val, atr_or_range, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
                try
                {
                    for (const auto &[new_date, new_price] : closing_prices)
                    {
                        // std::cout << "new value: " << new_price << "\t" <<
                        // new_date << std::endl;
                        new_chart.AddValue(new_price, std::chrono::clock_cast<std::chrono::utc_clock>(new_date));
                    }
                    charts_.emplace_back(std::make_pair(symbol, new_chart));
                    ++total_charts_processed;
                }
                catch (const std::exception &e)
                {
                    spdlog::error(
                        std::format("Unable to load data for symbol chart: {} from DB "
                                    "because: {}.",
                                    new_chart.MakeChartFileName(interval_i_, ""), e.what()));
                }
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Unable to retrieve data for symbol: {} from DB because: {}.", symbol, e.what()));
        }
    }
    return {total_symbols_processed, total_charts_processed, total_charts_updated};
}  // -----  end of method PF_CollectDataApp::ProcessSymbolsFromDB  -----

void PF_CollectDataApp::Run_Update()
{
    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    // look for existing data and load the saved JSON data if we have it.
    // then add the new data to the chart.

    for (const auto &val : params)
    {
        const auto &symbol = std::get<PF_Chart::e_symbol>(val);
        PF_Chart new_chart;
        try
        {
            fs::path existing_data_file_name =
                input_chart_directory_ / MakeChartNameFromParams(val, interval_i_, "json");
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

                auto atr = use_ATR_ ? ComputeATRForChart(symbol) : 0;
                if (use_ATR_)
                {
                    new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
                else
                {
                    new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
            }
            fs::path update_file_name =
                new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
            BOOST_ASSERT_MSG(
                fs::exists(update_file_name),
                std::format("\nCan't find data file for symbol: {} for update.", update_file_name).c_str());
            // TODO(dpriedel): add json code
            BOOST_ASSERT_MSG(source_format_ == SourceFormat::e_csv,
                             "\nJSON files are not yet supported for updating symbol data.");
            AddPriceDataToExistingChartCSV(new_chart, update_file_name);
            charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Unable to update data for chart: {} from file because: {}.",
                                      new_chart.MakeChartFileName(interval_i_, ""), e.what()));
        }
    }
}  // -----  end of method PF_CollectDataApp::Run_Update  -----

void PF_CollectDataApp::Run_UpdateFromDB()
{
    // if we are down this path then the expectation is that we are processing a
    // 'short' list of symbols and a 'recent' date so we will be collecting a
    // 'small' amount of data from the DB.  In this case, we can do it all up
    // front

    // look for existing data and load the saved JSON data if we have it.
    // then add the new data to the chart.

    PF_DB pf_db{db_params_};

    const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";
    auto db_data = pf_db.GetPriceDataForSymbolsInList(symbol_list_, begin_date_, end_date_, price_fld_name_, dt_format);
    // ranges::for_each(db_data, [](const auto& xx) {std::print("{}, {}, {}\n",
    // xx.symbol, xx.tp, xx.price); });

    // our data from the DB is grouped by symbol so we split it into sub-ranges
    // by symbol below.

    auto data_for_symbol = vws::chunk_by([](const auto &a, const auto &b) { return a.symbol_ == b.symbol_; });

    // then we process each sub-range and apply the data for each symbol to all
    // PF_Chart variants that were asked for.

    for (const auto &symbol_rng : db_data | data_for_symbol)
    {
        const auto &symbol = symbol_rng[0].symbol_;
        // std::print("symbol: {}\n", symbol);
        std::vector<std::string> the_symbol{symbol};

        auto params = vws::cartesian_product(the_symbol, box_size_list_, reversal_boxes_list_, scale_list_);

        for (const auto &val : params)
        {
            PF_Chart new_chart;
            try
            {
                if (chart_data_source_ == Source::e_file)
                {
                    fs::path existing_data_file_name =
                        input_chart_directory_ / MakeChartNameFromParams(val, interval_i_, "json");
                    if (fs::exists(existing_data_file_name))
                    {
                        new_chart = LoadAndParsePriceDataJSON(existing_data_file_name);
                        if (max_columns_for_graph_ != 0)
                        {
                            new_chart.SetMaxGraphicColumns(max_columns_for_graph_);
                        }
                    }
                }
                else  // should only be database here
                {
                    new_chart = PF_Chart::LoadChartFromChartsDB(PF_DB{db_params_}, val, interval_i_);
                }
                if (new_chart.empty())
                {
                    // no existing data to update, so make a new chart

                    auto atr = use_ATR_ ? ComputeATRForChartFromDB(symbol) : 0;
                    if (use_ATR_)
                    {
                        new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                    }
                    else
                    {
                        new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                    }
                }

                // apply new data to chart (which may be empty)

                rng::for_each(symbol_rng, [&new_chart](const auto &row) { new_chart.AddValue(row.close_, row.date_); });

                charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
            }
            catch (const std::exception &e)
            {
                spdlog::error(std::format("Unable to update data for chart: {} from DB because: {}.",
                                          new_chart.MakeChartFileName(interval_i_, ""), e.what()));
            }
        }
    }
}  // -----  end of method PF_CollectDataApp::Run_UpdateFromDB  -----

void PF_CollectDataApp::Run_Streaming()
{
    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    auto current_local_time = std::chrono::zoned_seconds(std::chrono::current_zone(),
                                                         floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    auto market_status =
        GetUS_MarketStatus(std::string_view{std::chrono::current_zone()->name()}, current_local_time.get_local_time());

    if (market_status != US_MarketStatus::e_NotOpenYet && market_status != US_MarketStatus::e_OpenForTrading)
    {
        std::cout << "Market not open for trading now so we can't stream quotes.\n";
        return;
    }

    if (market_status == US_MarketStatus::e_NotOpenYet)
    {
        std::cout << "Market not open for trading YET so we'll wait." << std::endl;
    }

    // initialize PF_Charts to be used by streaming code

    std::map<std::string, decimal::Decimal> cache;  // table for memoization of ATR

    for (const auto &val : params)
    {
        const auto &symbol = std::get<PF_Chart::e_symbol>(val);
        try
        {
            PF_Chart new_chart;
            // compute ATR once per symbol
            decimal::Decimal atr;
            if (use_ATR_)
            {
                atr = cache.contains(symbol) ? cache[symbol] : (cache[symbol] = ComputeATRForChart(symbol));
                new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
            }
            else
            {
                atr = 0;
                new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
            }
            charts_.emplace_back(std::make_pair(symbol, new_chart));
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Unable to compute ATR for: '{}' because: {}.\n", symbol, e.what()));
        }
    }

    // setup to capture streamed price data and price movement summary too

    for (const auto &symbol : symbol_list_)
    {
        streamed_prices_[symbol] = {};
        streamed_summary_[symbol] = {};
    }

    // let's stream !

    PrimeChartsForStreaming();

    CollectStreamingData();

}  // -----  end of method PF_CollectDataApp::Run_Streaming  -----

void PF_CollectDataApp::AddPriceDataToExistingChartCSV(PF_Chart &new_chart, const fs::path &update_file_name) const
{
    const std::string file_content = LoadDataFileForUse(update_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content, "\n");
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ",");
    BOOST_ASSERT_MSG(date_column.has_value(),
                     std::format("\nCan't find 'date' field in header record: {}.", header_record).c_str());

    auto close_column = FindColumnIndex(header_record, price_fld_name_, ",");
    BOOST_ASSERT_MSG(
        close_column.has_value(),
        std::format("\nCan't find price field: {} in header record: {}.", price_fld_name_, header_record).c_str());

    rng::for_each(
        symbol_data_records | vws::drop(1),
        [this, &new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record)
        {
            const auto fields = split_string<std::string_view>(record, ",");
            const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";
            new_chart.AddValue(sv2dec(fields[close_col]), StringToUTCTimePoint(dt_format, fields[date_col]));
        });

}  // -----  end of method PF_CollectDataApp::AddPriceDataToExistingChartCSV
   // -----

PF_Chart PF_CollectDataApp::LoadAndParsePriceDataJSON(const fs::path &symbol_file_name)
{
    PF_Chart new_chart = PF_Chart::LoadChartFromJSONChartFile(symbol_file_name);
    return new_chart;
}  // -----  end of method PF_CollectDataApp::LoadAndParsePriceDataJSON  -----

std::optional<int> PF_CollectDataApp::FindColumnIndex(std::string_view header, std::string_view column_name,
                                                      std::string_view delim)
{
    auto fields = rng_split_string<std::string_view>(header, delim) | ranges::to<std::vector>();
    auto do_compare(
        [&column_name](const auto &field_name)
        {
            // need case insensitive compare
            // found this on StackOverflow (but modified for my use)
            // (https://stackoverflow.com/questions/11635/case-insensitive-string-comparison-in-c)

            if (column_name.size() != field_name.size())
            {
                return false;
            }
            return rng::equal(column_name, field_name,
                              [](unsigned char a, unsigned char b) { return tolower(a) == tolower(b); });
        });

    if (auto found_it = rng::find_if(fields, do_compare); found_it != rng::end(fields))
    {
        return rng::distance(fields.begin(), found_it);
    }
    return {};

}  // -----  end of method PF_CollectDataApp::FindColumnIndex  -----

Decimal PF_CollectDataApp::ComputeATRForChart(const std::string &symbol) const
{
    std::unique_ptr<RemoteDataSource> history_getter;
    if (streaming_data_source_ == StreamingSource::e_Eodhd)
    {
        history_getter = std::make_unique<Eodhd>(Eodhd::Host{quote_host_name_}, Eodhd::Port{quote_host_port_},
                                                 Eodhd::APIKey{api_key_Eodhd_}, Eodhd::Prefix{});
    }
    else
    {
        // just 2 options for now
        history_getter = std::make_unique<Tiingo>(Tiingo::Host{quote_host_name_}, Tiingo::Port{quote_host_port_},
                                                  Tiingo::APIKey{api_key_Tiingo_}, Tiingo::Prefix{});
    }

    // we need to start from yesterday since we won't get history data for today
    // since we are doing this while the market is open

    std::chrono::year_month_day today{--floor<std::chrono::days>(std::chrono::system_clock::now())};

    // use our new holidays capability
    // we look backwards here. so add an extra year in case we are near New
    // Years.

    auto holidays = MakeHolidayList(today.year());
    rng::copy(MakeHolidayList(--(today.year())), std::back_inserter(holidays));

    const auto history = history_getter->GetMostRecentTickerData(symbol, today, number_of_days_history_for_ATR_ + 1,
                                                                 UseAdjusted::e_Yes, &holidays);

    auto atr = ComputeATR(symbol, history, number_of_days_history_for_ATR_);

    return atr;
}  // -----  end of method PF_CollectDataApp::ComputeBoxSizeUsingATR  -----

Decimal PF_CollectDataApp::ComputeATRForChartFromDB(const std::string &symbol) const
{
    PF_DB the_db{db_params_};

    Decimal atr{};
    try
    {
        auto price_data =
            the_db.RetrieveMostRecentStockDataRecordsFromDB(symbol, end_date_, number_of_days_history_for_ATR_ + 1);
        atr = ComputeATR(symbol, price_data, number_of_days_history_for_ATR_);
    }
    catch (const std::exception &e)
    {
        spdlog::error(std::format("Unable to compute ATR from DB for: '{}' because: {}.\n", symbol, e.what()));
    }

    return atr;
}  // -----  end of method PF_CollectDataApp::ComputeATRUsingDB  -----

void PF_CollectDataApp::PrimeChartsForStreaming()
{
    // for streaming, we want to retrieve the previous day's close and, if the
    // markets are already open, the day's open.  We do this to capture 'gaps'
    // and to set the direction at little sooner.

    auto today = std::chrono::year_month_day{floor<std::chrono::days>(std::chrono::system_clock::now())};
    std::chrono::year which_year = today.year();
    auto holidays = MakeHolidayList(which_year);
    rng::copy(MakeHolidayList(--which_year), std::back_inserter(holidays));

    auto current_local_time = std::chrono::zoned_seconds(std::chrono::current_zone(),
                                                         floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    auto market_status =
        GetUS_MarketStatus(std::string_view{std::chrono::current_zone()->name()}, current_local_time.get_local_time());

    std::unique_ptr<RemoteDataSource> history_getter;
    if (streaming_data_source_ == StreamingSource::e_Eodhd)
    {
        history_getter = std::make_unique<Eodhd>(Eodhd::Host{quote_host_name_}, Eodhd::Port{quote_host_port_},
                                                 Eodhd::APIKey{api_key_Eodhd_}, Eodhd::Prefix{});
    }
    else
    {
        // just 2 options for now
        history_getter = std::make_unique<Tiingo>(Tiingo::Host{quote_host_name_}, Tiingo::Port{quote_host_port_},
                                                  Tiingo::APIKey{api_key_Tiingo_}, Tiingo::Prefix{});
    }

    if (market_status == US_MarketStatus::e_NotOpenYet)
    {
        // just prior day's close

        std::map<std::string, std::vector<StockDataRecord>> cache;

        for (auto &[symbol, chart] : charts_)
        {
            const auto history =
                cache.contains(symbol)
                    ? cache[symbol]
                    : (cache[symbol] = history_getter->GetMostRecentTickerData(
                           symbol, today, 2,
                           price_fld_name_.starts_with("adj") ? UseAdjusted::e_Yes : UseAdjusted::e_No, &holidays));
            chart.AddValue(history[0].close_,
                           std::chrono::clock_cast<std::chrono::utc_clock>(current_local_time.get_sys_time()));
        }
        // initialize our streaming summary 'opening' price (really prior day's close)

        for (const auto &[symbol, h] : cache)
        {
            try
            {
                // all we've got at this poiint is yesterday's close
                streamed_summary_[symbol].opening_price_ = dec2dbl(h[0].close_);
            }
            catch (const std::exception &e)
            {
                spdlog::error(
                    std::format("Problem initializing streamed summary with streaming data for symbol: {} because: {}",
                                symbol, e.what()));
            }
        }
    }
    else if (market_status == US_MarketStatus::e_OpenForTrading)
    {
        history_getter->UseSymbols(symbol_list_);
        auto history = history_getter->GetTopOfBookAndLastClose();

        const auto close_time_stamp = std::chrono::clock_cast<std::chrono::utc_clock>(
            GetUS_MarketOpenTime(today).get_sys_time() - std::chrono::seconds{60});
        const auto open_time_stamp =
            std::chrono::clock_cast<std::chrono::utc_clock>(GetUS_MarketOpenTime(today).get_sys_time());

        for (const auto &h : history)
        {
            rng::for_each(
                charts_ | vws::filter([&h](auto &symbol_and_chart) { return symbol_and_chart.first == h.symbol_; }),
                [&](auto &symbol_and_chart)
                {
                    try
                    {
                        symbol_and_chart.second.AddValue(h.previous_close_, close_time_stamp);
                        // Eodhd runs 15-20 minutes delayed so it may no know open and last yet.
                        // for now, just skip these steps.
                        // But Tiingo will have values.

                        if (h.open_ != 0)
                        {
                            symbol_and_chart.second.AddValue(h.open_, open_time_stamp);
                            symbol_and_chart.second.AddValue(h.last_, h.time_stamp_nsecs_);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        spdlog::error(
                            std::format("Problem initializing PF_Chart with streaming data for symbol: {} because: {}",
                                        h.symbol_, e.what()));
                    }
                });
        }
        // initialize our streaming summary 'opening' price (really prior day's close)

        for (const auto &h : history)
        {
            try
            {
                streamed_summary_[h.symbol_].opening_price_ = dec2dbl(h.previous_close_);
                // again, Eodhd might not have data

                if (h.last_ == 0)
                {
                    streamed_summary_[h.symbol_].latest_price_ = dec2dbl(h.previous_close_);
                }
                else
                {
                    streamed_summary_[h.symbol_].latest_price_ = dec2dbl(h.last_);
                }
            }
            catch (const std::exception &e)
            {
                spdlog::error(
                    std::format("Problem initializing streamed summary with streaming data for symbol: {} because: {}",
                                h.symbol_, e.what()));
            }
        }
    }
}  // -----  end of method PF_CollectDataApp::PrimeChartsForStreaming  -----

void PF_CollectDataApp::CollectStreamingData()
{
    // we're going to use 2 threads here -- a producer thread which collects
    // streamed data from the streaming source and a consummer thread which will take that
    // data, decode it and load it into appropriate charts. Processing continues
    // until interrupted.

    // we've added a third thread to manage a countdown timer which will
    // interrupt our producer thread at market close.  Otherwise, the producer
    // thread will hang forever or until interrupted.

    // since this code can potentially run for hours on end
    // it's a good idea to provide a way to break into this processing and shut
    // it down cleanly. so, a little bit of C...(taken from "Advanced Unix
    // Programming" by Warren W. Gay, p. 317)

    // ok, get ready to handle keyboard interrupts, if any.

    std::cout << std::format("starting {} streaming.\n",
                             streaming_data_source_ == StreamingSource::e_Eodhd ? "Eodhd" : "Tiingo");

    struct sigaction sa_old
    {
    };
    struct sigaction sa_new
    {
    };

    sa_new.sa_handler = PF_CollectDataApp::HandleSignal;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
    sigaction(SIGINT, &sa_new, &sa_old);

    PF_CollectDataApp::had_signal_ = false;

    // if we are here then we already know that the US market is open for
    // trading.

    auto today = std::chrono::year_month_day{floor<std::chrono::days>(std::chrono::system_clock::now())};
    // add a couple minutes for padding
    auto local_market_close =
        std::chrono::zoned_seconds(std::chrono::current_zone(), GetUS_MarketCloseTime(today).get_sys_time() + 2min);

    std::mutex data_mutex;
    std::queue<std::string> streamed_data;

    // py::gil_scoped_release gil{};

    auto timer_task = std::async(std::launch::async, &PF_CollectDataApp::WaitForTimer, local_market_close);
    auto processing_task = std::async(std::launch::async, &PF_CollectDataApp::ProcessStreamedData, this,
                                      &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);
    while (!had_signal_)
    {
        try
        {
            std::unique_ptr<RemoteDataSource> quotes;
            if (streaming_data_source_ == StreamingSource::e_Eodhd)
            {
                quotes = std::make_unique<Eodhd>(Eodhd::Host{streaming_host_name_}, Eodhd::Port{quote_host_port_},
                                                 Eodhd::APIKey{api_key_Eodhd_},
                                                 Eodhd::Prefix{"/ws/us?api_token="s + api_key_Eodhd_});
            }
            else
            {
                // just 2 options for now
                quotes = std::make_unique<Tiingo>(Tiingo::Host{streaming_host_name_}, Tiingo::Port{quote_host_port_},
                                                  Tiingo::APIKey{api_key_Tiingo_}, Tiingo::Prefix{"/iex"});
            }

            quotes->UseSymbols(symbol_list_);

            auto streaming_task = std::async(std::launch::async, &RemoteDataSource::StreamData, quotes.get(),
                                             &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);
            // auto streaming_task = std::async(std::launch::async, &Eodhd::StreamData, &quotes,
            //                                  &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);
            streaming_task.get();
        }
        catch (RemoteDataSource::StreamingEOF &e)
        {
            // if we got an EOF on the stream, just start up again
            // unless, we had a signal.

            spdlog::info("Caught 'StreamingEOF'. Trying to continue.");

            continue;
        }
        catch (std::exception &e)
        {
            spdlog::error(std::format("Problem with {} streaming. Message: {}",
                                      streaming_data_source_ == StreamingSource::e_Eodhd ? "Eodhd" : "Tiingo",
                                      e.what()));
            had_signal_ = true;
        }
    }

    processing_task.get();
    timer_task.get();

    // make a last check to be sure we  didn't leave any data unprocessed

    ProcessStreamedData(&PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);

}  // -----  end of method PF_CollectDataApp::CollectStreamingData  -----

void PF_CollectDataApp::ProcessStreamedData(bool *had_signal, std::mutex *data_mutex,
                                            std::queue<std::string> *streamed_data)
{
    //    py::gil_scoped_acquire gil{};
    std::exception_ptr ep = nullptr;

    std::unique_ptr<RemoteDataSource> streamer;
    if (streaming_data_source_ == StreamingSource::e_Eodhd)
    {
        streamer = std::make_unique<Eodhd>(Eodhd::Host{streaming_host_name_}, Eodhd::Port{quote_host_port_},
                                           Eodhd::APIKey{api_key_Eodhd_},
                                           Eodhd::Prefix{"/ws/us?api_token="s + api_key_Eodhd_});
    }
    else
    {
        // just 2 options for now
        streamer = std::make_unique<Tiingo>(Tiingo::Host{streaming_host_name_}, Tiingo::Port{quote_host_port_},
                                            Tiingo::APIKey{api_key_Tiingo_}, Tiingo::Prefix{"/iex"});
    }
    while (true)
    {
        if (!streamed_data->empty())
        {
            std::string new_data;
            {
                const std::lock_guard<std::mutex> queue_lock(*data_mutex);
                new_data = streamed_data->front();
                streamed_data->pop();
            }
            const auto pf_data = streamer->ExtractStreamedData(new_data);

            // our PF_Data contains data for just 1 transaction for 1 symbol
            try
            {
                ProcessUpdatesForSymbol(pf_data);
            }
            catch (std::system_error &e)
            {
                // any system problems, we eventually abort, but only
                // after finishing work in process.

                spdlog::error(e.what());
                auto ec = e.code();
                spdlog::error("Category: {}. Value: {}. Message: {}.", ec.category().name(), ec.value(), ec.message());

                // OK, let's remember our first time here.

                if (!ep)
                {
                    ep = std::current_exception();
                }
                continue;
            }
            catch (std::exception &e)
            {
                // any problems, we'll document them and continue.

                spdlog::error(e.what());

                if (!ep)
                {
                    ep = std::current_exception();
                }
                continue;
            }
            catch (...)
            {
                // any problems, we'll document them and continue.

                spdlog::error("Unknown problem with an async download process");

                if (!ep)
                {
                    ep = std::current_exception();
                }
                continue;
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
    if (ep)
    {
        // spdlog::error(catenate("Processed: ", file_list.size(), " files.
        // Successes: ", success_counter,
        //         ". Errors: ", error_counter, "."));
        *had_signal = true;
        std::rethrow_exception(ep);
    }

}  // -----  end of method PF_CollectDataApp::ProcessEodhdStreamedData  -----

void PF_CollectDataApp::ProcessUpdatesForSymbol(const RemoteDataSource::PF_Data &update)
{
    // it is possible that the update could be empty if there was a problem extracting
    // the data.
    // Also, for now, we want to skip 'dark pool' transactions.
    // AND skip 1-share transactions -- maybe these are algo-traders probing the market.

    if (update.last_price_ == -1 || update.dark_pool_ || update.last_size_ == 1)
    {
        // last_price_ = -1 means the field was not set
        // during data extraction.
        return;
    }

    std::vector<PF_Chart *> need_to_update_graph;
    PF_SignalType new_signal{PF_SignalType::e_unknown};

    // since we can have multiple charts for each symbol, we need to pass the
    // new value to all appropriate charts so we find all the charts for each
    // symbol and give each a chance at the new data.

    rng::for_each(
        charts_ |
            vws::filter([&update](const auto &symbol_and_chart) { return symbol_and_chart.first == update.ticker_; }),
        [this, &need_to_update_graph, &update, &new_signal](auto &symbol_and_chart)
        {
            try
            {
                auto chart_changed =
                    symbol_and_chart.second.AddValue(update.last_price_, update.time_stamp_nanoseconds_utc_);
                if (chart_changed != PF_Column::Status::e_Ignored)
                {
                    need_to_update_graph.push_back(&symbol_and_chart.second);
                    if (chart_changed == PF_Column::Status::e_AcceptedWithSignal)
                    {
                        new_signal = symbol_and_chart.second.GetMostRecentSignal().value().signal_type_;
                    }
                }
            }
            catch (std::exception &e)
            {
                spdlog::error(std::format("Problem adding streamed value to chart for symbol: {} because: {}.",
                                          update.ticker_, e.what()));
            }
        });

    // we only need to collect this data once per symbol.
    // we'll share it when we do graphics for each PF_Chart.

    CollectStreamedData(update, new_signal);

    // we could have multiple chart updates for any given symbol but we only
    // want to update files and graphic once per symbol.

    rng::sort(need_to_update_graph);
    const auto [first, last] = rng::unique(need_to_update_graph);
    need_to_update_graph.erase(first, last);

    for (const PF_Chart *chart : need_to_update_graph)
    {
        try
        {
            fs::path graph_file_path = output_graphs_directory_ / (chart->MakeChartFileName("", "svg"));
            ConstructCDPFChartGraphicAndWriteToFile(*chart, graph_file_path, streamed_prices_[chart->GetSymbol()],
                                                    trend_lines_, PF_Chart::X_AxisFormat::e_show_time);

            fs::path chart_file_path = output_chart_directory_ / (chart->MakeChartFileName("", "json"));
            chart->ConvertChartToJsonAndWriteToFile(chart_file_path);
        }
        catch (std::exception &e)
        {
            spdlog::error("Problem creating graphic for updated streamed value: "s += chart->GetChartBaseName() +=
                          " "s += e.what());
        }
    }

    fs::path summary_graphic_path = output_graphs_directory_ / "PF_StreamingSummary.svg";
    ConstructCDSummaryGraphic(streamed_summary_, summary_graphic_path);
}  // -----  end of method PF_CollectDataApp::ProcessUpdatesForEodhdSymbol  -----

void PF_CollectDataApp::CollectStreamedData(const RemoteDataSource::PF_Data &update, PF_SignalType new_signal)
{
    // we get streamed data at the millisecond resolution.  This is too much to
    // show on a graphic. So, we filter to the second and keep the last value
    // for each second.

    const auto new_time_stamp =
        std::chrono::duration_cast<std::chrono::seconds>(update.time_stamp_nanoseconds_utc_.time_since_epoch()).count();
    if (!streamed_prices_[update.ticker_].timestamp_seconds_.empty())
    {
        if (new_time_stamp > streamed_prices_[update.ticker_].timestamp_seconds_.back())
        {
            streamed_prices_[update.ticker_].timestamp_seconds_.push_back(new_time_stamp);
            streamed_prices_[update.ticker_].price_.push_back(dec2dbl(update.last_price_));
            streamed_prices_[update.ticker_].signal_type_.push_back(std::to_underlying(new_signal));
        }
        else
        {
            // we just update our previous value for this second

            streamed_prices_[update.ticker_].price_.back() = dec2dbl(update.last_price_);
            if (new_signal != PF_SignalType::e_unknown)
            {
                streamed_prices_[update.ticker_].signal_type_.back() = std::to_underlying(new_signal);
            }
        }
    }
    else
    {
        streamed_prices_[update.ticker_].timestamp_seconds_.push_back(new_time_stamp);
        streamed_prices_[update.ticker_].price_.push_back(dec2dbl(update.last_price_));
        streamed_prices_[update.ticker_].signal_type_.push_back(std::to_underlying(new_signal));
    }

    // simple update for summary

    streamed_summary_[update.ticker_].latest_price_ = dec2dbl(update.last_price_);

}  // -----  end of method PF_CollectDataApp::CollectEodhdStreamedData  -----

std::tuple<int, int, int> PF_CollectDataApp::Run_DailyScan()
{
    // I expect this will be run fairly often so that the amount of data
    // retrieved from the stock price DB will be manageable so I will just do
    // that qeury up front and then process that data as the main loop. NOTE: I
    // will, however, segment the data by exchange;

    int32_t total_symbols_processed = 0;
    int32_t total_charts_processed = 0;
    int32_t total_charts_updated = 0;

    PF_DB pf_db{db_params_};
    const auto *dt_format = "%F";

    if (exchange_list_.empty())
    {
        exchange_list_ = pf_db.ListExchanges();

        // eliminate some exchanges we don't want to process

        auto dont_use = [](const auto &xchng) { return xchng == "NMFQS" || xchng == "INDX" || xchng == "US"; };
        const auto [first, last] = rng::remove_if(exchange_list_, dont_use);
        exchange_list_.erase(first, last);
    }
    spdlog::debug(fmt::format("exchanges for scan: {}\n", exchange_list_));

    // our data from the DB is grouped by symbol so we split it into sub-ranges
    // by symbol below.

    auto data_for_symbol = vws::chunk_by([](const auto &a, const auto &b) { return a.symbol_ == b.symbol_; });

    for (const auto &xchng : exchange_list_)
    {
        spdlog::info(std::format("Scanning charts for symbols on xchng: {} with adjusted dollar volume >= {}.", xchng,
                                 min_dollar_volume_));

        int32_t exchange_symbols_processed = 0;
        int32_t exchange_charts_processed = 0;
        int32_t exchange_charts_updated = 0;

        auto db_data = pf_db.GetPriceDataForSymbolsOnExchange(xchng, begin_date_, end_date_, price_fld_name_, dt_format,
                                                              min_dollar_volume_);
        // ranges::for_each(db_data, [](const auto& xx) {std::print("{}, {},
        // {}\n", xx.symbol, xx.tp, xx.price); });

        // then we process each sub-range and apply the data for each symbol to
        // all PF_Chart variants that we find in the DB for that symbol.

        for (const auto &symbol_rng : db_data | data_for_symbol)
        {
            const auto &symbol = symbol_rng[0].symbol_;
            exchange_symbols_processed += 1;

            // std::print("symbol: {}\n", symbol);

            auto charts_for_symbol = pf_db.RetrieveAllEODChartsForSymbol(symbol);

            for (auto &chart : charts_for_symbol)
            {
                // apply new data to chart (which may be empty)

                exchange_charts_processed += 1;
                bool chart_needs_update = false;
                try
                {
                    rng::for_each(symbol_rng,
                                  [&chart, &chart_needs_update](const auto &row)
                                  {
                                      auto status = chart.AddValue(row.close_, row.date_);
                                      chart_needs_update |= status == PF_Column::Status::e_Accepted ? 1 : 0;
                                  });
                    if (chart_needs_update)
                    {
                        // we are only doing EOD charts in this routine.
                        chart.UpdateChartInChartsDB(pf_db, interval_i_, PF_Chart::X_AxisFormat::e_show_date,
                                                    graphics_format_ == GraphicsFormat::e_csv);
                        exchange_charts_updated += 1;
                    }
                }
                catch (const std::exception &e)
                {
                    spdlog::error(
                        std::format("Unable to update data for chart: {} from DB because: "
                                    "{}.",
                                    chart.MakeChartFileName(interval_i_, ""), e.what()));
                }
            }
        }

        total_symbols_processed += exchange_symbols_processed;
        total_charts_processed += exchange_charts_processed;
        total_charts_updated += exchange_charts_updated;
        spdlog::info(
            std::format("Exchange: {}. Symbols: {}. Charts scanned: {}. Charts updated: "
                        "{}.",
                        xchng, exchange_symbols_processed, exchange_charts_processed, exchange_charts_updated));
    }

    spdlog::info(
        std::format("Total symbols: {}. Total charts scanned: {}. Total charts updated: "
                    "{}.",
                    total_symbols_processed, total_charts_processed, total_charts_updated));

    return {total_symbols_processed, total_charts_processed, total_charts_updated};

}  // -----  end of method PF_CollectDataApp::Run_DailyScan  -----

void PF_CollectDataApp::Shutdown()
{
    // py::gil_scoped_acquire gil{};

    if (destination_ == Destination::e_file)
    {
        ShutdownAndStoreOutputInFiles();
    }
    else
    {
        ShutdownAndStoreOutputInDB();
    }

    spdlog::info(std::format("\n\n*** End run {}  ***\n",
                             std::chrono::current_zone()->to_local(std::chrono::system_clock::now())));
}  // -----  end of method PF_CollectDataApp::Shutdown  -----

void PF_CollectDataApp::ShutdownAndStoreOutputInFiles()
{
    for (const auto &[symbol, chart] : charts_)
    {
        try
        {
            fs::path output_file_name =
                output_chart_directory_ /
                chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "json");
            chart.ConvertChartToJsonAndWriteToFile(output_file_name);

            if (graphics_format_ == GraphicsFormat::e_svg)
            {
                fs::path graph_file_path =
                    output_graphs_directory_ /
                    (chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "svg"));
                ConstructCDPFChartGraphicAndWriteToFile(
                    chart, graph_file_path,
                    (new_data_source_ == Source::e_streaming ? streamed_prices_[chart.GetSymbol()] : StreamedPrices{}),
                    trend_lines_,
                    interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time
                                                 : PF_Chart::X_AxisFormat::e_show_date);
            }
            else
            {
                fs::path graph_file_path =
                    output_graphs_directory_ /
                    (chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "csv"));
                chart.ConvertChartToTableAndWriteToFile(graph_file_path, interval_ != Interval::e_eod
                                                                             ? PF_Chart::X_AxisFormat::e_show_time
                                                                             : PF_Chart::X_AxisFormat::e_show_date);
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format(
                "Problem in shutdown: {} for chart: {}.\nTrying to "
                "complete "
                "shutdown.",
                e.what(), chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "")));
        }
    }
}  // -----  end of method PF_CollectDataApp::ShutdownStoreOutputInFiles  -----

void PF_CollectDataApp::ShutdownAndStoreOutputInDB()
{
    int32_t chart_count = 0;
    PF_DB pf_db{db_params_};
    for (const auto &[symbol, chart] : charts_)
    {
        try
        {
            if (graphics_format_ == GraphicsFormat::e_svg)
            {
                fs::path graph_file_path = output_graphs_directory_ / (chart.MakeChartFileName(interval_i_, "svg"));
                ConstructCDPFChartGraphicAndWriteToFile(
                    chart, graph_file_path,
                    (new_data_source_ == Source::e_streaming ? streamed_prices_[chart.GetSymbol()] : StreamedPrices{}),
                    trend_lines_,
                    interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time
                                                 : PF_Chart::X_AxisFormat::e_show_date);
            }
            chart.StoreChartInChartsDB(pf_db, interval_i_,
                                       interval_ != Interval::e_eod ? PF_Chart::X_AxisFormat::e_show_time
                                                                    : PF_Chart::X_AxisFormat::e_show_date,
                                       graphics_format_ == GraphicsFormat::e_csv);
            ++chart_count;
        }
        catch (const std::exception &e)
        {
            spdlog::error(
                std::format("Problem storing data in DB in shutdown: {} for chart: "
                            "{}.\nTrying to complete shutdown.",
                            e.what(), chart.MakeChartFileName(interval_i_, "")));
        }
    }
    spdlog::info(std::format("Stored {} charts in DB.", chart_count));

}  // -----  end of method PF_CollectDataApp::ShutdownStoreOutputInDB  -----

void PF_CollectDataApp::WaitForTimer(const std::chrono::zoned_seconds &stop_at)
{
    while (true)
    {
        // if the user has signaled time to leave, then do it

        if (PF_CollectDataApp::had_signal_)
        {
            std::cout << "\n*** User interrupted. ***" << std::endl;
            break;
        }

        const std::chrono::zoned_seconds now = std::chrono::zoned_seconds(
            std::chrono::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
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
}  // -----  end of method PF_CollectDataApp::WaitForTimer  -----

void PF_CollectDataApp::HandleSignal(int signal)

{
    // only thing we need to do

    PF_CollectDataApp::had_signal_ = true;

} /* -----  end of method PF_CollectDataApp::HandleSignal  ----- */
