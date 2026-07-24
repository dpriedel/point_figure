#include "scanner/PF_ScannerApp.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <ranges>
#include <sstream>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/assert.hpp>

#include "PF_Chart.h"
#include "PointAndFigureDB.h"
#include "utilities.h"

// =====================================================================================
//        Class:  PF_ScannerApp
//  Description:  Daily scan - updates charts and computes trend statistics
// =====================================================================================

PF_ScannerApp::PF_ScannerApp(int argc, char *argv[]) : PF_AppBase{argc, argv}
{
    app_.description("Point & Figure daily scanner: updates charts and reports trend statistics.");
    SetupProgramOptions();
}

PF_ScannerApp::PF_ScannerApp(const std::vector<std::string> &tokens) : PF_AppBase{tokens}
{
    app_.description("Point & Figure daily scanner: updates charts and reports trend statistics.");
    SetupProgramOptions();
}

bool PF_ScannerApp::Startup()
{
    constexpr const char *time_fmt = "\n\n*** Begin run {:%a, %b %d, %Y at %I:%M:%S %p %Z}  ***\n";
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

std::tuple<int, int, int> PF_ScannerApp::Run()
{
    auto results = Run_DailyScan();
    return results;
}

void PF_ScannerApp::Shutdown()
{
    // nothing to clean up for scanner
}

void PF_ScannerApp::SetupProgramOptions()
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

    app_.add_option("--db-host", db_params_.host_name_, "Database host name.")->default_val("localhost");

    app_.add_option("--db-port", db_params_.port_number_, "Database port number.")->default_val(5432);

    app_.add_option("--db-user", db_params_.user_name_, "Database user name.");

    app_.add_option("--db-name", db_params_.db_name_, "Database name.");

    app_.add_option("--db-mode", db_params_.PF_db_mode_, "Database mode: 'test' or 'live'.")
        ->default_val("test")
        ->check(CLI::IsMember({"test", "live"}));

    app_.add_option("--stock-db-data-source", db_params_.stock_db_data_source_,
                    "Stock data source in DB (e.g., 'new_stock_data.current_data').");

    // Logging

    app_.add_option("--log-path", log_file_path_name_, "Path to log file.")->default_val("");

    app_.add_option("--logging-level", logging_level_, "Logging level: 'none', 'error', 'information', 'debug'.")
        ->default_val("information")
        ->check(CLI::IsMember({"none", "error", "information", "debug"}));

    // Scanner-specific options

    app_.add_option("--exchange-list", exchange_list_, "Symbols from specified exchange(s).")
        ->delimiter(',')
        ->transform([](std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return (c != '/' ? ::toupper(c) : '_'); });
            return s;
        });

    app_.add_option("--min-dollar-volume", min_dollar_volume_,
                    "Minimum dollar volume to filter stocks. Default is $100000")
        ->default_val("100000");

    app_.add_option("--begin-date", begin_date_, "Start date for extracting data from database.")
        ->required()
        ->check(check_date);

    app_.add_option("--end-date", end_date_, "Stop date for extracting data from database.")->check(check_date);

    app_.add_option("--price-fld-name", price_fld_name_, "Data field to use for price value.")
        ->default_val("split_adj_close");
}

