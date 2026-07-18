#include "updater/PF_UpdaterApp.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sstream>
#include <string_view>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/assert.hpp>

#include "ConstructChartGraphic.h"
#include "PF_Chart.h"
#include "PF_Column.h"
#include "PointAndFigureDB.h"
#include "utilities.h"

using decimal::Decimal;
using namespace std::string_literals;
using namespace std::string_view_literals;

// =====================================================================================
//        Class:  PF_UpdaterApp
//  Description:  Update mode — incrementally updates charts with new price data
// =====================================================================================

PF_UpdaterApp::PF_UpdaterApp(int argc, char *argv[])
    : PF_AppBase{argc, argv}
{
    app_.description("Point & Figure updater: incrementally updates charts with new price data.");
    SetupProgramOptions();
}

PF_UpdaterApp::PF_UpdaterApp(const std::vector<std::string> &tokens)
    : PF_AppBase{tokens}
{
    app_.description("Point & Figure updater: incrementally updates charts with new price data.");
    SetupProgramOptions();
}

bool PF_UpdaterApp::Startup()
{
    spdlog::info(std::format("\n\n*** Starting run {} ***\n",
                              std::chrono::current_zone()->to_local(std::chrono::system_clock::now())));
    bool result{true};
    try
    {
        ParseProgramOptions(tokens_);
        ConfigureLogging();
        result = CheckArgs();
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

std::tuple<int, int, int> PF_UpdaterApp::Run()
{
    number_of_days_history_for_ATR_ = 20;

    if (new_data_source_ == Source::e_file)
    {
        Run_Update();
    }
    else if (new_data_source_ == Source::e_DB)
    {
        Run_UpdateFromDB();
    }
    return {};
}

void PF_UpdaterApp::Shutdown()
{
    if (destination_ == Destination::e_file)
    {
        ShutdownAndStoreOutputInFiles();
    }
    else
    {
        ShutdownAndStoreOutputInDB();
    }

    spdlog::info(std::format("\n\n*** End run {}  ***\n",
                              std::chrono::current_zone()->to_local(std::chrono::system_clock::now())));

    std::this_thread::sleep_for(std::chrono::seconds(2));
}

void PF_UpdaterApp::SetupProgramOptions()
{
    app_.preparse_callback([](size_t argCount) {
        if (argCount == 0)
        {
            throw CLI::CallForHelp();
        }
    });

    app_.failure_message(CLI::FailureMessage::help);

    auto check_date = [](const std::string &str) -> std::string {
        std::istringstream in{str};
        std::chrono::sys_days tp;
        std::chrono::from_stream(in, "%F", tp);
        if (in.fail())
        {
            in.clear();
            in.rdbuf()->pubseekpos(0);
            std::chrono::from_stream(in, "%Y-%b-%d", tp);
        }
        if (in.fail() || in.bad())
        {
            return std::format("Error: Unable to parse supplied date: {}", str);
        }
        auto ymd = static_cast<std::chrono::year_month_day>(tp);
        if (!ymd.ok())
        {
            return std::format("Invalid supplied date: {}", str);
        }
        return {};
    };

    // DB connection parameters

    app_.add_option("--db-host", db_params_.host_name_, "Database host name.")
        ->default_val("localhost");

    app_.add_option("--db-port", db_params_.port_number_, "Database port number.")
        ->default_val(5432);

    app_.add_option("--db-user", db_params_.user_name_, "Database user name.");

    app_.add_option("--db-name", db_params_.db_name_, "Database name.");

    app_.add_option("--db-mode", db_params_.PF_db_mode_, "Database mode: 'test' or 'live'.")
        ->default_val("test")
        ->check(CLI::IsMember({"test", "live"}));

    app_.add_option("--stock-db-data-source", db_params_.stock_db_data_source_,
                    "Stock data source in DB (e.g., 'new_stock_data.current_data').");

    // Logging

    app_.add_option("--log-path", log_file_path_name_, "Path to log file.")
        ->default_val("");

    app_.add_option("--logging-level", logging_level_, "Logging level: 'none', 'error', 'information', 'debug'.")
        ->default_val("information")
        ->check(CLI::IsMember({"none", "error", "information", "debug"}));

    // Symbol options

    auto symbols_source_group =
        app_.add_option_group("Symbols source", "Specify ticker symbols to process.");
    symbols_source_group
        ->add_option("-s,--symbol", symbol_list_,
                     "Name of symbol we are processing data for. Repeat for multiple symbols.")
        ->delimiter(',')
        ->transform([](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(), ::toupper);
            return s;
        });

    symbols_source_group->add_option("--symbol-list", symbol_list_i_,
                                     "Comma-separated list of symbols.");

    // Data source options

    app_.add_option("--new-data-source", new_data_source_i_, "Source for new price data: 'file' or 'database'.")
        ->required()
        ->check(CLI::IsMember({"file", "database"}));

    app_.add_option("--new-data-dir", new_data_input_directory_, "Directory containing new price data files.")
        ->check(CLI::ExistingDirectory);

    app_.add_option("--source-format", source_format_i_, "Format of source data files: 'csv' or 'json'.")
        ->default_val("csv")
        ->check(CLI::IsMember({"csv", "json"}));

    // Chart data source — where to find existing charts to update

    app_.add_option("--chart-data-source", chart_data_source_i_,
                    "Source for existing chart data: 'file' or 'database'.")
        ->default_val("file")
        ->check(CLI::IsMember({"file", "database"}));

    // Mode is fixed to 'update' for this app — no CLI option needed

    // Interval and chart parameters

    app_.add_option("--interval", interval_i_, "Data interval: 'eod', 'live', 'sec1', 'sec5', 'min1', 'min5'.")
        ->default_val("eod")
        ->check(CLI::IsMember({"eod", "live", "sec1", "sec5", "min1", "min5"}));

    app_.add_option("--scale", scale_i_list_, "Chart scale: 'linear' or 'percent'. Repeat for multiple scales.")
        ->default_val("linear")
        ->check(CLI::IsMember({"linear", "percent"}));

    app_.add_option("--price-fld-name", price_fld_name_, "Data field to use for price value.")
        ->default_val("Close");

    // Destination options

    app_.add_option("--destination", destination_i_, "Where to store chart data: 'file' or 'database'.")
        ->required()
        ->check(CLI::IsMember({"file", "database"}));

    app_.add_option("--output-chart-dir", output_chart_directory_, "Directory for output chart JSON files.");

    app_.add_option("--chart-data-dir", input_chart_directory_, "Directory with existing chart data.");

    app_.add_option("--output-graph-dir", output_graphs_directory_, "Directory for output graph files.");

    // Box size and reversal

    app_.add_option("--boxsize", box_size_i_list_, "Box size value. Repeat for multiple values.")
        ->required();

    app_.add_option("--reversal", reversal_boxes_list_, "Reversal boxes count. Repeat for multiple values.")
        ->required();

    // Graphics and ATR options

    app_.add_option("--graphics-format", graphics_format_i_, "Graphics format: 'svg' or 'csv'.")
        ->default_val("svg")
        ->check(CLI::IsMember({"svg", "csv"}));

    app_.add_flag("--use-ATR", use_ATR_, "Use ATR-based box size calculation.");

    app_.add_option("--quote-host", quote_host_name_, "Quote data host name.")
        ->default_val("eodhd.com");

    app_.add_option("--quote-port", quote_host_port_, "Port for quotes.")
        ->default_val("443");

    app_.add_option("--quote-data-source", quote_data_source_i_, "Quote data source: 'Eodhd' or 'Tiingo'.")
        ->default_val("Eodhd")
        ->check(CLI::IsMember({"Eodhd", "Tiingo"}));

    app_.add_option("--quote-api-key", quote_host_api_key_, "API key file name in config directory.");

    app_.add_option("--config-dir", PF_CollectDataConfigDir_, "Path to config directory.");

    app_.add_option("--max-graphic-cols", max_columns_for_graph_, "Maximum columns for graphic output. -1 for unlimited.")
        ->default_val(-1);

    // Date options (for DB source)

    app_.add_option("--begin-date", begin_date_, "Start date for extracting data from database.")
        ->check(check_date);

    app_.add_option("--end-date", end_date_, "Stop date for extracting data from database.")
        ->check(check_date);

    // Trend lines option

    app_.add_option("--show-trend-lines", trend_lines_, "Show trend lines: 'no', 'data', or 'angle'.")
        ->default_val("no")
        ->check(CLI::IsMember({"no", "data", "angle"}));
}

bool PF_UpdaterApp::CheckArgs()
{
    // Resolve config directory from CLI option or environment variable
    if (PF_CollectDataConfigDir_.empty())
    {
        const std::string config_var{"PF_COLLECT_DATA_CONFIG_DIR"};
        const char *env_var = std::getenv(config_var.c_str());
        PF_CollectDataConfigDir_ = env_var == nullptr ? "" : env_var;
    }

    boxsize_source_ = (use_ATR_ ? BoxsizeSource::e_from_ATR : BoxsizeSource::e_from_args);

    new_data_source_ = new_data_source_i_ == "file"       ? Source::e_file
                        : new_data_source_i_ == "database" ? Source::e_DB
                                                           : Source::e_unknown;

    chart_data_source_ = chart_data_source_i_ == "file"     ? Source::e_file
                           : chart_data_source_i_ == "database" ? Source::e_DB
                                                                : Source::e_unknown;

    destination_ = destination_i_ == "file" ? Destination::e_file : Destination::e_DB;

    if (!symbol_list_i_.empty())
    {
        rng::for_each(split_string<std::string>(symbol_list_i_, ","),
                      [this](const auto sym) { symbol_list_.push_back(sym); });
        rng::sort(symbol_list_);
        const auto [first, last] = rng::unique(symbol_list_);
        symbol_list_.erase(first, last);
        rng::for_each(symbol_list_, [](auto &symbol) { rng::for_each(symbol, [](char &c) { c = std::toupper(c); }); });
    }

    BOOST_ASSERT_MSG(!symbol_list_.empty() || !symbol_list_i_.empty(),
                     "\nMust provide either 1 or more '-s' values or 'symbol-list'.");

    if (new_data_source_ == Source::e_file)
    {
        BOOST_ASSERT_MSG(!new_data_input_directory_.empty(),
                         "\nMust specify 'new-data-dir' when data source is 'file'.");

        source_format_ = source_format_i_ == "csv" ? SourceFormat::e_csv : SourceFormat::e_json;

        // if we are adding to existing data then we need to know where to find that data

        if (chart_data_source_ == Source::e_file)
        {
            if (output_chart_directory_.empty())
            {
                output_chart_directory_ = input_chart_directory_;
            }
        }
    }

    graphics_format_ = graphics_format_i_ == "svg" ? GraphicsFormat::e_svg : GraphicsFormat::e_csv;

    if (destination_ == Destination::e_file)
    {
        BOOST_ASSERT_MSG(!output_chart_directory_.empty(),
                         "\nMust specify 'output-chart-dir' when data destination is 'file'.");
        if (!fs::exists(output_chart_directory_))
        {
            fs::create_directories(output_chart_directory_);
        }

        if (output_graphs_directory_.empty())
        {
            output_graphs_directory_ = output_chart_directory_;
        }
    }

    if (destination_ == Destination::e_file || graphics_format_ == GraphicsFormat::e_svg)
    {
        BOOST_ASSERT_MSG(!output_graphs_directory_.empty(), "\nMust specify 'output-graph-dir'.");
        if (!fs::exists(output_graphs_directory_))
        {
            fs::create_directories(output_graphs_directory_);
        }
    }

    if (new_data_source_ == Source::e_DB || destination_ == Destination::e_DB)
    {
        BOOST_ASSERT_MSG(!db_params_.host_name_.empty(),
                         "\nMust provide 'db-host' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(db_params_.port_number_ != -1,
                         "\nMust provide 'db-port' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(!db_params_.user_name_.empty(),
                         "\nMust provide 'db-user' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(!db_params_.db_name_.empty(),
                         "\nMust provide 'db-name' when data source or destination is 'database'.");
        BOOST_ASSERT_MSG(db_params_.PF_db_mode_ == "test" || db_params_.PF_db_mode_ == "live",
                         "\n'db-mode' must be 'test' or 'live'.");
        if (new_data_source_ == Source::e_DB)
        {
            BOOST_ASSERT_MSG(!db_params_.stock_db_data_source_.empty(),
                             "\n'stock-db-data-source' must be specified when load source is 'database'.");
        }
    }

    if ((new_data_source_ == Source::e_file && use_ATR_) || new_data_source_ == Source::e_streaming)
    {
        quote_data_source_ = quote_data_source_i_ == "Tiingo" ? QuoteDataSource::e_Tiingo : QuoteDataSource::e_Eodhd;

        BOOST_ASSERT_MSG(!quote_host_api_key_.empty(), "Must specify a quote source API key file for non-DB ATR.");
        BOOST_ASSERT_MSG(fs::exists(PF_CollectDataConfigDir_ / quote_host_api_key_),
                         std::format("\nCan't find ATR quotes source api key file: {}", quote_host_api_key_).c_str());

        std::ifstream quotes_key_file(PF_CollectDataConfigDir_ / quote_host_api_key_);
        quotes_key_file >> quotes_api_key_;
    }

    if (new_data_source_ == Source::e_DB)
    {
        BOOST_ASSERT_MSG(!begin_date_.empty(), "\nMust specify 'begin-date' when data source is 'database'.");
    }

    BOOST_ASSERT_MSG(max_columns_for_graph_ >= -1, "\nmax-graphic-cols must be >= -1.");

    const std::map<std::string, Interval> possible_intervals = {{"eod", Interval::e_eod},   {"live", Interval::e_live},
                                                                 {"sec1", Interval::e_sec1}, {"sec5", Interval::e_sec5},
                                                                 {"min1", Interval::e_min1}, {"min5", Interval::e_min5}};
    auto which_interval = possible_intervals.find(interval_i_);
    BOOST_ASSERT_MSG(which_interval != possible_intervals.end(),
                     std::format("\nInterval must be: 'eod', 'live', 'sec1', 'sec5', 'min1', 'min5': {}", interval_i_)
                         .c_str());
    interval_ = which_interval->second;

    if (scale_i_list_.empty())
    {
        scale_i_list_.emplace_back("linear");
    }

    rng::for_each(scale_i_list_, [](const auto &scale) {
        BOOST_ASSERT_MSG(scale == "linear" || scale == "percent",
                         std::format("\nChart scale must be: 'linear' or 'percent': {}", scale).c_str());
    });
    rng::for_each(scale_i_list_, [this](const auto &scale_i) {
        this->scale_list_.emplace_back(scale_i == "linear" ? BoxScale::e_Linear : BoxScale::e_Percent);
    });

    // Convert box_size strings to Decimal values
    for (const auto &b : box_size_i_list_)
    {
        box_size_list_.emplace_back(Decimal{b});
    }

    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);
    rng::for_each(params, [](const auto &x) {
        std::cout << std::format("{}\t{}\t{}\t{}\n", std::get<0>(x), std::get<1>(x).format("f"), std::get<2>(x),
                                  std::get<3>(x));
    });
    std::cout << std::endl;

    return true;
}

