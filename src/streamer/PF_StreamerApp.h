#ifndef PF_STREAMERAPP_INC
#define PF_STREAMERAPP_INC

#include <chrono>
#include <memory>
#include <string>

using namespace std::chrono_literals;

#include "common/PF_AppBase.h"
#include "PF_Chart.h"
#include "Streamer.h"
#include "utilities.h"

class PF_StreamerApp : public PF_AppBase
{
public:
    using PF_Charts = std::vector<std::pair<std::string, PF_Chart>>;

    PF_StreamerApp(int argc, char *argv[]);
    explicit PF_StreamerApp(const std::vector<std::string> &tokens);

    bool Startup();
    void Run();
    void Shutdown();

protected:
    void SetupProgramOptions();

private:
    void Run_Streaming();
    void PrimeChartsForStreaming();
    void CollectStreamingData();
    void CollectStreamedData(const RemoteDataSource::PF_Data &update, PF_SignalType new_signal);
    void StreamedDataParser(RemoteDataSource::StreamerContext &streamer_context,
                            std::vector<RemoteDataSource::ProcessorContext> &processor_contexts,
                            std::map<std::string, int> &symbol_to_context_map);
    void ProcessUpdatesForSymbol(RemoteDataSource::ProcessorContext &processor_context);
    void Do_ProcessUpdatesForSymbol(const RemoteDataSource::PF_Data &update);

    [[nodiscard]] decimal::Decimal ComputeATRForChart(const std::string &symbol) const;

    // Resume functionality
    void LoadChartsFromFiles();
    void LoadStreamedPricesFromFiles();
    void LoadStreamedSummaryFromFile();
    void SaveStreamedPricesToFiles();
    void SaveStreamedSummaryToFile();

    PF_StreamedPrices streamed_prices_;
    PF_StreamedSummary streamed_summary_;

    PF_Charts charts_;

    std::chrono::time_point<std::chrono::system_clock> last_summary_draw_time_;
    std::map<std::string, std::chrono::time_point<std::chrono::system_clock>> last_draw_times_;
    const std::chrono::seconds minimum_delay_ = 2s;

    std::unique_ptr<RemoteDataSource> PF_streamer_;

    fs::path output_chart_directory_;
    fs::path output_graphs_directory_;

    std::string streaming_host_name_;
    fs::path streaming_host_api_key_;
    std::string streaming_host_port_;
    std::string quote_host_name_;
    fs::path quote_host_api_key_;
    std::string quote_host_port_;

    enum class StreamingSource : int32_t
    {
        e_unknown,
        e_Eodhd,
        e_Tiingo
    };

    enum class QuoteDataSource : int32_t
    {
        e_unknown,
        e_Eodhd,
        e_Tiingo
    };

    enum class GraphicsFormat : int32_t
    {
        e_unknown,
        e_svg,
        e_csv
    };

    std::string quote_data_source_i_;
    std::string streaming_data_source_i_;
    std::string quotes_api_key_;
    std::string streaming_api_key_;
    std::string graphics_format_i_;
    std::vector<std::string> scale_i_list_;
    std::vector<std::string> box_size_i_list_;

    StreamingSource streaming_data_source_ = StreamingSource::e_unknown;
    QuoteDataSource quote_data_source_ = QuoteDataSource::e_unknown;
    GraphicsFormat graphics_format_ = GraphicsFormat::e_unknown;

    std::vector<std::string> symbol_list_;
    std::vector<BoxScale> scale_list_;
    std::vector<decimal::Decimal> box_size_list_;
    std::vector<int32_t> reversal_boxes_list_;

    std::string price_fld_name_;
    std::string trend_lines_;

    int32_t max_columns_for_graph_ = -1;
    int32_t number_of_days_history_for_ATR_ = 0;
    bool use_ATR_ = false;
    bool resume_mode_ = false;

};

#endif
