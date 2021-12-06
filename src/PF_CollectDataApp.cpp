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

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <fstream>
#include <set>

#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/drop.hpp>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
#include <string_view>
//#include <decDouble.h>

#include "DDecQuad.h"
#include "PF_Chart.h"
#include "PF_CollectDataApp.h"
#include "PF_Column.h"
#include "utilities.h"
#include "aLine.h"


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
    spdlog::info(fmt::format("\n\n*** Begin run {}  ***\n", LocalDateTimeAsString(std::chrono::system_clock::now())));
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
	
    symbol_list_ = split_string<std::string>(symbol_, ',');

    if (! input_file_dirctory_.empty())
    {
        BOOST_ASSERT_MSG(fs::exists(input_file_dirctory_), fmt::format("Can't find input file directory: {}", input_file_dirctory_).c_str());
    }

    if (! output_file_directory_.empty())
    {
        if (! fs::exists(output_file_directory_))
        {
            fs::create_directories(output_file_directory_);
        }
    }

    BOOST_ASSERT_MSG(source_i == "file" | source_i == "streaming", fmt::format("Data source must be 'file' or 'streaming': {}", source_i).c_str());
    source_ = source_i == "file" ? Source::e_file : Source::e_streaming;
    
    BOOST_ASSERT_MSG(source_format_i == "csv" | source_format_i == "json", fmt::format("Data source must be 'csv' or 'json': {}", source_format_i).c_str());
    source_format_ = source_format_i == "csv" ? SourceFormat::e_csv : SourceFormat::e_json;
    
    BOOST_ASSERT_MSG(destination_i == "file" | destination_i == "DB", fmt::format("Data destination must be 'file' or 'DB': {}", destination_i).c_str());
    destination_ = destination_i == "file" ? Destination::e_file : Destination::e_DB;
    if (destination_ == Destination::e_file)
    {
        BOOST_ASSERT_MSG(! output_file_directory_.empty(), "Output data being sent to a file but no output directory provided.");
    }

    BOOST_ASSERT_MSG(mode_i == "load" | mode_i == "update", fmt::format("Mode must be 'load' or 'update': {}", mode_i).c_str());
    mode_ = mode_i == "load" ? Mode::e_load : Mode::e_update;

    const std::set<std::string> possible_intervals = {"eod", "live", "sec1", "sec5", "min1", "min5"};
    BOOST_ASSERT_MSG(possible_intervals.contains(interval_i), fmt::format("Interval must be 'eod', 'live', 'sec1', 'sec5', 'min1', 'min5': {}", interval_i).c_str());
    if (interval_i == "eod")
    {
        interval_ = Interval::e_eod;
    }
    else if (interval_i == "live")
    {
        interval_ = Interval::e_live;
    }
    else if (interval_i == "sec1")
    {
        interval_ = Interval::e_sec1;
    }
    else if (interval_i == "sec5")
    {
        interval_ = Interval::e_sec5;
    }
    else if (interval_i == "min1")
    {
        interval_ = Interval::e_min1;
    }
    else if (interval_i == "min5")
    {
        interval_ = Interval::e_min5;
    }
    BOOST_ASSERT_MSG(scale_i == "arithmetic" | scale_i == "logarithmetic", fmt::format("Chart scale must be 'arithmetic' or 'logarithmetic': {}", scale_i).c_str());
    scale_ = scale_i == "arithmetic" ? PF_Column::ColumnScale::e_arithmetic : PF_Column::ColumnScale::e_logarithmic;

	return true ;
}		// -----  end of method PF_CollectDataApp::Do_CheckArgs  -----

