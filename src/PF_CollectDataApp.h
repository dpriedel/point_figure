// =====================================================================================
// 
//       Filename:  PF_CollectDataApp.h
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

#ifndef  PF_COLLECTDATAAPP_INC
#define  PF_COLLECTDATAAPP_INC


#include <filesystem>
#include <memory>
#include <tuple>

namespace fs = std::filesystem;

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <date/date.h>
#include <spdlog/spdlog.h>

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

	po::positional_options_description	positional_;			//	old style options
    std::unique_ptr<po::options_description> newoptions_;    	//	new style options (with identifiers)
	po::variables_map					variablemap_;

	int argc_ = 0;
	char** argv_ = nullptr;
	const std::vector<std::string> tokens_;
    std::string logging_level_{"information"};

    fs::path inputpath_;
    fs::path outputpath_;
    fs::path log_file_path_name_;
    std::string symbol_;
    std::string dbname_;

    std::shared_ptr<spdlog::logger> logger_;

    enum class source { unknown, file, stdin, network };
    enum class destination { unknown, DB, file, stdout };
    enum class mode { unknown, load, update };
    enum class interval { unknown, eod, sec1, sec5, min1, min5, live };
    enum class scale { unknown, arithmetic,log };

    source source_;
    destination destination_;
    mode mode_;
    interval interval_;
    scale scale_;

    int32_t boxsize_;
    int32_t reversalboxes_;
    bool inputispath_;
    bool outputispath_;

    static bool had_signal_;
}; // -----  end of class CMyApp  -----

// custom fmtlib formatter for filesytem paths

template <> struct fmt::formatter<std::filesystem::path>: formatter<std::string> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(const std::filesystem::path& p, FormatContext& ctx) {
    std::string f_name = p.string();
    return formatter<std::string>::format(f_name, ctx);
  }
};

//// custom fmtlib formatter for date year_month_day
//
//template <> struct fmt::formatter<date::year_month_day>: formatter<std::string> {
//  // parse is inherited from formatter<string_view>.
//  template <typename FormatContext>
//  auto format(date::year_month_day d, FormatContext& ctx) {
//    std::string s_date = date::format("%Y-%m-%d", d);
//    return formatter<std::string>::format(s_date, ctx);
//  }
//};


#endif   // ----- #ifndef PF_COLLECTDATAAPP_INC  ----- 
