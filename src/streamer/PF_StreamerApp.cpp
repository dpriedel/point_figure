#include "PF_StreamerApp.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <print>
#include <queue>
#include <ranges>
#include <regex>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace rng = std::ranges;
namespace fs  = std::filesystem;
namespace vws = std::ranges::views;

#include <boost/assert.hpp>
#include <json/json.h>

#include "ConstructChartGraphic.h"
#include "Eodhd.h"
#include "Tiingo.h"
#include "utilities.h"

// =====================================================================================
//        Class:  PF_StreamerApp
//  Description:  application specific stuff for streaming mode
// =====================================================================================

PF_StreamerApp::PF_StreamerApp(int argc, char *argv[])
    : PF_AppBase{argc, argv}
{
    app_.description("Point & Figure streaming charts: real-time chart updates during market hours.");
    SetupProgramOptions();
}

PF_StreamerApp::PF_StreamerApp(const std::vector<std::string> &tokens)
    : PF_AppBase{tokens}
{
    app_.description("Point & Figure streaming charts: real-time chart updates during market hours.");
    SetupProgramOptions();
}

void PF_StreamerApp::SetupProgramOptions()
{
    // Symbol options
    app_.add_option("-s,--symbol", symbol_list_, "Symbol to stream.")
        ->expected(1, -1)
        ->check(CLI::NonexistentPath);

    app_.add_option("--symbol-list", symbol_list_i_, "Comma-separated list of symbols to stream.");

    // Chart parameters
    app_.add_option("-b,--boxsize", box_size_i_list_, "Box size for chart.")
        ->expected(1, -1)
        ->check(CLI::Range(0.001, 1000.0));

    app_.add_option("-r,--reversal", reversal_boxes_list_, "Number of reversal boxes.")
        ->expected(1, -1)
        ->check(CLI::Range(1, 10));

    app_.add_option("--scale", scale_i_list_, "Chart scale: 'linear' or 'percent'.")
        ->expected(1, -1)
        ->check(CLI::IsMember({"linear", "percent"}));

    // Output options
    app_.add_option("--output-chart-dir", output_chart_directory_, "Directory for chart JSON files.");
    app_.add_option("--output-graph-dir", output_graphs_directory_, "Directory for SVG graphics.");

    app_.add_option("--graphics-format", graphics_format_i_, "Graphics format: 'svg' or 'csv'.")
        ->default_val("svg")
        ->check(CLI::IsMember({"svg", "csv"}));

    app_.add_option("--max-graphic-cols", max_columns_for_graph_, "Maximum columns in graphic (-1 = all).")
        ->default_val(-1)
        ->check(CLI::Range(-1, 200));

    app_.add_option("--show-trend-lines", trend_lines_, "Show trend lines: 'no', 'data', or 'angle'.")
        ->default_val("no")
        ->check(CLI::IsMember({"no", "data", "angle"}));

    // Streaming Options
    app_.add_option("--streaming-host", streaming_host_name_, "Web site to stream from.");
    app_.add_option("--streaming-port", streaming_host_port_, "Port for streaming.")
        ->default_val("443");

    // Quote/ATR Options
    app_.add_option("--quote-host", quote_host_name_, "Web site to download ATR history from.");
    app_.add_option("--quote-port", quote_host_port_, "Port for quotes.")
        ->default_val("443");

    app_.add_option("--price-fld-name", price_fld_name_, "Price field name.")
        ->default_val("close");

    // Database Options (for DB destination)
    app_.add_option("--db-host", db_params_.host_name_, "Database host.")
        ->default_val("localhost");
    app_.add_option("--db-port", db_params_.port_number_, "Database port.")
        ->default_val(5432);
    app_.add_option("--db-user", db_params_.user_name_, "Database user name.");
    app_.add_option("--db-name", db_params_.db_name_, "Database name.");
    app_.add_option("--db-mode", db_params_.PF_db_mode_, "'test' or 'live' schema.")
        ->default_val("test")
        ->check(CLI::IsMember({"test", "live"}));

    app_.add_option("--stock-db-data-source", db_params_.stock_db_data_source_, "Table containing symbol data.")
        ->default_val("new_stock_data.current_data");

    // Data source options
    app_.add_option("--quote-data-source", quote_data_source_i_, "ATR quotes data source.")
        ->check(CLI::IsMember({"Eodhd", "Tiingo"}));
    app_.add_option("--streaming-data-source", streaming_data_source_i_, "Streaming data source.")
        ->check(CLI::IsMember({"Eodhd", "Tiingo"}));

    // Config and Keys
    app_.add_option("--config-dir", PF_CollectDataConfigDir_, "Path to config directory.")
        ->check(CLI::ExistingDirectory)
        ->check([](const std::string &path_str) {
            if (std::filesystem::is_empty(path_str))
            {
                return "Directory 'config-dir': '" + path_str + "' contains no files.";
            }
            return std::string{};
        });
    app_.add_option("--quote-api-key", quote_host_api_key_, "File containing quotes API key.");
    app_.add_option("--streaming-api-key", streaming_host_api_key_, "File containing streaming API key.");

    // Flags
    auto atr_or_minmax = app_.add_option_group("atr", "Specify 'use-ATR' for ATR-based box size.");
    atr_or_minmax->add_flag("--use-ATR", use_ATR_, "Compute ATR and use to compute box size.");
    atr_or_minmax->add_flag("--use-MinMax", use_min_max_, "Use MinMax-based box size calculation.");

    app_.add_flag("--resume", resume_mode_, "Resume streaming from saved data files.");

    // Compatibility options (accepted but ignored for streamer)
    app_.add_option("--new-data-source", new_data_source_i_, "Data source (ignored for streamer).");
    app_.add_option("--new-data-dir", new_data_input_directory_, "Data directory (ignored for streamer).");
    app_.add_option("--source-format", source_format_i_, "Source format (ignored for streamer).");
    app_.add_option("--destination", destination_i_, "Destination (ignored for streamer).");
    app_.add_option("--exchange-list", exchange_list_, "Exchange list (ignored for streamer).")
        ->delimiter(',')
        ->transform([](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        });
    app_.add_option("--interval", interval_i_, "Interval (ignored for streamer).");

    // Logging options
    app_.add_option("--log-path", log_file_path_name_, "Path name for log file.");
    app_.add_option("-l,--log-level", logging_level_, "Logging level: 'none|error|information|debug'.")
        ->default_val("information")
        ->check(CLI::IsMember({"none", "error", "information", "debug"}));

    // Final validation
    app_.callback([this]() {
        if (argc_ > 1)
        {
            if (!use_min_max_ && box_size_i_list_.empty())
            {
                throw CLI::ValidationError("Box size must be specified or --use-MinMax must be used.");
            }
            if (reversal_boxes_list_.empty())
            {
                throw CLI::ValidationError("Reversal boxes must be provided.");
            }
        }
    });

    app_.failure_message(CLI::FailureMessage::help);
}