void PF_UpdaterApp::Run_Update()
{
    auto params = vws::cartesian_product(symbol_list_, box_size_list_, reversal_boxes_list_, scale_list_);

    for (const auto &val : params)
    {
        const auto &symbol = std::get<PF_Chart::e_symbol>(val);
        PF_Chart new_chart;
        fs::path existing_data_file_name;
        try
        {
            existing_data_file_name = input_chart_directory_ / MakeChartNameFromParams(val, interval_i_, "json");
            if (fs::exists(existing_data_file_name))
            {
                new_chart = LoadAndParsePriceDataJSON(existing_data_file_name);
                if (max_columns_for_graph_ != 0)
                {
                    new_chart.SetMaxGraphicColumns(max_columns_for_graph_);
                }
            }
            else
            {
                // no existing data to update, so make a new chart

                auto atr = use_ATR_ ? ComputeATRForChart(symbol) : 0;
                if (use_ATR_)
                {
                    new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
                else
                {
                    new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                }
            }
            fs::path update_file_name =
                new_data_input_directory_ / (symbol + '.' + (source_format_ == SourceFormat::e_csv ? "csv" : "json"));
            BOOST_ASSERT_MSG(
                fs::exists(update_file_name),
                std::format("\nCan't find data file for symbol: {} for update.", update_file_name).c_str());
            BOOST_ASSERT_MSG(source_format_ == SourceFormat::e_csv,
                             "\nJSON files are not yet supported for updating symbol data.");
            AddPriceDataToExistingChartCSV(new_chart, update_file_name);
            charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
        }
        catch (const Json::Exception &e)
        {
            spdlog::error(std::format("Unable to process JSON data from file: {} because: {}.", existing_data_file_name,
                                      e.what()));
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Unable to update data for chart: {} from file because: {}.",
                                      new_chart.MakeChartFileName(interval_i_, ""), e.what()));
        }
    }
}

