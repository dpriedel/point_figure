// =====================================================================================
// 
//       Filename:  collect_p_f_data.h
// 
//    Description:  Module to calculation Point & Figure data for a given symbol.
// 
//        Version:  1.0
//        Created:  04/24/2013 02:06:24 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================

#include "CApplication.h"

// =====================================================================================
//        Class:  CMyApp
//  Description:  application specific stuff
// =====================================================================================
class CMyApp : public CApplication
{
	public:
		// ====================  LIFECYCLE     =======================================
		CMyApp (int argc, char* argv[]);                             // constructor
		~CMyApp(void);


		// ====================  ACCESSORS     =======================================

		// ====================  MUTATORS      =======================================

		// ====================  OPERATORS     =======================================

	protected:

		virtual void Do_StartUp(void);
		virtual void Do_CheckArgs(void);

		virtual void Do_SetupProgramOptions(void);

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================

}; // -----  end of class CMyApp  -----

