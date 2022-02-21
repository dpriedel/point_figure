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
#include <set>
#include <string_view>
#include <thread>

#include <range/v3/algorithm/equal.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/drop.hpp>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

#include <pybind11/embed.h>
#include <pybind11/gil.h>

namespace py = pybind11;
using namespace py::literals;

#include "DDecQuad.h"
#include "PF_Chart.h"
#include "PF_CollectDataApp.h"
#include "PF_Column.h"
#include "Tiingo.h"
#include "utilities.h"

using namespace std::string_literals;
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
	
    // first, we want upper case symbols.

    ranges::for_each(symbol_, [](char& c) { c = std::toupper(c); });
    symbol_list_ = split_string<std::string>(symbol_, ',');

    // possibly empty if this is our first time or we are starting over

    if (! fs::exists(input_chart_directory_))
    {
        fs::create_directories(input_chart_directory_);
    }

    // we could write out data to a separate location if we want 
    // otherwise, use the charts directory. 

    if (output_chart_directory_.empty())
    {
        output_chart_directory_ = input_chart_directory_;
    }
    if (! fs::exists(output_chart_directory_))
    {
        fs::create_directories(output_chart_directory_);
    }

    BOOST_ASSERT_MSG(source_i == "file" | source_i == "streaming", fmt::format("Data source must be 'file' or 'streaming': {}", source_i).c_str());
    source_ = source_i == "file" ? Source::e_file : Source::e_streaming;
    
    if (source_ == Source::e_file)
    {
        BOOST_ASSERT_MSG(! new_data_input_directory_.empty(), "Must specify 'new_data_dir' when data source is 'file'.");
        BOOST_ASSERT_MSG(fs::exists(new_data_input_directory_), fmt::format("Can't find new data input directory: {}", new_data_input_directory_).c_str());

        BOOST_ASSERT_MSG(source_format_i == "csv" | source_format_i == "json", fmt::format("Data source must be 'csv' or 'json': {}", source_format_i).c_str());
        source_format_ = source_format_i == "csv" ? SourceFormat::e_csv : SourceFormat::e_json;
    }
    else
    {
        BOOST_ASSERT_MSG(! tiingo_api_key_.empty(), "Must specify api 'key' file when data source is 'streaming'.");
        BOOST_ASSERT_MSG(fs::exists(tiingo_api_key_), fmt::format("Can't tiingo api key file: {}", tiingo_api_key_).c_str());
    }
    
    BOOST_ASSERT_MSG(destination_i == "file" | destination_i == "DB", fmt::format("Data destination must be 'file' or 'DB': {}", destination_i).c_str());
    destination_ = destination_i == "file" ? Destination::e_file : Destination::e_DB;

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

    BOOST_ASSERT_MSG(scale_i == "linear" | scale_i == "percent", fmt::format("Chart scale must be 'linear' or 'percent': {}", scale_i).c_str());
    scale_ = scale_i == "linear" ? Boxes::BoxScale::e_linear : Boxes::BoxScale::e_percent;

    if (scale_ == Boxes::BoxScale::e_percent || box_size_.GetExponent() < 0)
    {
        fractional_boxes_ = Boxes::BoxType::e_fractional;
    }
    else 
    {
        fractional_boxes_ = Boxes::BoxType::e_integral;
    }

	return true ;
}		// -----  end of method PF_CollectDataApp::Do_CheckArgs  -----

