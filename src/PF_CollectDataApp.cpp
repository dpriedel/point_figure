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

#include <date/tz.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>
//#include <decDouble.h>

#include "PF_CollectDataApp.h"
#include "PF_Chart.h"
#include "aLine.h"


bool CMyApp::had_signal_ = false;

// utility to convert a date::year_month_day to a string
// using Howard Hinnant's date library

inline std::string LocalDateTimeAsString(std::chrono::system_clock::time_point a_date_time)
{
    auto t = date::make_zoned(date::current_zone(), a_date_time);
    std::string ts = date::format("%a, %b %d, %Y at %I:%M:%S %p %Z", t);
    return ts;
}

//--------------------------------------------------------------------------------------
//       Class:  CMyApp
//      Method:  CMyApp
// Description:  constructor
//--------------------------------------------------------------------------------------
CMyApp::CMyApp (int argc, char* argv[])
    : mArgc{argc}, mArgv{argv},
	mSource{source::unknown}, mDestination{destination::unknown}, mMode{mode::unknown}, mInterval{interval::unknown}, 
	mScale{scale::unknown}, mInputIsPath{false}, mOutputIsPath{false} 

{
}  // -----  end of method CMyApp::CMyApp  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  CMyApp
//      Method:  CMyApp
// Description:  constructor
//--------------------------------------------------------------------------------------
CMyApp::CMyApp (const std::vector<std::string>& tokens)
    : tokens_{tokens}
{
}  // -----  end of method CMyApp::CMyApp  (constructor)  -----


void CMyApp::ConfigureLogging ()
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

}		// -----  end of method CMyApp::ConfigureLogging  ----- 

bool CMyApp::Startup ()
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
}		// -----  end of method CMyApp::Do_StartUp  -----


bool CMyApp::CheckArgs ()
{
	//	let's get our input and output set up
	
    BOOST_ASSERT_MSG(mVariableMap.count("symbol") != 0, "Symbol must be specified.");
	mSymbol = mVariableMap["symbol"].as<std::string>();

	if(mVariableMap.count("file") == 0)
	{
		mSource = source::stdin;
		mInputIsPath = false;
	}
	else
	{
		std::string temp = mVariableMap["file"].as<std::string>();
		if (temp == "-")
		{
			mSource = source::stdin;
			mInputIsPath = false;
		}
		else
		{
			mSource = source::file;
			mInputPath = temp;
            BOOST_ASSERT_MSG(fs::exists(mInputPath), fmt::format("Can't find input file: {}", mInputPath).c_str());
		}
	}

	//	if not specified, assume we are doing a 'load' operation for symbol.
	
	if (mVariableMap.count("mode") == 0)
	{
		mMode = mode::load;
	}
	else
	{
		std::string temp = mVariableMap["mode"].as<std::string>();
		if (temp == "load")
        {
			mMode = mode::load;
        }
		else if (temp == "update")
        {
			mMode = mode::update;
        }
		else
        {
			throw std::invalid_argument(fmt::format("Unkown 'mode': {}", temp));
        }
	}

	//	if not specified, assume we are sending output to stdout
	
	if (mVariableMap.count("destination") == 0)
	{
		mDestination = destination::stdout;
		mOutputIsPath = false;
	}
	else
	{
		std::string temp = mVariableMap["destination"].as<std::string>();
		if (temp == "file")
		{
			mDestination = destination::file;
            BOOST_ASSERT_MSG(mVariableMap.count("output") != 0, "Output destination of 'file' specified but no 'output' name provided.");
			std::string temp = mVariableMap["output"].as<std::string>();
			if (temp == "-")
			{
				mOutputIsPath = false;
				mDestination = destination::stdout;
			}
			else
			{
				mOutputPath = temp;
				mOutputIsPath = true;
			}
		}
		else if (temp == "DB")
		{
			mDestination = destination::DB;
            BOOST_ASSERT_MSG(mVariableMap.count("output") != 0, "Output destination of 'DB' specified but no 'output' table name provided.");
			std::string temp = mVariableMap["output"].as<std::string>();
            BOOST_ASSERT_MSG(temp != "-", "Invalid DB table name specified: '-'.");
			mDBName = temp;
		}
		else
        {
			throw std::invalid_argument("Invalid destination type specified. Must be: 'file' or 'DB'.");
        }

	}

    BOOST_ASSERT_MSG(mVariableMap.count("boxsize") != 0, "'Boxsize must be specified.");
	int32_t boxsize = mVariableMap["boxsize"].as<int32_t>();
	mBoxSize = boxsize;

	if (mVariableMap.count("reversal") == 0)
    {
		mReversalBoxes = 1;
    }
	else
    {
		mReversalBoxes = mVariableMap["reversal"].as<int>();
    }

	if (mVariableMap.count("scale") == 0)
    {
		mScale = scale::arithmetic;
    }
	else
	{
		std::string temp = mVariableMap["reversal"].as<std::string>();
		if (temp == "arithmetic")
        {
			mScale = scale::arithmetic;
        }
		else if (temp == "log")
        {
			mScale = scale::log;
        }
		else
        {
			throw std::invalid_argument("Scale must be either 'arithmetic' or 'log'.");
        }
	}
	return true ;
}		// -----  end of method CMyApp::Do_CheckArgs  -----

