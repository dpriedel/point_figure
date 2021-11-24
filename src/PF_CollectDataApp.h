// =====================================================================================
// 
//       Filename:  PF_CollectDataApp.h
// 
//    Description:  Module to collect Point & Figure data for a given set of symbols.
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
#include <string>
#include <map>
#include <optional>

namespace fs = std::filesystem;

#include <boost/program_options.hpp>

namespace po = boost::program_options;

#include <date/date.h>
#include <spdlog/spdlog.h>

#include "DDecQuad.h"
#include "PF_Chart.h"

// =====================================================================================
//        Class:  PF_CollectDataApp
//  Description:  application specific stuff
// =====================================================================================
class PF_CollectDataApp
{
public:

    using PF_Data = std::map<std::string, PF_Chart>;

    // ====================  LIFECYCLE     =======================================
    PF_CollectDataApp (int argc, char* argv[]);                             // constructor

    // use ctor below for testing with predefined options

    explicit PF_CollectDataApp(const std::vector<std::string>& tokens);
    
    PF_CollectDataApp() = delete;
	PF_CollectDataApp(const PF_CollectDataApp& rhs) = delete;
	PF_CollectDataApp(PF_CollectDataApp&& rhs) = delete;

    ~PF_CollectDataApp() = default;

    static bool SignalReceived() { return had_signal_ ; }

    bool Startup();
    std::tuple<int, int, int> Run();
    void Shutdown();


    // ====================  ACCESSORS     =======================================

    // ====================  MUTATORS      =======================================

    // ====================  OPERATORS     =======================================

    PF_CollectDataApp& operator=(const PF_CollectDataApp& rhs) = delete;
    PF_CollectDataApp& operator=(PF_CollectDataApp&& rhs) = delete;

protected:

	//	Setup for parsing program options.

	void	SetupProgramOptions();
	void 	ParseProgramOptions(const std::vector<std::string>& tokens);

    void    ConfigureLogging();

	bool	CheckArgs ();
	void	Do_Quit ();

    void    LoadSymbolPriceDataCSV(const std::string& symbol, std::ifstream& input_file);
    std::optional<int> FindColumnIndex(std::string_view header, std::string_view column_name, char delim);

    // ====================  DATA MEMBERS  =======================================

private:

    static void HandleSignal(int signal);

    // ====================  DATA MEMBERS  =======================================

    PF_Data charts_;

	po::positional_options_description	positional_;			//	old style options
    std::unique_ptr<po::options_description> newoptions_;    	//	new style options (with identifiers)
	po::variables_map					variablemap_;

	int argc_ = 0;
	char** argv_ = nullptr;
	const std::vector<std::string> tokens_;
    std::string logging_level_{"information"};

    fs::path input_file_dirctory_;
    fs::path output_file_directory_;
    fs::path log_file_path_name_;
    std::string symbol_;
    std::vector<std::string> symbol_list_;
    std::string dbname_;
    std::string host_name_;
    int32_t host_port_;

    std::shared_ptr<spdlog::logger> logger_;

    enum class Source { e_unknown, e_file, e_streaming };
    enum class SourceFormat { e_unknown, e_csv, e_json };
    enum class Destination { e_unknown, e_DB, e_file };
    enum class Mode { e_unknown, e_load, e_update };
    enum class Interval { e_unknown, e_eod, e_sec1, e_sec5, e_min1, e_min5, e_live };

    std::string source_i;
    std::string source_format_i;
    std::string destination_i;
    std::string mode_i;
    std::string interval_i;
    std::string scale_i;
    std::string use_adjusted_i;

    Source source_ = Source::e_unknown;
    SourceFormat source_format_ = SourceFormat::e_csv;
    Destination destination_ = Destination::e_unknown;
    Mode mode_ = Mode::e_unknown;
    Interval interval_ = Interval::e_unknown;
    PF_Column::ColumnScale scale_ = PF_Column::ColumnScale::e_arithmetic;
    std::string price_fld_name_;

    DprDecimal::DDecQuad boxsize_;
    int32_t reversal_boxes_;
    bool input_is_path_;
    bool output_is_path_;

    static bool had_signal_;
}; // -----  end of class PF_CollectDataApp  -----



#endif   // ----- #ifndef PF_COLLECTDATAAPP_INC  ----- 