void PF_UpdaterApp::Run_UpdateFromDB()
{
    PF_DB pf_db{db_params_};

    const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";
    auto db_data = pf_db.GetPriceDataForSymbolsInList(symbol_list_, begin_date_, end_date_, price_fld_name_, dt_format);

    auto data_for_symbol = vws::chunk_by([](const auto &a, const auto &b) { return a.symbol_ == b.symbol_; });

    for (const auto &symbol_rng : db_data | data_for_symbol)
    {
        const auto &symbol = symbol_rng[0].symbol_;
        std::vector<std::string> the_symbol{symbol};

        auto params = vws::cartesian_product(the_symbol, box_size_list_, reversal_boxes_list_, scale_list_);

        for (const auto &val : params)
        {
            PF_Chart new_chart;
            try
            {
                if (chart_data_source_ == Source::e_file)
                {
                    fs::path existing_data_file_name =
                        input_chart_directory_ / MakeChartNameFromParams(val, interval_i_, "json");
                    if (fs::exists(existing_data_file_name))
                    {
                        new_chart = LoadAndParsePriceDataJSON(existing_data_file_name);
                        if (max_columns_for_graph_ != 0)
                        {
                            new_chart.SetMaxGraphicColumns(max_columns_for_graph_);
                        }
                    }
                }
                else
                {
                    new_chart = PF_Chart::LoadChartFromChartsDB(PF_DB{db_params_}, val, interval_i_);
                }
                if (new_chart.empty())
                {
                    auto atr = use_ATR_ ? ComputeATRForChartFromDB(symbol) : 0;
                    if (use_ATR_)
                    {
                        new_chart = PF_Chart{atr, val, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                    }
                    else
                    {
                        new_chart = PF_Chart{val, atr, max_columns_for_graph_ < 1 ? -1 : max_columns_for_graph_};
                    }
                }

                rng::for_each(symbol_rng, [&new_chart](const auto &row) { new_chart.AddValue(row.close_, row.date_); });

                charts_.emplace_back(std::make_pair(symbol, std::move(new_chart)));
            }
            catch (const std::exception &e)
            {
                spdlog::error(std::format("Unable to update data for chart: {} from DB because: {}.",
                                          new_chart.MakeChartFileName(interval_i_, ""), e.what()));
            }
        }
    }
}

void PF_UpdaterApp::AddPriceDataToExistingChartCSV(PF_Chart &new_chart, const fs::path &update_file_name) const
{
    const std::string file_content = LoadDataFileForUse(update_file_name);

    const auto symbol_data_records = split_string<std::string_view>(file_content, "\n");
    const auto header_record = symbol_data_records.front();

    auto date_column = FindColumnIndex(header_record, "date", ",");
    BOOST_ASSERT_MSG(date_column.has_value(),
                     std::format("\nCan't find 'date' field in header record: {}.", header_record).c_str());

    auto close_column = FindColumnIndex(header_record, price_fld_name_, ",");
    BOOST_ASSERT_MSG(
        close_column.has_value(),
        std::format("\nCan't find price field: {} in header record: {}.", price_fld_name_, header_record).c_str());

    rng::for_each(
        symbol_data_records | vws::drop(1),
        [this, &new_chart, close_col = close_column.value(), date_col = date_column.value()](const auto record) {
            const auto fields = split_string<std::string_view>(record, ",");
            const auto *dt_format = interval_ == Interval::e_eod ? "%F" : "%F %T%z";
            new_chart.AddValue(sv2dec(fields[close_col]), StringToUTCTimePoint(dt_format, fields[date_col]));
        });
}

PF_Chart PF_UpdaterApp::LoadAndParsePriceDataJSON(const fs::path &symbol_file_name)
{
    PF_Chart new_chart;
    PF_Chart::LoadChartFromJSONPF_ChartFile(new_chart, symbol_file_name);
    return new_chart;
}

std::optional<int> PF_UpdaterApp::FindColumnIndex(std::string_view header, std::string_view column_name,
                                                   std::string_view delim)
{
    auto fields = rng_split_string<std::string_view>(header, delim) | rng::to<std::vector>();
    auto do_compare([&column_name](const auto &field_name) {
        if (column_name.size() != field_name.size())
        {
            return false;
        }
        return rng::equal(column_name, field_name,
                          [](unsigned char a, unsigned char b) { return tolower(a) == tolower(b); });
    });

    if (auto found_it = rng::find_if(fields, do_compare); found_it != rng::end(fields))
    {
        return rng::distance(fields.begin(), found_it);
    }
    return {};
}

Decimal PF_UpdaterApp::ComputeATRForChart(const std::string &symbol) const
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

    auto atr = ComputeATR(symbol, history, number_of_days_history_for_ATR_);

    return atr;
}