void CMyApp::SetupProgramOptions ()
{
    mNewOptions = std::make_unique<po::options_description>();

	mNewOptions->add_options()
		("help",											"produce help message")
		("symbol,s",			po::value<std::string>(),	"name of symbol we are processing data for")
		("file,f",				po::value<std::string>(),	"name of file containing data for symbol. Default is stdin")
		("mode,m",				po::value<std::string>(),	"mode: either 'load' new data or 'update' existing data. Default is 'load'")
		("output,o",			po::value<std::string>(),	"output file name")
		("destination,d",		po::value<std::string>(),	"send data to file or DB. Default is 'stdout'.")
		("boxsize,b",			po::value<int32_t>(),   	"box step size. 'n', 'm.n'")
		("reversal,r",			po::value<int32_t>(),		"reversal size in number of boxes. Default is 1")
		("scale",				po::value<std::string>(),	"'arithmetic', 'log'. Default is 'arithmetic'")
		("interval,i",			po::value<std::string>(),	"'eod', 'tic', '1sec', '5sec', '1min', '5min', etc. Default is 'tic'")
		("log-path", po::value<fs::path>(&log_file_path_name_),	"path name for log file.")
		("log-level,l", po::value<std::string>(&logging_level_),
         "logging level. Must be 'none|error|information|debug'. Default is 'information'.")
		;

}		// -----  end of method CMyApp::Do_SetupProgramOptions  -----


void CMyApp::ParseProgramOptions (const std::vector<std::string>& tokens)
{
    if (tokens.empty())
    {
	    auto options = po::parse_command_line(mArgc, mArgv, *mNewOptions);
        po::store(options, mVariableMap);
        if (this->mArgc == 1 ||	mVariableMap.count("help") == 1)
        {
            std::cout << *mNewOptions << "\n";
            throw std::runtime_error("\nExiting after 'help'.");
        }
    }
    else
    {
        auto options = po::command_line_parser(tokens).options(*mNewOptions).run();
        po::store(options, mVariableMap);
        if (mVariableMap.count("help") == 1)
        {
            std::cout << *mNewOptions << "\n";
            throw std::runtime_error("\nExiting after 'help'.");
        }
    }
	po::notify(mVariableMap);    
}		/* -----  end of method ExtractorApp::ParseProgramOptions  ----- */


std::tuple<int, int, int> CMyApp::Run()
{
	// open a stream on the specified input source.
	
	std::istream* theInput{nullptr};
	std::ifstream iFile;

	if (mSource == source::stdin)
    {
		theInput = &std::cin;
    }
	else if (mSource == source::file)
	{
		iFile.open(mInputPath.string(), std::ios_base::in | std::ios_base::binary);
        BOOST_ASSERT_MSG(iFile.is_open(), fmt::format("Unable to open input file: {}", mInputPath).c_str());
		theInput = &iFile;
	}
	else
    {
        throw std::invalid_argument("Unspecified input.");
    }

	std::istream_iterator<aLine> itor{*theInput};
	std::istream_iterator<aLine> itor_end;

	std::ostream* theOutput{nullptr};
	std::ofstream oFile;

	if (mDestination == destination::stdout)
    {
		theOutput = &std::cout;
    }
	else
	{
		oFile.open(mOutputPath.string(), std::ios::out | std::ios::binary);
        BOOST_ASSERT_MSG(oFile.is_open(), fmt::format("Unable to open output file: {}", mInputPath).c_str());
		theOutput = &oFile;
	}

//	std::ostream_iterator<aLine> otor{*theOutput, "\n"};
//
////	std::copy(itor, itor_end, otor);
//
//	// sampel code using a lambda
//	std:transform(itor, itor_end, otor,
//				[] (const aLine& data) {DprDecimal::DDecDouble aa(data.lineData); aLine bb; bb.lineData = aa.ToStr(); return bb; });

    PF_Chart chart{mSymbol, mBoxSize, mReversalBoxes};

    chart.LoadData<DprDecimal::DDecDouble>(theInput);
    std::cout << "chart info: " << chart.GetNumberOfColumns() << '\n';

    chart.ConstructChartAndWriteToFile(mOutputPath);
	//	play with decimal support in c++11
	
	return {} ;
}		// -----  end of method CMyApp::Do_Run  -----

void CMyApp::Shutdown ()
{
    spdlog::info(fmt::format("\n\n*** End run {} ***\n", LocalDateTimeAsString(std::chrono::system_clock::now())));
}       // -----  end of method ExtractorApp::Shutdown  -----