bool PF_ScannerApp::CheckArgs()
{
    BOOST_ASSERT_MSG(!db_params_.host_name_.empty(), "\nMust provide 'db-host' for daily scan.");
    BOOST_ASSERT_MSG(db_params_.port_number_ != -1, "\nMust provide 'db-port' for daily scan.");
    BOOST_ASSERT_MSG(!db_params_.user_name_.empty(), "\nMust provide 'db-user' for daily scan.");
    BOOST_ASSERT_MSG(!db_params_.db_name_.empty(), "\nMust provide 'db-name' for daily scan.");
    BOOST_ASSERT_MSG(db_params_.PF_db_mode_ == "test" || db_params_.PF_db_mode_ == "live",
                     "\n'db-mode' must be 'test' or 'live'.");
    BOOST_ASSERT_MSG(!db_params_.stock_db_data_source_.empty(),
                     "\n'stock-db-data-source' must be specified for daily scan.");
    BOOST_ASSERT_MSG(!begin_date_.empty(), "\nMust specify 'begin-date' for daily scan.");

    if (!exchange_list_.empty())
    {
        rng::sort(exchange_list_);
        const auto [first, last] = rng::unique(exchange_list_);
        exchange_list_.erase(first, last);

        PF_DB pf_db{db_params_};
        auto exchanges = pf_db.ListExchanges();
        spdlog::debug("available exchanges: {}\n", exchanges);

        rng::for_each(exchange_list_, [&exchanges](const auto &xchng) {
            BOOST_ASSERT_MSG(std::find_if(exchanges.begin(), exchanges.end(),
                                          [&xchng](const auto &e) { return e == xchng; }) != exchanges.end(),
                             std::format("Exchange '{}' not found in database.", xchng).c_str());
        });
    }

    spdlog::debug("begin-date: {}\n", begin_date_);
    if (!end_date_.empty())
    {
        spdlog::debug("end-date: {}\n", end_date_);
    }
    else
    {
        auto now = std::chrono::system_clock::now();
        auto today = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(now)};
        end_date_ = std::format("{:%F}", today);
        spdlog::debug("end-date defaults to today: {}\n", end_date_);
    }

    return true;
}

std::tuple<int, int, int> PF_ScannerApp::Run_DailyScan()
{
    int32_t total_symbols_processed = 0;
    int32_t total_charts_processed = 0;
    int32_t total_charts_updated = 0;

    PF_DB pf_db{db_params_};
    const auto *dt_format = "%F";

    if (exchange_list_.empty())
    {
        exchange_list_ = pf_db.ListExchanges();

        auto dont_use = [](const auto &xchng) { return xchng == "NMFQS" || xchng == "INDX" || xchng == "US"; };
        const auto [first, last] = rng::remove_if(exchange_list_, dont_use);
        exchange_list_.erase(first, last);
    }
    spdlog::debug("exchanges for scan: {}\n", exchange_list_);

    auto data_for_symbol = vws::chunk_by([](const auto &a, const auto &b) { return a.symbol_ == b.symbol_; });

    for (const auto &xchng : exchange_list_)
    {
        spdlog::info(std::format("Scanning charts for symbols on xchng: {} with adjusted dollar volume >= "
                                 "{}.",
                                 xchng, min_dollar_volume_));

        int32_t exchange_symbols_processed = 0;
        int32_t exchange_charts_processed = 0;
        int32_t exchange_charts_updated = 0;

        auto db_data = pf_db.GetPriceDataForSymbolsOnExchange(xchng, begin_date_, end_date_, price_fld_name_, dt_format,
                                                              min_dollar_volume_);

        for (const auto &symbol_rng : db_data | data_for_symbol)
        {
            const auto &symbol = symbol_rng[0].symbol_;
            exchange_symbols_processed += 1;

            auto charts_for_symbol = pf_db.RetrieveAllEODChartsForSymbol(symbol);

            for (auto &chart : charts_for_symbol)
            {
                exchange_charts_processed += 1;
                bool chart_needs_update = false;
                try
                {
                    rng::for_each(symbol_rng, [&chart, &chart_needs_update](const auto &row) {
                        auto status = chart.AddValue(row.close_, row.date_);
                        chart_needs_update |= status == PF_Column::Status::e_Accepted ? 1 : 0;
                    });
                    if (chart_needs_update)
                    {
                        chart.UpdateChartInChartsDB(pf_db, "eod", X_AxisFormat::e_show_date, false);
                        exchange_charts_updated += 1;
                    }
                }
                catch (const std::exception &e)
                {
                    spdlog::error(std::format("Unable to update data for chart: {} from DB because: "
                                              "{}.",
                                              chart.MakeChartFileName("eod", ""), e.what()));
                }
            }
        }

        total_symbols_processed += exchange_symbols_processed;
        total_charts_processed += exchange_charts_processed;
        total_charts_updated += exchange_charts_updated;
        spdlog::info(std::format("Exchange: {}. Symbols: {}. Charts scanned: {}. Charts updated: "
                                 "{}.",
                                 xchng, exchange_symbols_processed, exchange_charts_processed,
                                 exchange_charts_updated));

        pf_db.UpdateLastCheckedDateInChartsDB(xchng, end_date_);
    }

    const auto [ups1, downs1] = CountChartReversalsUpAndDown();
    const auto [ups2, downs2] = CountChartTrendsContinueUpAndDown();
    const auto [ups3, downs3] = CountChartTrendsUnanimousUpAndDown();

    spdlog::info(std::format("Total symbols: {}. Total charts scanned: {}. Total charts updated: "
                             "{}.",
                             total_symbols_processed, total_charts_processed, total_charts_updated));

    spdlog::info(std::format("Reversals. Up: {}. Down: {}. Net reversals {}: {}.", ups1, downs1,
                             (ups1 - downs1 > 0 ? "UP" : "DOWN"), std::abs(ups1 - downs1)));
    spdlog::info(std::format("Trends continued. Up: {}. Down: {}. Net continues {}: {}.", ups2, downs2,
                             (ups2 - downs2 > 0 ? "UP" : "DOWN"), std::abs(ups2 - downs2)));
    spdlog::info(std::format("Unanimous trends. Up: {}. Down: {}. Net unanimous {}: {}.", ups3, downs3,
                             (ups3 - downs3 > 0 ? "UP" : "DOWN"), std::abs(ups3 - downs3)));

    return {total_symbols_processed, total_charts_processed, total_charts_updated};
}