void PF_CollectDataApp::SetupProgramOptions ()
{
    newoptions_ = std::make_unique<po::options_description>();

	newoptions_->add_options()
		("help,h",											"produce help message")
		("symbol,s",			po::value<std::string>(&this->symbol_)->required(),	"name of symbol we are processing data for. May be a comma-delimited list.")
		("new-data-dir",		po::value<fs::path>(&this->new_data_input_directory_),	"name of directory containing files with new data for symbols we are using.")
		("chart-data-dir",		po::value<fs::path>(&this->input_chart_directory_)->required(),	"name of directory containing existing files with data for symbols we are using.")
		("destination",	    	po::value<std::string>(&this->destination_i)->default_value("file"),	"destination: send data to 'file' or 'DB'. Default is 'file'.")
		("source",				po::value<std::string>(&this->source_i)->default_value("file"),	"source: either 'file' or 'streaming'. Default is 'file'")
		("source-format",		po::value<std::string>(&this->source_format_i)->default_value("csv"),	"source data format: either 'csv' or 'json'. Default is 'csv'")
		("mode,m",				po::value<std::string>(&this->mode_i)->default_value("load"),	"mode: either 'load' new data or 'update' existing data. Default is 'load'")
		("interval,i",			po::value<std::string>(&this->interval_i)->default_value("eod"),	"interval: 'eod', 'live', '1sec', '5sec', '1min', '5min'. Default is 'eod'")
		("scale",				po::value<std::string>(&this->scale_i)->default_value("linear"),	"scale: 'linear', 'percent'. Default is 'linear'")
		("price-fld-name",		po::value<std::string>(&this->price_fld_name_)->default_value("Close"),	"price_fld_name: which data field to use for price value. Default is 'Close'.")
		("output-dir,o",		po::value<fs::path>(&this->output_chart_directory_),	"output: directory for output files.")
		("boxsize,b",			po::value<DprDecimal::DDecQuad>(&this->box_size_)->required(),   	"box step size. 'n', 'm.n'")
		("reversal,r",			po::value<int32_t>(&this->reversal_boxes_)->default_value(2),		"reversal size in number of boxes. Default is 2")
		("log-path",            po::value<fs::path>(&log_file_path_name_),	"path name for log file.")
		("log-level,l",         po::value<std::string>(&logging_level_)->default_value("information"), "logging level. Must be 'none|error|information|debug'. Default is 'information'.")
        ("host",                po::value<std::string>(&this->host_name_)->default_value("api.tiingo.com"), "web site we download from. Default is 'api.tiingo.com'.")
        ("port",                po::value<std::string>(&this->host_port_)->default_value("443"), "Port number to use for web site. Default is '443'.")
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
    if(source_ == Source::e_file)
    {
        if (mode_ == Mode::e_load)
        {
            for (const auto& symbol : symbol_list_)
            {
                fs::path symbol_file_name = new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
                BOOST_ASSERT_MSG(fs::exists(symbol_file_name), fmt::format("Can't find data file for symbol: {} for update.", symbol).c_str());
                auto chart = LoadSymbolPriceDataCSV(symbol, symbol_file_name);
                charts_[symbol] = chart;
            }
        }
        else if (mode_ == Mode::e_update)
        {
            // look for existing data and load the saved JSON data if we have it.
            // then add the new data to the chart.

            for (const auto& symbol : symbol_list_)
            {
                PF_Chart new_chart{symbol, box_size_, reversal_boxes_, fractional_boxes_, scale_};
                fs::path existing_data_file_name = input_chart_directory_ / new_chart.ChartName("json");
                if (fs::exists(existing_data_file_name))
                {
                    new_chart = LoadAndParsePriceDataJSON(symbol, existing_data_file_name);
                }
                fs::path update_file_name = new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
                BOOST_ASSERT_MSG(fs::exists(update_file_name), fmt::format("Can't find data file for symbol: {} for update.", update_file_name).c_str());
                AddPriceDataToExistingChartCSV(new_chart, update_file_name);
                charts_[symbol] = new_chart;
            }
        }
    }
    else
    {
        api_key_ = LoadDataFileForUse(tiingo_api_key_);
        if (api_key_.ends_with('\n'))
        {
            api_key_.resize(api_key_.size() - 1);
        }

        // this should be a program param... 

        number_of_days_history_for_ATR_ = 20;

        // set up our charts 

        for (const auto& symbol : symbol_list_)
        {
            DprDecimal::DDecQuad box_size;
            if (use_ATR_)
            {
                box_size = ComputeBoxSizeUsingATR(symbol) * box_size_;
                box_size = box_size.Rescale(box_size.GetExponent() -1);
            }
            else
            {
                box_size = box_size_;
            }
            charts_[symbol] = PF_Chart{symbol, box_size, reversal_boxes_, fractional_boxes_, scale_};
        }
        // let's stream !
        
        CollectStreamingData();
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
//    PF_Chart chart{symbol_, box_size_, reversalboxes_};
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
    PF_Chart new_chart{symbol, box_size_, reversal_boxes_, fractional_boxes_, scale_};

    AddPriceDataToExistingChartCSV(new_chart, symbol_file_name);

    return new_chart;
}		// -----  end of method PF_CollectDataApp::LoadSymbolPriceDataCSV  ----- 

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
            auto dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";
            new_chart.AddValue(DprDecimal::DDecQuad(fields[close_col]), StringToTimePoint(dt_format, fields[date_col]));
        });

}		// -----  end of method PF_CollectDataApp::AddPriceDataToExistingChartCSV  ----- 

PF_Chart PF_CollectDataApp::LoadAndParsePriceDataJSON (const std::string& symbol, const fs::path& symbol_file_name) const
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


DprDecimal::DDecQuad PF_CollectDataApp::ComputeBoxSizeUsingATR (const std::string& symbol) const
{
    Tiingo history_getter{host_name_, host_port_, api_key_};

    // we need to start from yesterday since we won't get history data for today 
    // since we are doing this while the market is open

    date::year_month_day today{--floor<date::days>(std::chrono::system_clock::now())};

    // use our new holidays capability 
    // we look backwards here. so add an extra year in case we are near New Years.
    
    auto holidays = MakeHolidayList(today.year());
    ranges::for_each(MakeHolidayList(--(today.year())), [&holidays](const auto& e) { holidays.push_back(e); });

    const auto history = history_getter.GetMostRecentTickerData(symbol, today, number_of_days_history_for_ATR_ + 1, &holidays);

    auto atr = ComputeATR(symbol, history, number_of_days_history_for_ATR_, UseAdjusted::e_No);

    auto box_size = (box_size_ * atr).Rescale(box_size_.GetExponent() - 1);
    return box_size;
}		// -----  end of method PF_CollectDataApp::ComputeBoxSizeUsingATR  ----- 

