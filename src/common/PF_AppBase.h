#ifndef PF_APPBASE_INC
#define PF_APPBASE_INC

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace std::chrono_literals;

#include <CLI/CLI.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "PointAndFigureDB.h"

class PF_AppBase
{
public:
    PF_AppBase(int argc, char *argv[]);
    explicit PF_AppBase(const std::vector<std::string> &tokens);
    virtual ~PF_AppBase();

    static bool SignalReceived() { return had_signal_; }
    static void SetSignal() { had_signal_ = true; }
    static void WaitForTimer(const std::chrono::zoned_seconds &stop_at);

protected:
    void ConfigureLogging();
    void ParseProgramOptions(const std::vector<std::string> &tokens);

    std::shared_ptr<spdlog::logger> original_logger_;
    CLI::App app_{"Point and Figure Charts for Linux."};
    int argc_ = 0;
    char **argv_ = nullptr;
    const std::vector<std::string> tokens_;
    std::string logging_level_{"information"};
    fs::path log_file_path_name_;
    fs::path PF_CollectDataConfigDir_;
    PF_DB::DB_Params db_params_{.port_number_ = -1};

private:
    static void HandleSignal(int signal);

protected:
    static bool had_signal_;
};

#endif
