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
#include "DDecimal_16.h"

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
		virtual	void Do_ParseProgramOptions(void);
		virtual void Do_Run(void);

		// ====================  DATA MEMBERS  =======================================

	private:
		// ====================  DATA MEMBERS  =======================================
	
	fs::path mInputPath;
	fs::path mOutputPath;
	std::string mSymbol;
	std::string mDBName;

	enum class source { unknown, file, stdin, network };
	enum class destination { unknown, DB, file, stdout };
	enum class mode { unknown, load, update };
	enum class interval { unknown, eod, sec1, sec5, min1, min5 };
	enum class scale { unknown, arithmetic,log };

	source mSource;
	destination mDestination;
	mode mMode;
	interval mInterval;
	scale mScale;

	DDecimal<16> mBoxSize;
	int mReversalBoxes;
	bool mInputIsPath;
	bool mOutputIsPath;

}; // -----  end of class CMyApp  -----

