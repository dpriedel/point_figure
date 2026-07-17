#ifndef PF_LOADERAPP_INC
#define PF_LOADERAPP_INC

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <decimal.hh>

#include "common/PF_AppBase.h"
#include "Eodhd.h"
#include "PF_Chart.h"
#include "PointAndFigureDB.h"
#include "Tiingo.h"
#include "utilities.h"

namespace fs = std::filesystem;

class PF_LoaderApp : public PF_AppBase
{
public:
    using PF_Charts = std::vector<std::pair<std::string, PF_Chart>>;

    PF_LoaderApp(int argc, char *argv[]);
    explicit PF_LoaderApp(const std::vector<std::string> &tokens);

    PF_LoaderApp() = delete;
    PF_LoaderApp(const PF_LoaderApp &) = delete;
    PF_LoaderApp(PF_LoaderApp &&) = delete;

    [[nodiscard]] const PF_Charts &GetCharts() const { return charts_; }

    bool Startup();
    std::tuple<int, int, int> Run();
    void Shutdown();

    PF_LoaderApp &operator=(const PF_LoaderApp &) = delete;
    PF_LoaderApp &operator=(PF_LoaderApp &&) = delete;

private:
    void SetupProgramOptions();
    bool CheckArgs();

    void Run_Load();
    std::tuple<int, int, int> Run_LoadFromDB();
    std::tuple<int, int, int> ProcessSymbolsFromDB(const std::vector<std::string> &symbol_list);

    [[nodiscard]] static PF_Chart LoadAndParsePriceDataJSON(const fs::path &symbol_file_name);
    void AddPriceDataToExistingChartCSV(PF_Chart &new_chart, const fs::path &update_file_name) const;
    [[nodiscard]] static std::optional<int> FindColumnIndex(std::string_view header, std::string_view column_name,
                                                            std::string_view delim);

    [[nodiscard]] decimal::Decimal ComputeATRForChart(const std::string &symbol) const;
    [[nodiscard]] decimal::Decimal ComputeATRForChartFromDB(const std::string &symbol) const;

    void ShutdownAndStoreOutputInFiles();
    void ShutdownAndStoreOutputInDB();

    // ====================  DATA MEMBERS

    PF_Charts charts_;

    fs::path new_data_input_directory_;
    fs::path input_chart_directory_;
    fs::path output_chart_directory_;
    fs::path output_graphs_directory_;

    std::string quote_host_name_;
    fs::path quote_host_api_key_;
    std::string quote_host_port_;
    std::string quotes_api_key_;

    enum class Destination : int32_t
    {
        e_unknown,
        e_DB,
        e_file
    };

    enum class GraphicsFormat : int32_t
    {
        e_unknown,
        e_svg,
        e_csv
    };

    enum class Interval : int32_t
    {
        e_unknown,
        e_eod,
        e_sec1,
        e_sec5,
        e_min1,
        e_min5,
        e_live
    };

    enum class QuoteDataSource : int32_t
    {
        e_unknown,
        e_Eodhd,
        e_Tiingo
    };

    enum class Source : int32_t
    {
        e_unknown,
        e_file,
        e_streaming,
        e_DB
    };

    enum class SourceFormat : int32_t
    {
        e_unknown,
        e_csv,
        e_json
    };

    enum class BoxsizeSource : int32_t
    {
        e_unknown,
        e_from_args,
        e_from_ATR,
        e_from_MinMax
    };

    std::string quote_data_source_i_;
    std::string graphics_format_i_;
    std::string destination_i_;
    std::string interval_i_;
    std::string source_format_i_;
    std::string new_data_source_i_;
    std::string symbol_list_i_;
    std::vector<std::string> scale_i_list_;
    std::vector<std::string> box_size_i_list_;

    Source new_data_source_ = Source::e_unknown;
    SourceFormat source_format_ = SourceFormat::e_csv;
    Destination destination_ = Destination::e_unknown;
    GraphicsFormat graphics_format_ = GraphicsFormat::e_unknown;
    BoxsizeSource boxsize_source_ = BoxsizeSource::e_unknown;
    QuoteDataSource quote_data_source_ = QuoteDataSource::e_unknown;
    Interval interval_ = Interval::e_unknown;

    std::chrono::year_month_day start_date_;
    std::chrono::year_month_day stop_date_;

    std::vector<std::string> symbol_list_;
    std::vector<BoxScale> scale_list_;
    std::vector<decimal::Decimal> box_size_list_;
    std::vector<int32_t> reversal_boxes_list_;

    std::string price_fld_name_;
    std::string trend_lines_;
    std::string begin_date_;
    std::string end_date_;
    std::string min_dollar_volume_;

    int64_t min_close_volume_ = 100'000;
    int32_t max_columns_for_graph_ = -1;
    int32_t number_of_days_history_for_ATR_ = 0;
    bool use_ATR_ = false;
    bool use_min_max_ = false;
};

#endif