bool PF_StreamerApp::Startup()
{
    constexpr const char *time_fmt = "\n\n*** Begin run {:%a, %b %d, %Y at %I:%M:%S %p %Z}  ***\n";
    spdlog::info(std::format("\n\n*** Starting run {} ***\n",
                             std::chrono::current_zone()->to_local(std::chrono::system_clock::now())));
    bool result{true};
    try
    {
        if (tokens_.empty() && argc_ <= 1)
        {
            std::cout << app_.help();
            return false;
        }
        ParseProgramOptions(tokens_);
        ConfigureLogging();

        // Parse box sizes
        rng::for_each(box_size_i_list_, [this](const auto &b) { this->box_size_list_.emplace_back(decimal::Decimal{b}); });

        // Parse scales
        if (scale_i_list_.empty())
        {
            scale_i_list_.emplace_back("linear");
        }
        rng::for_each(scale_i_list_, [](const auto &scale) {
            BOOST_ASSERT_MSG(
                scale == "linear" || scale == "percent",
                std::format("\nChart scale must be: 'linear' or 'percent': {}", scale).c_str());
        });
        rng::for_each(scale_i_list_, [this](const auto &scale_i) {
            this->scale_list_.emplace_back(scale_i == "linear" ? BoxScale::e_Linear : BoxScale::e_Percent);
        });

        // Parse graphics format
        graphics_format_ = graphics_format_i_ == "svg" ? GraphicsFormat::e_svg : GraphicsFormat::e_csv;

        // Parse quote data source
        if (!quote_data_source_i_.empty())
        {
            quote_data_source_ = quote_data_source_i_ == "Tiingo" ? QuoteDataSource::e_Tiingo : QuoteDataSource::e_Eodhd;
        }

        // Parse streaming data source
        if (!streaming_data_source_i_.empty())
        {
            streaming_data_source_ =
                streaming_data_source_i_ == "Tiingo" ? StreamingSource::e_Tiingo : StreamingSource::e_Eodhd;
        }

        // Validate streaming source
        BOOST_ASSERT_MSG(!streaming_host_name_.empty(), "\nMust provide 'streaming-host'.");
        BOOST_ASSERT_MSG(streaming_data_source_ != StreamingSource::e_unknown,
                         "\nMust provide 'streaming-data-source'.");

        // Read streaming API key
        BOOST_ASSERT_MSG(!streaming_host_api_key_.empty(),
                         "Must specify a streaming source API key file.");
        BOOST_ASSERT_MSG(
            fs::exists(PF_CollectDataConfigDir_ / streaming_host_api_key_),
            std::format("\nCan't find streaming source api key file: {}", streaming_host_api_key_).c_str());

        std::ifstream streaming_key_file(PF_CollectDataConfigDir_ / streaming_host_api_key_);
        streaming_key_file >> streaming_api_key_;

        // Read quote API key if using ATR
        if (use_ATR_)
        {
            BOOST_ASSERT_MSG(!quote_host_api_key_.empty(), "Must specify a quote source API key file for ATR.");
            BOOST_ASSERT_MSG(fs::exists(PF_CollectDataConfigDir_ / quote_host_api_key_),
                             std::format("\nCan't find ATR quotes source api key file: {}", quote_host_api_key_)
                                 .c_str());

            std::ifstream quotes_key_file(PF_CollectDataConfigDir_ / quote_host_api_key_);
            quotes_key_file >> quotes_api_key_;
        }

        // Setup output directories
        BOOST_ASSERT_MSG(!output_chart_directory_.empty(), "\nMust specify 'output-chart-dir'.");
        if (!fs::exists(output_chart_directory_))
        {
            fs::create_directories(output_chart_directory_);
        }

        if (output_graphs_directory_.empty())
        {
            output_graphs_directory_ = output_chart_directory_;
        }

        if (graphics_format_ == GraphicsFormat::e_svg)
        {
            BOOST_ASSERT_MSG(!output_graphs_directory_.empty(), "\nMust specify 'output-graph-dir'.");
            if (!fs::exists(output_graphs_directory_))
            {
                fs::create_directories(output_graphs_directory_);
            }
        }

        // Parse symbol-list if provided
        if (!symbol_list_i_.empty() && symbol_list_i_ != "ALL")
        {
            rng::for_each(split_string<std::string>(symbol_list_i_, ","),
                          [this](const auto sym) { symbol_list_.push_back(sym); });
            rng::sort(symbol_list_);
            const auto [first, last] = rng::unique(symbol_list_);
            symbol_list_.erase(first, last);
        }

        // Uppercase symbols
        rng::for_each(symbol_list_, [](auto &symbol) { rng::for_each(symbol, [](char &c) { c = std::toupper(c); }); });

        // Print chart combinations
        auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);
        rng::for_each(params, [](const auto &x) {
            std::cout << std::format("{}\t{}\t{}\t{}\n", std::get<0>(x), std::get<1>(x).format("f"), std::get<2>(x),
                                     std::get<3>(x));
        });
        std::cout << std::endl;

    }
    catch (const std::exception &e)
    {
        spdlog::error(std::format("Problem in startup: {}\n", e.what()));
        result = false;
    }
    catch (...)
    {
        spdlog::error("Unexpected problem during Startup processing\n");
        result = false;
    }
    return result;
}