Decimal PF_UpdaterApp::ComputeATRForChartFromDB(const std::string &symbol) const
{
    PF_DB the_db{db_params_};

    Decimal atr{};
    try
    {
        auto price_data =
            the_db.RetrieveMostRecentStockDataRecordsFromDB(symbol, end_date_, number_of_days_history_for_ATR_ + 1);
        atr = ComputeATR(symbol, price_data, number_of_days_history_for_ATR_);
    }
    catch (const std::exception &e)
    {
        spdlog::error(std::format("Unable to compute ATR from DB for: '{}' because: {}.\n", symbol, e.what()));
    }

    return atr;
}

void PF_UpdaterApp::ShutdownAndStoreOutputInFiles()
{
    for (const auto &[symbol, chart] : charts_)
    {
        if (chart.empty())
            continue;
        try
        {
            fs::path output_file_name =
                output_chart_directory_ /
                chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "json");
            chart.ConvertChartToJsonAndWriteToFile(output_file_name);

            if (graphics_format_ == GraphicsFormat::e_svg)
            {
                fs::path graph_file_path =
                    output_graphs_directory_ /
                    (chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "svg"));
                ConstructCDPFChartGraphicAndWriteToFile(
                    chart, graph_file_path, StreamedPrices{}, trend_lines_, interval_ != Interval::e_eod ? X_AxisFormat::e_show_time : X_AxisFormat::e_show_date);
            }
            else
            {
                fs::path graph_file_path =
                    output_graphs_directory_ /
                    (chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "csv"));
                chart.ConvertChartToTableAndWriteToFile(graph_file_path, interval_ != Interval::e_eod
                                                                              ? X_AxisFormat::e_show_time
                                                                              : X_AxisFormat::e_show_date);
            }
        }
        catch (const std::exception &e)
        {
            spdlog::error(
                std::format("Problem in shutdown: {} for chart: {}.\nTrying to "
                            "complete shutdown.",
                            e.what(),
                            chart.MakeChartFileName((new_data_source_ == Source::e_streaming ? "" : interval_i_), "")));
        }
    }
}

