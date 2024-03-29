// =====================================================================================
//
//       Filename:  Main.cpp
//
//    Description:  Driver program for application
//
//        Version:  1.0
//        Created:  02/28/2014 03:04:02 PM
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


#include <exception>
#include <iostream>
 
// #include <pybind11/embed.h> // everything needed for embedding
// namespace py = pybind11;
// using namespace py::literals;

#include <decimal.hh>

using decimal::Decimal;

#include "PF_CollectDataApp.h"

int main(int argc, char** argv)
{
	int result = 0;

	try
	{
        decimal::context_template = decimal::IEEEContext(decimal::DECIMAL64);
        decimal::context_template.round(decimal::ROUND_HALF_UP);
        decimal::context = decimal::context_template;

	    //	help to optimize c++ stream I/O (may screw up threaded I/O though)

	    std::ios_base::sync_with_stdio(false);

        // py::scoped_interpreter guard{false}; // start the interpreter and keep it alive
        //
        // py::exec(R"(
        //     import PF_DrawChart
        //     )"
        // );
		PF_CollectDataApp  myApp(argc, argv);
		bool startup_ok = myApp.Startup();
        if (startup_ok)
        {
            myApp.Run();
            myApp.Shutdown();
        }
	}

    catch (std::system_error& e)
    {
        // any system problems, we eventually abort, but only after finishing work in process.

        auto ec = e.code();
        std::cerr << "Category: " << ec.category().name() << ". Value: " << ec.value() <<
                ". Message: " << ec.message() << '\n';
        result = 3;
    }
    catch (std::exception& e)
    {
        std::cout << "Problem collecting files: " << e.what() << '\n';
        result = 4;
    }
    catch (...)
    {
        std::cout << "Unknown problem collecting files." << '\n';
        result = 5;
    }

	return result;
}