std::pair<int, int> PF_ScannerApp::CountChartReversalsUpAndDown() const
{
    const auto query_up =
        std::format("SELECT count(*) FROM {}_point_and_figure.find_trend_reversals('e_up')", db_params_.PF_db_mode_);
    const auto query_down =
        std::format("SELECT count(*) FROM {}_point_and_figure.find_trend_reversals('e_down')", db_params_.PF_db_mode_);

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::nontransaction trxn{c};

    auto charts_up = trxn.query_value<int>(query_up);
    auto charts_down = trxn.query_value<int>(query_down);
    return std::make_pair(charts_up, charts_down);
}

std::pair<int, int> PF_ScannerApp::CountChartTrendsContinueUpAndDown() const
{
    const auto query_up =
        std::format("SELECT count(*) FROM {}_point_and_figure.find_trend_continues('e_up')", db_params_.PF_db_mode_);
    const auto query_down =
        std::format("SELECT count(*) FROM {}_point_and_figure.find_trend_continues('e_down')", db_params_.PF_db_mode_);

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::nontransaction trxn{c};

    auto charts_up = trxn.query_value<int>(query_up);
    auto charts_down = trxn.query_value<int>(query_down);
    return std::make_pair(charts_up, charts_down);
}

std::pair<int, int> PF_ScannerApp::CountChartTrendsUnanimousUpAndDown() const
{
    const auto query_up =
        std::format("SELECT count(*) FROM {}_point_and_figure.find_unanimous_trends('e_up')", db_params_.PF_db_mode_);
    const auto query_down =
        std::format("SELECT count(*) FROM {}_point_and_figure.find_unanimous_trends('e_down')", db_params_.PF_db_mode_);

    pqxx::connection c{std::format("dbname={} user={}", db_params_.db_name_, db_params_.user_name_)};
    pqxx::nontransaction trxn{c};

    auto charts_up = trxn.query_value<int>(query_up);
    auto charts_down = trxn.query_value<int>(query_down);
    return std::make_pair(charts_up, charts_down);
}