void PF_UpdaterApp::ShutdownAndStoreOutputInDB()
{
    int32_t chart_count = 0;
    PF_DB pf_db{db_params_};
    for (const auto &[symbol, chart] : charts_)
    {
        if (chart.empty())
            continue;
        try
        {
            if (graphics_format_ == GraphicsFormat::e_svg)
            {
                fs::path graph_file_path = output_graphs_directory_ / (chart.MakeChartFileName(interval_i_, "svg"));
                ConstructCDPFChartGraphicAndWriteToFile(
                    chart, graph_file_path, StreamedPrices{}, trend_lines_, interval_ != Interval::e_eod ? X_AxisFormat::e_show_time : X_AxisFormat::e_show_date);
            }
            chart.StoreChartInChartsDB(pf_db, interval_i_,
                                       interval_ != Interval::e_eod ? X_AxisFormat::e_show_time
                                                                    : X_AxisFormat::e_show_date,
                                       graphics_format_ == GraphicsFormat::e_csv);
            ++chart_count;
        }
        catch (const std::exception &e)
        {
            spdlog::error(std::format("Problem storing data in DB in shutdown: {} for chart: "
                                      "{}.\nTrying to complete shutdown.",
                                      e.what(), chart.MakeChartFileName(interval_i_, "")));
        }
    }
    spdlog::info(std::format("Stored {} charts in DB.", chart_count));
}
