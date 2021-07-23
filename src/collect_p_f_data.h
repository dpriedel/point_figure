// =====================================================================================
// 
//       Filename:  collect_p_f_data.h
// 
//    Description:  Module to calculation Point & Figure data for a given symbol.
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

#ifndef  collect_p_f_data_INC
#define  collect_p_f_data_INC


#include <filesystem>
#include <memory>
#include <tuple>

namespace fs = std::filesystem;

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <date/date.h>
#include <spdlog/spdlog.h>


#include "DDecDouble.h"

// =====================================================================================
//        Class:  CMyApp
//  Description:  application specific stuff
// =====================================================================================
class CMyApp
{
public:
    // ====================  LIFECYCLE     =======================================
    CMyApp (int argc, char* argv[]);                             // constructor

    // use ctor below for testing with predefined options

    explicit CMyApp(const std::vector<std::string>& tokens);
    
    CMyApp() = delete;
	CMyApp(const CMyApp& rhs) = delete;
	CMyApp(CMyApp&& rhs) = delete;

    ~CMyApp() = default;

    static bool SignalReceived() { return had_signal_ ; }

    bool Startup();
    std::tuple<int, int, int> Run();
    void Shutdown();


    // ====================  ACCESSORS     =======================================

    // ====================  MUTATORS      =======================================

    // ====================  OPERATORS     =======================================

    CMyApp& operator=(const CMyApp& rhs) = delete;
    CMyApp& operator=(CMyApp&& rhs) = delete;

protected:

	//	Setup for parsing program options.

	void	SetupProgramOptions();
	void 	ParseProgramOptions(const std::vector<std::string>& tokens);

    void    ConfigureLogging();

	bool	CheckArgs ();
	void	Do_Quit ();


    // ====================  DATA MEMBERS  =======================================

private:

    static void HandleSignal(int signal);

    // ====================  DATA MEMBERS  =======================================

	po::positional_options_description	mPositional;			//	old style options
    std::unique_ptr<po::options_description> mNewOptions;    	//	new style options (with identifiers)
	po::variables_map					mVariableMap;

	int mArgc = 0;
	char** mArgv = nullptr;
	const std::vector<std::string> tokens_;
    std::string logging_level_{"information"};

    fs::path mInputPath;
    fs::path mOutputPath;
    fs::path log_file_path_name_;
    std::string mSymbol;
    std::string mDBName;

    std::shared_ptr<spdlog::logger> logger_;

    enum class source { unknown, file, stdin, network };
    enum class destination { unknown, DB, file, stdout };
    enum class mode { unknown, load, update };
    enum class interval { unknown, eod, sec1, sec5, min1, min5, live };
    enum class scale { unknown, arithmetic,log };

    source mSource;
    destination mDestination;
    mode mMode;
    interval mInterval;
    scale mScale;

    DprDecimal::DDecDouble mBoxSize;
    int mReversalBoxes;
    bool mInputIsPath;
    bool mOutputIsPath;

    static bool had_signal_;
}; // -----  end of class CMyApp  -----

#endif   // ----- #ifndef collect_p_f_data_INC  ----- 