void PF_CollectDataApp::SetupProgramOptions ()
{
    newoptions_ = std::make_unique<po::options_description>();

	newoptions_->add_options()
		("help,h",											"produce help message")
		("symbol,s",			po::value<std::string>(&this->symbol_)->required(),	"name of symbol we are processing data for. May be a comma-delimited list.")
		("input_dir",			po::value<fs::path>(&this->input_file_dirctory_),	"name of directory containing files for symbols we are using.")
		("destination",	    	po::value<std::string>(&this->destination_i)->default_value("file"),	"destination: send data to 'file' or 'DB'. Default is 'file'.")
		("source",				po::value<std::string>(&this->source_i)->default_value("file"),	"source: either 'file' or 'streaming'. Default is 'file'")
		("source_format",		po::value<std::string>(&this->source_format_i)->default_value("csv"),	"source data format: either 'csv' or 'json'. Default is 'csv'")
		("mode,m",				po::value<std::string>(&this->mode_i)->default_value("load"),	"mode: either 'load' new data or 'update' existing data. Default is 'load'")
		("interval,i",			po::value<std::string>(&this->interval_i)->default_value("eod"),	"intervale: 'eod', 'live', '1sec', '5sec', '1min', '5min'. Default is 'eod'")
		("scale",				po::value<std::string>(&this->scale_i)->default_value("arithmetic"),	"scale: 'arithmetic', 'log'. Default is 'arithmetic'")
		("price_fld_name",		po::value<std::string>(&this->price_fld_name_)->default_value("Close"),	"price_fld_name: which data field to use for price value. Default is 'Close'.")
		("output_dir,o",		po::value<fs::path>(&this->output_file_directory_),	"output: directory for output files.")
		("boxsize,b",			po::value<DprDecimal::DDecQuad>(&this->boxsize_)->required(),   	"box step size. 'n', 'm.n'")
		("reversal,r",			po::value<int32_t>(&this->reversal_boxes_)->default_value(2),		"reversal size in number of boxes. Default is 2")
		("log-path",            po::value<fs::path>(&log_file_path_name_),	"path name for log file.")
		("log-level,l",         po::value<std::string>(&logging_level_)->default_value("information"), "logging level. Must be 'none|error|information|debug'. Default is 'information'.")
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
    if(source_ == Source::e_file)
    {
        for (const auto& symbol : symbol_list_)
        {
            fs::path symbol_file_name = input_file_dirctory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
            BOOST_ASSERT_MSG(fs::exists(symbol_file_name), fmt::format("Can't find data file for symbol: {} for update.", symbol).c_str());
            auto chart = LoadSymbolPriceDataCSV(symbol, symbol_file_name);
            charts_[symbol] = chart;
        }
    }

//	// open a stream on the specified input source.
//	
//	std::istream* theInput{nullptr};
//	std::ifstream iFile;
//
//	if (source_ == Source::stdin)
//    {
//		theInput = &std::cin;
//    }
//	else if (source_ == Source::file)
//	{
//		iFile.open(inputpath_.string(), std::ios_base::in | std::ios_base::binary);
//        BOOST_ASSERT_MSG(iFile.is_open(), fmt::format("Unable to open input file: {}", inputpath_).c_str());
//		theInput = &iFile;
//	}
//	else
//    {
//        throw std::invalid_argument("Unspecified input.");
//    }
//
//	std::istream_iterator<aLine> itor{*theInput};
//	std::istream_iterator<aLine> itor_end;
//
//	std::ostream* theOutput{nullptr};
//	std::ofstream oFile;
//
//	if (destination_ == Destination::stdout)
//    {
//		theOutput = &std::cout;
//    }
//	else
//	{
//		oFile.open(outputpath_.string(), std::ios::out | std::ios::binary);
//        BOOST_ASSERT_MSG(oFile.is_open(), fmt::format("Unable to open output file: {}", inputpath_).c_str());
//		theOutput = &oFile;
//	}
//
////	std::ostream_iterator<aLine> otor{*theOutput, "\n"};
////
//////	std::copy(itor, itor_end, otor);
////
////	// sampel code using a lambda
////	std:transform(itor, itor_end, otor,
////				[] (const aLine& data) {DprDecimal::DDecDouble aa(data.lineData); aLine bb; bb.lineData = aa.ToStr(); return bb; });
//
//    PF_Chart chart{symbol_, boxsize_, reversalboxes_};
//
////    chart.LoadData<DprDecimal::DDecQuad>(theInput);
//    std::cout << "chart info: " << chart.GetNumberOfColumns() << '\n';
//
//    chart.ConstructChartAndWriteToFile(outputpath_);
//	//	play with decimal support in c++11
//	
	return {} ;
}		// -----  end of method PF_CollectDataApp::Do_Run  -----

// function to find out which column number is the specifed column.



PF_Chart PF_CollectDataApp::LoadSymbolPriceDataCSV (const std::string& symbol, const fs::path& symbol_file_name) const
{
    const std::string file_content = LoadDataFileForUse(symbol_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content, '\n');
    
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ',');
    BOOST_ASSERT_MSG(date_column.has_value(), fmt::format("Can't find 'date' field in header record: {}.", header_record).c_str());

    auto close_column = FindColumnIndex(header_record, price_fld_name_, ',');
    BOOST_ASSERT_MSG(close_column.has_value(), fmt::format("Can't find price field: {} in header record: {}.", price_fld_name_, header_record).c_str());

    PF_Chart new_chart{symbol, boxsize_, reversal_boxes_};

    ranges::for_each(symbol_data_records | ranges::views::drop(1), [&new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record)
        {
            const auto fields = split_string<std::string_view> (record, ',');
            new_chart.AddValue(DprDecimal::DDecQuad(fields[close_col]), StringToTimePoint("%Y-%m-%d", fields[date_col]));
        });

    return new_chart;
}		// -----  end of method PF_CollectDataApp::LoadSymbolPriceDataCSV  ----- 

std::optional<int> PF_CollectDataApp::FindColumnIndex (std::string_view header, std::string_view column_name, char delim) const
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

void PF_CollectDataApp::Shutdown ()
{
    if (destination_ == Destination::e_file)
    {
        for (const auto& [symbol, chart] : charts_)
        {
            fs::path output_file_name = output_file_directory_ / chart.ChartName();
            std::ofstream output(output_file_name, std::ios::out | std::ios::binary);
            BOOST_ASSERT_MSG(output.is_open(), fmt::format("Unable to open output file: {}.", output_file_name).c_str());
            chart.ConvertChartToJsonAndWriteToStream(output);
            output.close();
        }

    }
    spdlog::info(fmt::format("\n\n*** End run {} ***\n", LocalDateTimeAsString(std::chrono::system_clock::now())));
}       // -----  end of method ExtractorApp::Shutdown  -----

