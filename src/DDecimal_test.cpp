// =====================================================================================
//
//       Filename:  DDecimal_test.cpp
//
//    Description:  This program will run various tests of the
//    				c++ libdecimal wrapper classes
//
//        Version:  1.0
//        Created:  05/17/2013 02:00:49 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:
//
// =====================================================================================

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>

#include "DDecimal_16.h"
#include "DDecimal_test.h"
#include "TException.h"
#include "aLine.h"
// #include "DDecimal_32.h"

// template<32>
decContext DprDecimal::DDecimal<16>::mCtx;

int main(int argc, char *argv[])
{
    int result = EXIT_SUCCESS;

    CMyApp *myApp = NULL;

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
        myApp->StartUp(); //	all errors will throw an exception
        myApp->Run();
    }

    catch (std::exception &theProblem)
    {
        CApplication::sCErrorHandler->HandleException(theProblem);
        result = 3;
        if (myApp)
            myApp->Done(); //	base class exceptions are always fatal.
    }
    catch (...)
    {
        result = 4;
        std::cerr << "Something totally unexpected happened." << std::endl;
        if (myApp)
            myApp->Done(); //	base class exceptions are always fatal.
    }

    if (myApp && myApp->IsDone())
    {
        myApp->Quit();
        delete myApp;
    }

    return result;
} // ----------  end of function main  ----------

//--------------------------------------------------------------------------------------
//       Class:  CMyApp
//      Method:  CMyApp
// Description:  constructor
//--------------------------------------------------------------------------------------
CMyApp::CMyApp(int argc, char *argv[])
    : CApplication(argc, argv), mSource(source::unknown), mMode(mode::unknown), mInputIsPath(false)
{
} // -----  end of method CMyApp::CMyApp  (constructor)  -----

CMyApp::~CMyApp(void)
{
    return;
} // -----  end of method CMyApp::~CMyApp  -----

void CMyApp::Do_StartUp(void)
{
    return;
} // -----  end of method CMyApp::Do_StartUp  -----

void CMyApp::Do_CheckArgs(void)
{
    //	let's get our input and output set up

    if (mVariableMap.count("file") > 0)
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
            dfail_if_(!fs::exists(mInputPath), "Can't find input file: ", mInputPath.c_str());
        }
    }

    //	if not specified, assume we are testing 16 digit decimal.

    if (mVariableMap.count("mode") == 0)
    {
        mMode = mode::d_16;
    }
    else
    {
        std::string temp = mVariableMap["mode"].as<std::string>();
        if (temp == "16")
            mMode = mode::d_16;
        else if (temp == "32")
            mMode = mode::d_32;
        else if (temp == "any")
            mMode = mode::d_any;
        else
            dfail_msg_("Unkown 'mode': ", temp.c_str());
    }

    return;
} // -----  end of method CMyApp::Do_CheckArgs  -----

void CMyApp::Do_SetupProgramOptions(void)
{
    mNewOptions.add_options()("help", "produce help message")("file,f", po::value<std::string>(),
                                                              "name of file containing test directives")(
        "mode,m", po::value<std::string>(), "mode: either '16', '32' or 'any'")
        //		("output,o",			po::value<std::string>(),	"output file name")
        ;

    return;
} // -----  end of method CMyApp::Do_SetupProgramOptions  -----

void CMyApp::Do_ParseProgramOptions(void)
{
    return;
} // -----  end of method CMyApp::Do_ParseProgramOptions  -----

void CMyApp::Do_Run(void)
{
    //	simple copy code from input to output
    //	and initialize a DDecimal value from the string input
    //	and then convert back to a string to see if anything
    //	gets lost in translation.

    /* if (mSource == source::stdin && mDestination == destination::stdout) */
    /* { */
    /* 	std::ostream_iterator<aLine> otor(std::cout, "\n"); */
    /* 	std::istream_iterator<aLine> itor(std::cin); */
    /* 	std::istream_iterator<aLine> itor_end; */

    /* 	//std:copy(itor, itor_end, otor); */
    /* 	std:transform(itor, itor_end, otor, */
    /* 				[] (const aLine& data) {DDecimal<16> aa(data.lineData); aLine bb; bb.lineData = aa.ToStr(); return
     * bb; }); */
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

    DprDecimal::DDecimal<16> testDec;
    DprDecimal::DDecimal<16> testDec2("123.45");

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

    DprDecimal::DDecimal<16> a = testDec + testDec2;
    std::cout << "adding 2 numbers: " << testDec << " and " << testDec2 << " = " << a << std::endl;

    bool xx = testDec == testDec2;
    std::cout << "test " << testDec << " and " << testDec2 << " for equality.  result is: " << std::boolalpha << xx
              << std::endl;

    xx = testDec < testDec2;
    std::cout << "test " << testDec << " and " << testDec2 << " for less than.  result is: " << std::boolalpha << xx
              << std::endl;

    std::string yy = testDec2.ToStr();
    std::cout << "convert back to string: " << yy << std::endl;

    return;
} // -----  end of method CMyApp::Do_Run  -----