void PF_CollectDataApp::CollectStreamingData ()
{
    // we're going to use 2 threads here -- a producer thread which collects streamed 
    // data from Tiingo nad a consummer thread which will take that data, decode it 
    // and load it into appropriate charts. 
    // Processing continues until interrupted. 

    // since this code can potentially run for hours on end 
    // it's a good idea to provide a way to break into this processing and shut it down cleanly.
    // so, a little bit of C...(taken from "Advanced Unix Programming" by Warren W. Gay, p. 317)

    // ok, get ready to handle keyboard interrupts, if any.

    struct sigaction sa_old;
    struct sigaction sa_new;

    sa_new.sa_handler = PF_CollectDataApp::HandleSignal;
    sigemptyset(&sa_new.sa_mask);
    sa_new.sa_flags = 0;
    sigaction(SIGINT, &sa_new, &sa_old);

    PF_CollectDataApp::had_signal_= false;

    Tiingo quotes{host_name_, host_port_, "/iex", api_key_, symbol_list_};
    quotes.Connect();

    std::mutex data_mutex;
    std::queue<std::string> streamed_data;

    py::gil_scoped_release gil{};
    auto streaming_task = std::async(std::launch::async, &Tiingo::StreamData, &quotes, &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);
    auto processing_task = std::async(std::launch::async, &PF_CollectDataApp::ProcessStreamedData, this, &quotes, &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);

    streaming_task.get();
    processing_task.get();

    quotes.Disconnect();

    // make a last check to be sure we  didn't leave any data unprocessed 

    ProcessStreamedData(&quotes, &PF_CollectDataApp::had_signal_, &data_mutex, &streamed_data);

}		// -----  end of method PF_CollectDataApp::CollectStreamingData  ----- 

void PF_CollectDataApp::ProcessStreamedData (Tiingo* quotes, bool* had_signal, std::mutex* data_mutex, std::queue<std::string>* streamed_data)
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
            quotes->ExtractData(new_data);
            std::vector<std::string> need_to_update_graph;
            for (const auto& new_value: *quotes)
            {
                auto chart_changed = charts_[new_value.ticker_].AddValue(new_value.last_price_, PF_Column::tpt{std::chrono::nanoseconds{new_value.time_stamp_seconds_}});
                if (chart_changed != PF_Column::Status::e_ignored)
                {
                    need_to_update_graph.push_back(new_value.ticker_);
                }

                quotes->ClearExtractedData();
            }
            for (const auto& ticker : need_to_update_graph)
            {
                py::gil_scoped_acquire gil{};
                fs::path graph_file_path = output_chart_directory_ / (charts_[ticker].ChartName("svg"));
                charts_[ticker].ConstructChartGraphAndWriteToFile(graph_file_path, PF_Chart::Y_AxisFormat::e_show_time);

                fs::path chart_file_path = output_chart_directory_ / (charts_[ticker].ChartName("json"));
                std::ofstream updated_file{chart_file_path, std::ios::out | std::ios::binary};
                BOOST_ASSERT_MSG(updated_file.is_open(), fmt::format("Unable to open file: {} to write updated data.", chart_file_path).c_str());
                charts_[ticker].ConvertChartToJsonAndWriteToStream(updated_file);
                updated_file.close();
            }
        }
        else
        {
            std::this_thread::sleep_for(2ms);
        }
        if (streamed_data->empty() && *had_signal == true)
        {
            break;
        }
    }
}		// -----  end of method PF_CollectDataApp::ProcessStreamedData  ----- 

void PF_CollectDataApp::Shutdown ()
{
    if (destination_ == Destination::e_file)
    {
        for (const auto& [symbol, chart] : charts_)
        {
            fs::path output_file_name = output_chart_directory_ / chart.ChartName("json"); 
            std::ofstream output(output_file_name, std::ios::out | std::ios::binary);
            BOOST_ASSERT_MSG(output.is_open(), fmt::format("Unable to open output file: {}.", output_file_name).c_str());
            chart.ConvertChartToJsonAndWriteToStream(output);
            output.close();

            fs::path graph_file_path = output_chart_directory_ / (chart.ChartName("svg"));
            chart.ConstructChartGraphAndWriteToFile(graph_file_path, interval_ != Interval::e_eod ? PF_Chart::Y_AxisFormat::e_show_time : PF_Chart::Y_AxisFormat::e_show_date);
        }
    }
    spdlog::info(fmt::format("\n\n*** End run {} ***\n", LocalDateTimeAsString(std::chrono::system_clock::now())));
}       // -----  end of method PF_CollectDataApp::Shutdown  -----

void PF_CollectDataApp::HandleSignal(int signal)

{
    // only thing we need to do

    PF_CollectDataApp::had_signal_ = true;

}		/* -----  end of method PF_CollectDataApp::HandleSignal  ----- */