void PF_StreamerApp::Run()
{
    number_of_days_history_for_ATR_ = 20;
    Run_Streaming();
}

void PF_StreamerApp::Shutdown()
{
    // Save streamed data for resume functionality
    SaveStreamedPricesToFiles();
    SaveStreamedSummaryToFile();

    for (const auto &[symbol, chart] : charts_)
    {
        if (chart.empty())
            continue;
        try
        {
            fs::path output_file_name = output_chart_directory_ / chart.MakeChartFileName("", "json");
            chart.ConvertChartToJsonAndWriteToFile(output_file_name);

            if (graphics_format_ == GraphicsFormat::e_svg)
            {
                fs::path graph_file_path = output_graphs_directory_ / chart.MakeChartFileName("", "svg");
                ConstructCDPFChartGraphicAndWriteToFile(
                    chart, graph_file_path, streamed_prices_[chart.GetSymbol()], trend_lines_,
                    X_AxisFormat::e_show_time);
            }
            else
            {
                fs::path graph_file_path = output_graphs_directory_ / chart.MakeChartFileName("", "csv");
                chart.ConvertChartToTableAndWriteToFile(graph_file_path, X_AxisFormat::e_show_time);
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Problem in shutdown: {} for chart: {}.\nTrying to complete shutdown.", e.what(),
                                       chart.MakeChartFileName("", "")));
        }
    }

    if (graphics_format_ == GraphicsFormat::e_svg)
    {
        fs::path summary_graphic_path = output_graphs_directory_ / "PF_StreamingSummary.svg";
        ConstructCDSummaryGraphic(streamed_summary_, summary_graphic_path);
    }

    spdlog::info(std::format("\n\n*** End run {}  ***\n",
                             std::chrono::current_zone()->to_local(std::chrono::system_clock::now())));

    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void PF_StreamerApp::Run_Streaming()
{
    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    auto current_local_time = std::chrono::zoned_seconds(std::chrono::current_zone(),
                                                         floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    auto market_status = GetUS_MarketStatus(
        std::string_view{std::chrono::current_zone()->name()}, current_local_time.get_local_time());

    if (market_status != US_MarketStatus::e_NotOpenYet && market_status != US_MarketStatus::e_OpenForTrading)
    {
        std::cout << "Market not open for trading now so we can't stream quotes.\n";
        return;
    }

    if (market_status == US_MarketStatus::e_NotOpenYet)
    {
        std::cout << "Market not open for trading YET so we'll wait." << std::endl;
    }

    // === RESUME MODE: Load existing data ===
    if (resume_mode_)
    {
        LoadChartsFromFiles();
        LoadStreamedPricesFromFiles();
        LoadStreamedSummaryFromFile();
        spdlog::info("Resume mode: loaded existing data");
    }
    else
    {
        // === NORMAL MODE: Create new charts ===
        std::map<std::string, decimal::Decimal> cache;

        for (const auto &val : params)
        {
            const auto &symbol = std::get<PF_Chart::e_symbol>(val);
            try
            {
                PF_Chart new_chart;
                decimal::Decimal atr;
                if (use_ATR_)
                {
                    atr = cache.contains(symbol) ? cache[symbol] : (cache[symbol] = ComputeATRForChart(symbol));
                    new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
                else
                {
                    atr = 0;
                    new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
                charts_.emplace_back(std::make_pair(symbol, new_chart));
            }
            catch (const std::exception &e)
            {
                spdlog::error(std::format("Unable to compute ATR for: '{}' because: {}.\n", symbol, e.what()));
            }
        }

        for (const auto &symbol : symbol_list_)
        {
            streamed_prices_[symbol] = {};
            streamed_summary_[symbol] = {};
        }

        PrimeChartsForStreaming();
    }

    // set up time delays between drawing chart updates
    auto now = std::chrono::system_clock::now();
    for (const auto &symbol : symbol_list_)
    {
        last_draw_times_[symbol] = now;
    }
    last_summary_draw_time_ = now;

    CollectStreamingData();
}

void PF_StreamerApp::PrimeChartsForStreaming()
{
    auto today = std::chrono::year_month_day{floor<std::chrono::days>(std::chrono::system_clock::now())};
    std::chrono::year which_year = today.year();
    auto holidays = MakeHolidayList(which_year);
    rng::copy(MakeHolidayList(--which_year), std::back_inserter(holidays));

    auto current_local_time = std::chrono::zoned_seconds(std::chrono::current_zone(),
                                                         floor<std::chrono::seconds>(std::chrono::system_clock::now()));
    auto market_status = GetUS_MarketStatus(
        std::string_view{std::chrono::current_zone()->name()}, current_local_time.get_local_time());

    std::unique_ptr<RemoteDataSource> history_getter;
    if (quote_data_source_ == QuoteDataSource::e_Eodhd)
    {
        history_getter = std::make_unique<Eodhd>(Eodhd::Host{quote_host_name_}, Eodhd::Port{quote_host_port_},
                                                 Eodhd::APIKey{quotes_api_key_}, Eodhd::Prefix{});
    }
    else
    {
        history_getter = std::make_unique<Tiingo>(Tiingo::Host{quote_host_name_}, Tiingo::Port{quote_host_port_},
                                                  Tiingo::APIKey{quotes_api_key_}, Tiingo::Prefix{"/iex"});
    }

    if (market_status == US_MarketStatus::e_NotOpenYet)
    {
        std::map<std::string, std::vector<StockDataRecord>> cache;

        for (auto &[symbol, chart] : charts_)
        {
            const auto history =
                cache.contains(symbol)
                    ? cache[symbol]
                    : (cache[symbol] = history_getter->GetMostRecentTickerData(
                           symbol, today, 2,
                           price_fld_name_.starts_with("adj") ? UseAdjusted::e_Yes : UseAdjusted::e_No, &holidays));
            chart.AddValue(history[0].close_,
                           std::chrono::clock_cast<std::chrono::utc_clock>(current_local_time.get_sys_time()));
        }

        for (const auto &[symbol, h] : cache)
        {
            try
            {
                streamed_summary_[symbol].opening_price_ = dec2dbl(h[0].close_);
                streamed_summary_[symbol].latest_price_ = streamed_summary_[symbol].opening_price_;
            }
            catch (const std::exception &e)
            {
                spdlog::error(std::format(
                    "Problem initializing streamed summary with streaming data for symbol: {} because: {}", symbol,
                    e.what()));
            }
        }
    }
    else if (market_status == US_MarketStatus::e_OpenForTrading)
    {
        history_getter->UseSymbols(symbol_list_);
        auto history = history_getter->GetTopOfBookAndLastClose();

        const auto close_time_stamp =
            std::chrono::clock_cast<std::chrono::utc_clock>(GetUS_MarketOpenTime(today).get_sys_time() - std::chrono::seconds{60});
        const auto open_time_stamp =
            std::chrono::clock_cast<std::chrono::utc_clock>(GetUS_MarketOpenTime(today).get_sys_time());

        for (const auto &h : history)
        {
            rng::for_each(
                charts_ | vws::filter([&h](auto &symbol_and_chart) { return symbol_and_chart.first == h.symbol_; }),
                [&](auto &symbol_and_chart) {
                    try
                    {
                        symbol_and_chart.second.AddValue(h.previous_close_, close_time_stamp);

                        if (h.open_ != 0)
                        {
                            symbol_and_chart.second.AddValue(h.open_, open_time_stamp);
                            symbol_and_chart.second.AddValue(h.last_, h.time_stamp_nsecs_);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        spdlog::error(std::format(
                            "Problem initializing PF_Chart with streaming data for symbol: {} because: {}", h.symbol_,
                            e.what()));
                    }
                });
        }

        for (const auto &h : history)
        {
            try
            {
                streamed_summary_[h.symbol_].opening_price_ = dec2dbl(h.previous_close_);

                if (h.last_ == 0)
                {
                    streamed_summary_[h.symbol_].latest_price_ = dec2dbl(h.previous_close_);
                }
                else
                {
                    streamed_summary_[h.symbol_].latest_price_ = dec2dbl(h.last_);
                }
            }
            catch (const std::exception &e)
            {
                spdlog::error(std::format(
                    "Problem initializing streamed summary with streaming data for symbol: {} because: {}", h.symbol_,
                    e.what()));
            }
        }
    }
}

void PF_StreamerApp::CollectStreamingData()
{
    std::cout << std::format("starting {} streaming.",
                             streaming_data_source_ == StreamingSource::e_Eodhd ? "Eodhd" : "Tiingo")
              << std::endl;

    had_signal_ = false;

    auto today = std::chrono::year_month_day{floor<std::chrono::days>(std::chrono::system_clock::now())};
    auto local_market_close = std::chrono::zoned_seconds(
        std::chrono::current_zone(), GetUS_MarketCloseTime(today).get_sys_time() + 2min);

    RemoteDataSource::StreamerContext streamer_context;
    std::vector<RemoteDataSource::ProcessorContext> processor_contexts(symbol_list_.size());

    std::map<std::string, int> symbol_to_context_map;
    int indx = 0;
    for (const auto &symbol : symbol_list_)
    {
        symbol_to_context_map[symbol] = indx++;
    }

    std::vector<std::thread> processor_threads;
    for (auto &context : processor_contexts)
    {
        processor_threads.emplace_back(&PF_StreamerApp::ProcessUpdatesForSymbol, this, std::ref(context));
    }

    auto parsing_task = std::async(std::launch::async, &PF_StreamerApp::StreamedDataParser, this,
                                   std::ref(streamer_context), std::ref(processor_contexts),
                                   std::ref(symbol_to_context_map));

    auto timer_task = std::async(std::launch::async, &PF_AppBase::WaitForTimer, local_market_close);

    try
    {
        if (streaming_data_source_ == StreamingSource::e_Eodhd)
        {
            PF_streamer_ = std::make_unique<Eodhd>(Eodhd::Host{streaming_host_name_}, Eodhd::Port{streaming_host_port_},
                                                   Eodhd::APIKey{streaming_api_key_},
                                                   Eodhd::Prefix{std::string("/ws/us?api_token=") + streaming_api_key_});
        }
        else
        {
            PF_streamer_ = std::make_unique<Tiingo>(Tiingo::Host{streaming_host_name_}, Tiingo::Port{streaming_host_port_},
                                                    Tiingo::APIKey{streaming_api_key_}, Tiingo::Prefix{"/iex"});
        }

        PF_streamer_->UseSymbols(symbol_list_);

        auto streaming_task = std::async(std::launch::async, &RemoteDataSource::StreamData, PF_streamer_.get(),
                                         &had_signal_, std::ref(streamer_context));
        streaming_task.get();
    }
    catch (std::exception &e)
    {
        spdlog::error(std::format("Problem with {} streaming. Message: {}",
                                  streaming_data_source_ == StreamingSource::e_Eodhd ? "Eodhd" : "Tiingo", e.what()));
        had_signal_ = true;
    }

    streamer_context.done_ = true;
    streamer_context.cv_.notify_one();
    parsing_task.get();

    for (auto &context : processor_contexts)
    {
        context.done_ = true;
        context.cv_.notify_one();
    }

    for (auto &thread : processor_threads)
    {
        thread.join();
    }

    timer_task.get();
    spdlog::debug("got here after timer expired");
}

void PF_StreamerApp::StreamedDataParser(RemoteDataSource::StreamerContext &streamer_context,
                                        std::vector<RemoteDataSource::ProcessorContext> &processor_contexts,
                                        std::map<std::string, int> &symbol_to_context_map)
{
    while (true)
    {
        std::string new_data;
        {
            std::unique_lock<std::mutex> lock(streamer_context.mtx_);

            streamer_context.cv_.wait(lock, [&streamer_context] {
                return !streamer_context.streamed_data_.empty() || streamer_context.done_;
            });

            if (streamer_context.done_ && streamer_context.streamed_data_.empty())
            {
                std::println("Consumer/Producer: Work complete.");
                break;
            }

            if (streamer_context.streamed_data_.empty())
            {
                continue;
            }
            new_data = std::move(streamer_context.streamed_data_.front());
            streamer_context.streamed_data_.pop();
        }

        try
        {
            const RemoteDataSource::PF_Data extracted_data = PF_streamer_->ExtractStreamedData(new_data);
            if (extracted_data.ticker_.empty())
            {
                continue;
            }
            auto &processor_ctx = processor_contexts[symbol_to_context_map.at(extracted_data.ticker_)];

            {
                std::lock_guard<std::mutex> lock(processor_ctx.mtx_);
                processor_ctx.extracted_data_.emplace(extracted_data);
            }

            processor_ctx.cv_.notify_one();
        }
        catch (const std::exception &e)
        {
            spdlog::error("Error parsing websocket data: {}\n{}", new_data, e.what());
        }
    }
}

void PF_StreamerApp::ProcessUpdatesForSymbol(RemoteDataSource::ProcessorContext &processor_context)
{
    std::exception_ptr ep = nullptr;

    while (true)
    {
        RemoteDataSource::PF_Data pf_data;
        {
            std::unique_lock<std::mutex> lock(processor_context.mtx_);

            processor_context.cv_.wait(lock, [&processor_context] {
                return !processor_context.extracted_data_.empty() || processor_context.done_;
            });

            if (processor_context.done_ && processor_context.extracted_data_.empty())
            {
                std::println("Consumer: Work complete.");
                break;
            }

            if (processor_context.extracted_data_.empty())
            {
                continue;
            }
            pf_data = std::move(processor_context.extracted_data_.front());
            processor_context.extracted_data_.pop();
        }

        try
        {
            Do_ProcessUpdatesForSymbol(pf_data);
        }
        catch (std::system_error &e)
        {
            spdlog::error(e.what());
            auto ec = e.code();
            spdlog::error("Category: {}. Value: {}. Message: {}.", ec.category().name(), ec.value(), ec.message());

            if (!ep)
            {
                ep = std::current_exception();
            }
            continue;
        }
        catch (std::exception &e)
        {
            spdlog::error(e.what());

            if (!ep)
            {
                ep = std::current_exception();
            }
            continue;
        }
        catch (...)
        {
            spdlog::error("Unknown problem with an async download process");

            if (!ep)
            {
                ep = std::current_exception();
            }
            continue;
        }
    }
    if (ep)
    {
        std::rethrow_exception(ep);
    }
}

void PF_StreamerApp::Do_ProcessUpdatesForSymbol(const RemoteDataSource::PF_Data &update)
{
    if (update.last_price_ == -1 || update.last_size_ == 1)
    {
        return;
    }

    std::vector<PF_Chart *> need_to_update_graph;
    PF_SignalType new_signal{PF_SignalType::e_unknown};

    rng::for_each(
        charts_ | vws::filter([&update](const auto &symbol_and_chart) { return symbol_and_chart.first == update.ticker_; }),
        [this, &need_to_update_graph, &update, &new_signal](auto &symbol_and_chart) {
            try
            {
                auto chart_changed = symbol_and_chart.second.AddValue(
                    update.last_price_, PF_Column::TmPt{update.time_stamp_nanoseconds_utc_});
                if (chart_changed != PF_Column::Status::e_Ignored)
                {
                    need_to_update_graph.push_back(&symbol_and_chart.second);
                    if (chart_changed == PF_Column::Status::e_AcceptedWithSignal)
                    {
                        new_signal = symbol_and_chart.second.GetMostRecentSignal().value().signal_type_;
                    }
                }
            }
            catch (std::exception &e)
            {
                spdlog::error(std::format("Problem adding streamed value to chart for symbol: {} because: {}.",
                                          update.ticker_, e.what()));
            }
        });

    CollectStreamedData(update, new_signal);

    rng::sort(need_to_update_graph);
    const auto [first, last] = rng::unique(need_to_update_graph);
    need_to_update_graph.erase(first, last);

    auto now = std::chrono::system_clock::now();
    for (const PF_Chart *chart : need_to_update_graph)
    {
        auto last_drawn = last_draw_times_.at(chart->GetSymbol());
        if (now < last_drawn + minimum_delay_)
        {
            continue;
        }
        try
        {
            fs::path graph_file_path = output_graphs_directory_ / (chart->MakeChartFileName("", "svg"));
            ConstructCDPFChartGraphicAndWriteToFile(*chart, graph_file_path, streamed_prices_[chart->GetSymbol()],
                                                    trend_lines_, X_AxisFormat::e_show_time);

            fs::path chart_file_path = output_chart_directory_ / (chart->MakeChartFileName("", "json"));
            chart->ConvertChartToJsonAndWriteToFile(chart_file_path);
            last_draw_times_.at(chart->GetSymbol()) = now;
        }
        catch (std::exception &e)
        {
            spdlog::error(std::string("Problem creating graphic for updated streamed value: ") + chart->GetChartBaseName() +
                          " " + e.what());
        }
    }

    if (now > last_summary_draw_time_ + minimum_delay_)
    {
        fs::path summary_graphic_path = output_graphs_directory_ / "PF_StreamingSummary.svg";
        ConstructCDSummaryGraphic(streamed_summary_, summary_graphic_path);
        last_summary_draw_time_ = now;
    }
}

void PF_StreamerApp::CollectStreamedData(const RemoteDataSource::PF_Data &update, PF_SignalType new_signal)
{
    const auto new_time_stamp =
        std::chrono::duration_cast<std::chrono::seconds>(update.time_stamp_nanoseconds_utc_.time_since_epoch()).count();
    if (!streamed_prices_[update.ticker_].timestamp_seconds_.empty())
    {
        if (new_time_stamp > streamed_prices_[update.ticker_].timestamp_seconds_.back())
        {
            streamed_prices_[update.ticker_].timestamp_seconds_.push_back(new_time_stamp);
            streamed_prices_[update.ticker_].price_.push_back(dec2dbl(update.last_price_));
            streamed_prices_[update.ticker_].signal_type_.push_back(std::to_underlying(new_signal));
        }
        else
        {
            streamed_prices_[update.ticker_].price_.back() = dec2dbl(update.last_price_);
            if (new_signal != PF_SignalType::e_unknown)
            {
                streamed_prices_[update.ticker_].signal_type_.back() = std::to_underlying(new_signal);
            }
        }
    }
    else
    {
        streamed_prices_[update.ticker_].timestamp_seconds_.push_back(new_time_stamp);
        streamed_prices_[update.ticker_].price_.push_back(dec2dbl(update.last_price_));
        streamed_prices_[update.ticker_].signal_type_.push_back(std::to_underlying(new_signal));
    }

    streamed_summary_[update.ticker_].latest_price_ = dec2dbl(update.last_price_);
}

decimal::Decimal PF_StreamerApp::ComputeATRForChart(const std::string &symbol) const
{
    std::unique_ptr<RemoteDataSource> history_getter;
    if (quote_data_source_ == QuoteDataSource::e_Eodhd)
    {
        history_getter = std::make_unique<Eodhd>(Eodhd::Host{quote_host_name_}, Eodhd::Port{quote_host_port_},
                                                 Eodhd::APIKey{quotes_api_key_}, Eodhd::Prefix{});
    }
    else
    {
        history_getter = std::make_unique<Tiingo>(Tiingo::Host{quote_host_name_}, Tiingo::Port{quote_host_port_},
                                                  Tiingo::APIKey{quotes_api_key_}, Tiingo::Prefix{});
    }

    std::chrono::year_month_day today{--floor<std::chrono::days>(std::chrono::system_clock::now())};
    auto holidays = MakeHolidayList(today.year());
    rng::copy(MakeHolidayList(--(today.year())), std::back_inserter(holidays));

    const auto history = history_getter->GetMostRecentTickerData(symbol, today, number_of_days_history_for_ATR_ + 1,
                                                                 UseAdjusted::e_Yes, &holidays);

    return ComputeATR(symbol, history, number_of_days_history_for_ATR_);
}

void PF_StreamerApp::LoadChartsFromFiles()
{
    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    for (const auto &val : params)
    {
        const auto &symbol = std::get<PF_Chart::e_symbol>(val);
        fs::path chart_file_path = output_chart_directory_ / MakeChartNameFromParams(val, "", "json");

        try
        {
            if (fs::exists(chart_file_path))
            {
                PF_Chart loaded_chart;
                PF_Chart::LoadChartFromJSONPF_ChartFile(loaded_chart, chart_file_path);
                if (max_columns_for_graph_ != 0)
                {
                    loaded_chart.SetMaxGraphicColumns(max_columns_for_graph_);
                }
                charts_.emplace_back(std::make_pair(symbol, std::move(loaded_chart)));
                spdlog::info("Loaded chart from: {}", chart_file_path.string());
            }
            else
            {
                decimal::Decimal atr = use_ATR_ ? ComputeATRForChart(symbol) : 0;
                PF_Chart new_chart = use_ATR_
                                         ? PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_}
                                         : PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
                spdlog::info("No saved chart for {}, creating new one", symbol);
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error("Corrupt chart file {} for symbol {}: {}. Exiting.", chart_file_path.string(), symbol,
                          e.what());
            throw;
        }
    }
}

void PF_StreamerApp::LoadStreamedPricesFromFiles()
{
    for (const auto &symbol : symbol_list_)
    {
        fs::path prices_file = output_chart_directory_ / (symbol + "_streamed_prices.json");

        if (fs::exists(prices_file))
        {
            try
            {
                Json::Value streamed_data;
                std::ifstream in(prices_file);
                in >> streamed_data;
                in.close();

                StreamedPrices prices;
                if (streamed_data.isMember("timestamp_seconds") && streamed_data["timestamp_seconds"].isArray())
                {
                    const auto &ts_array = streamed_data["timestamp_seconds"];
                    for (const auto &val : ts_array)
                    {
                        prices.timestamp_seconds_.push_back(val.asInt64());
                    }
                }
                if (streamed_data.isMember("price") && streamed_data["price"].isArray())
                {
                    const auto &price_array = streamed_data["price"];
                    for (const auto &val : price_array)
                    {
                        prices.price_.push_back(val.asDouble());
                    }
                }
                if (streamed_data.isMember("signal_type") && streamed_data["signal_type"].isArray())
                {
                    const auto &signal_array = streamed_data["signal_type"];
                    for (const auto &val : signal_array)
                    {
                        prices.signal_type_.push_back(val.asInt());
                    }
                }

                streamed_prices_[symbol] = prices;
                spdlog::info("Loaded streamed prices for {} from {}", symbol, prices_file.string());
            }
            catch (const Json::Exception &e)
            {
                spdlog::error("Corrupt streamed prices file {} for symbol {}: {}. Exiting.", prices_file.string(),
                              symbol, e.what());
                throw;
            }
        }
        else
        {
            spdlog::info("No streamed prices file for {}, starting fresh", symbol);
            streamed_prices_[symbol] = {};
        }
    }
}

void PF_StreamerApp::LoadStreamedSummaryFromFile()
{
    fs::path summary_file = output_chart_directory_ / "streamed_summary.json";

    if (fs::exists(summary_file))
    {
        try
        {
            Json::Value summary_data;
            std::ifstream in(summary_file);
            in >> summary_data;
            in.close();

            for (const auto &symbol : symbol_list_)
            {
                std::string symbol_key = symbol;
                if (summary_data.isMember(symbol_key))
                {
                    const auto &symbol_data = summary_data[symbol_key];
                    StreamedSummary summary;
                    summary.opening_price_ = symbol_data["opening_price"].asDouble();
                    summary.latest_price_ = symbol_data["latest_price"].asDouble();
                    summary.curent_signal_type_ = symbol_data["curent_signal_type"].asInt();
                    streamed_summary_[symbol] = summary;
                }
            }
            spdlog::info("Loaded streamed summary from {}", summary_file.string());
        }
        catch (const Json::Exception &e)
        {
            spdlog::error("Corrupt summary file {}: {}. Exiting.", summary_file.string(), e.what());
            throw;
        }
    }
    else
    {
        spdlog::info("No summary file found, starting fresh");
        for (const auto &symbol : symbol_list_)
        {
            streamed_summary_[symbol] = {};
        }
    }
}

void PF_StreamerApp::SaveStreamedPricesToFiles()
{
    for (const auto &[symbol, prices] : streamed_prices_)
    {
        if (prices.timestamp_seconds_.empty())
        {
            continue;
        }

        fs::path prices_file = output_chart_directory_ / (symbol + "_streamed_prices.json");
        std::ofstream out(prices_file);

        Json::Value root;
        root["timestamp_seconds"] = Json::Value(Json::arrayValue);
        root["price"] = Json::Value(Json::arrayValue);
        root["signal_type"] = Json::Value(Json::arrayValue);

        for (auto timestamp : prices.timestamp_seconds_)
        {
            root["timestamp_seconds"].append(timestamp);
        }
        for (auto price : prices.price_)
        {
            root["price"].append(price);
        }
        for (auto signal : prices.signal_type_)
        {
            root["signal_type"].append(signal);
        }

        out << root << std::endl;
        out.close();
    }
    spdlog::info("Saved streamed prices to {}", output_chart_directory_.string());
}

void PF_StreamerApp::SaveStreamedSummaryToFile()
{
    fs::path summary_file = output_chart_directory_ / "streamed_summary.json";
    std::ofstream out(summary_file);

    Json::Value root;
    for (const auto &[symbol, summary] : streamed_summary_)
    {
        std::string symbol_key = symbol;
        root[symbol_key]["opening_price"] = summary.opening_price_;
        root[symbol_key]["latest_price"] = summary.latest_price_;
        root[symbol_key]["curent_signal_type"] = summary.curent_signal_type_;
    }

    out << root << std::endl;
    out.close();
    spdlog::info("Saved streamed summary to {}", summary_file.string());
}
