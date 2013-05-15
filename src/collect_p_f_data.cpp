// =====================================================================================
// 
//       Filename:  collect_p_f_data.cpp
// 
//    Description:  This program will compute Point & Figure data for the
//    				given input file.
// 
//        Version:  1.0
//        Created:  04/23/2013 02:00:49 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company: 
// 
// =====================================================================================

#include <cstdlib>
#include <iostream>
#include <algorithm>
#include <iterator>
//#include <decDouble.h>

#include "TException.h"
#include "aLine.h"
#include "collect_p_f_data.h"
//#include "DDecimal_16.h"
#include "DDecimal_32.h"

//template<32>
decContext DDecimal<32>::mCtx;

int
main ( int argc, char *argv[] )
{
	int result = EXIT_SUCCESS;
	 
	CMyApp* myApp = NULL;

	try
	{
		//	first, let's create an instance of our class
	
		myApp = new CMyApp(argc, argv);
		CApplication::sTheApplication = myApp;
	}
	catch (...)
	{
		//	If we're here, there is probably no application object
			
		std::cerr << "Error creating the Application.  Nothing done!" << std::endl;

		//	any errors on startup and we're outta here
		
		return 2;
	}
	//	Normal processing...
	
	try
	{
		myApp->StartUp ();		//	all errors will throw an exception
		myApp->Run ();
	}

	catch (std::exception& theProblem)
	{
		CApplication::sCErrorHandler->HandleException(theProblem);
		result = 3;
		if (myApp)
			myApp->Done ();				//	base class exceptions are always fatal.
	}
	catch (...)
	{
		result = 4;
		std::cerr << "Something totally unexpected happened." << std::endl;
		if (myApp)
			myApp->Done ();				//	base class exceptions are always fatal.
	}

	if (myApp && myApp->IsDone ())
	{
		myApp->Quit ();
		delete myApp;
	}

	return result;
}// ----------  end of function main  ----------


//--------------------------------------------------------------------------------------
//       Class:  CMyApp
//      Method:  CMyApp
// Description:  constructor
//--------------------------------------------------------------------------------------
CMyApp::CMyApp (int argc, char* argv[])
	: CApplication(argc, argv),
	mInputIsPath(false), mOutputIsPath(false), mSource(source::unknown), mDestination(destination::unknown),
	mMode(mode::unknown)

{
}  // -----  end of method CMyApp::CMyApp  (constructor)  -----

CMyApp::~CMyApp (void)
{
	return ;
}		// -----  end of method CMyApp::~CMyApp  -----

void
CMyApp::Do_StartUp (void)
{
	return ;
}		// -----  end of method CMyApp::Do_StartUp  -----


void
CMyApp::Do_CheckArgs (void)
{
	//	let's get our input and output set up
	
	dfail_if_(! mVariableMap.count("symbol"), "Symbol must be specified.");
	mSymbol = mVariableMap["symbol"].as<std::string>();

	dfail_if_(! mVariableMap.count("file"), "Input source must be specified.");
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
		dfail_if_(! fs::exists(mInputPath), "Can't find input file: ", mInputPath.c_str());
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
			mMode = mode::load;
		else if (temp == "update")
			mMode = mode::update;
		else
			dfail_msg_("Unkown 'mode': ", temp.c_str());
	}

	//	if not specified, assume we are sending output to stdout
	
	if (mVariableMap.count("destination") == 0)
	{
		mDestination = destination::stdout;
	}
	else
	{
		std::string temp = mVariableMap["destination"].as<std::string>();
		if (temp == "file")
		{
			mDestination = destination::file;
			dfail_if_(! mVariableMap.count("output"), "Output destination of 'file' specified but no 'output' name provided.");
			std::string temp = mVariableMap["output"].as<std::string>();
			if (temp == "-")
			{
				mOutputIsPath = false;
				mDestination = destination::stdout;
			}
			else
				mOutputPath = temp;
		}
		else if (temp == "DB")
		{
			mDestination = destination::DB;
			dfail_if_(! mVariableMap.count("output"), "Output destination of 'DB' specified but no 'output' table name provided.");
			std::string temp = mVariableMap["output"].as<std::string>();
			dfail_if_(temp == "-", "Invalid DB table name specified: '-'.");
			mDBName = temp;
		}
		else
			dfail_msg_("Invalid destination type specified. Must be: 'file' or 'DB'.");

	}
	return ;
}		// -----  end of method CMyApp::Do_CheckArgs  -----

void
CMyApp::Do_SetupProgramOptions (void)
{
	mNewOptions.add_options()
		("help",											"produce help message")
		("symbol,s",			po::value<std::string>(),	"name of symbol we are processing data for")
		("file,f",				po::value<std::string>(),	"name of file containing data for symbol")
		("mode,m",				po::value<std::string>(),	"mode: either 'load' new data or 'update' existing data. Default is 'load'.")
		("output,o",			po::value<std::string>(),	"output file name")
		("destination,d",		po::value<std::string>(),	"send data to file or DB. Default is 'stdout'.")
		;

	return ;
}		// -----  end of method CMyApp::Do_SetupProgramOptions  -----


void
CMyApp::Do_ParseProgramOptions (void)
{
	return ;
}		// -----  end of method CMyApp::Do_ParseProgramOptions  -----


void
CMyApp::Do_Run (void)
{
	//	simple copy code from input to output
	
	/* if (mSource == source::stdin && mDestination == destination::stdout) */
	/* { */
	/* 	std::ostream_iterator<aLine> otor(std::cout, "\n"); */
	/* 	std::istream_iterator<aLine> itor(std::cin); */
	/* 	std::istream_iterator<aLine> itor_end; */

	/* 	std:copy(itor, itor_end, otor); */
	/* } */
	/* else */
	/* 	std::cout << "not stdin and stdout." << std::endl; */

	//	play with decimal support in c++11
	
	/* decContext set; */
	/* decContextDefault(&set, DEC_INIT_DECDOUBLE); */

	/* decDouble a,b,c; */
	/* decDoubleFromString(&a, "12.3", &set); */
	/* decDoubleFromString(&b, "0.345", &set); */
	/* decDoubleAdd(&c, &a, &b, &set); */

	/* char output [DECDOUBLE_String]; */

	/* decDoubleToString(&c, output); */

	/* std::cout << "12.3 + 0,345 = " << output << std::endl; */

	DDecimal<32> testDec;
	DDecimal<32> testDec2("123.45");

	std::cout << testDec2 << std::endl;

	std::cin >> testDec;
	std::cout << "after reading stream " << testDec << std::endl;

	testDec += 4.3;
	std::cout << "after adding 4.3: " << testDec << std::endl;

	testDec -= 4.3;
	std::cout << "after subtracting 4.3: " << testDec << std::endl;

	testDec *= 4.3;
	std::cout << "after multiplying by 4.3: " << testDec << std::endl;

	testDec /= 4.3;
	std::cout << "after dividing by 4.3: " << testDec << std::endl;

	DDecimal<32> a = testDec + testDec2;
	std::cout << "adding 2 numbers: " << testDec << " and " << testDec2 << " = " << a << std::endl;

	bool xx = testDec == testDec2;
	std::cout << "test " << testDec << " and " << testDec2 << " for equality.  result is: " << std::boolalpha << xx << std::endl;

	xx = testDec < testDec2;
	std::cout << "test " << testDec << " and " << testDec2 << " for less than.  result is: " << std::boolalpha << xx << std::endl;

	std::string yy = testDec2.ToStr();
	std::cout << "convert back to string: " << yy << std::endl;

	return ;
}		// -----  end of method CMyApp::Do_Run  -----

