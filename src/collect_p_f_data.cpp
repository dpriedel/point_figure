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

#include "collect_p_f_data.h"

int
main ( int argc, char *argv[] )
{
	CMyApp myApp = CMyApp(argc, argv);

	myApp.StartUp();
	myApp.CheckArgs();

	std::cout << "we are running from " << myApp.GetAppFolder() << std::endl;

	myApp.Run();
	myApp.Quit();
	
	return EXIT_SUCCESS;
}// ----------  end of function main  ----------


//--------------------------------------------------------------------------------------
//       Class:  CMyApp
//      Method:  CMyApp
// Description:  constructor
//--------------------------------------------------------------------------------------
CMyApp::CMyApp (int argc, char* argv[])
	: CApplication(argc, argv)

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
	return ;
}		// -----  end of method CMyApp::Do_CheckArgs  -----

void
CMyApp::Do_SetupProgramOptions (void)
{
	return ;
}		// -----  end of method CMyApp::Do_SetupProgramOptions  -----

