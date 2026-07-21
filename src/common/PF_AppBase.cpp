#include <chrono>
#include <format>
#include <iostream>
#include <map>
#include <ranges>
#include <sstream>
#include <string_view>
#include <thread>

using namespace std::chrono_literals;

namespace rng = std::ranges;

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "PF_AppBase.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

bool PF_AppBase::had_signal_ = false;

PF_AppBase::PF_AppBase(int argc, char *argv[]) : argc_{argc}, argv_{argv}
{
    original_logger_ = spdlog::default_logger();
    auto tempLogger = spdlog::stdout_color_mt("temp_logger");
    spdlog::set_default_logger(tempLogger);
    spdlog::set_level(spdlog::level::info);
}

PF_AppBase::PF_AppBase(const std::vector<std::string> &tokens) : tokens_{tokens}
{
    original_logger_ = spdlog::default_logger();
    auto tempLogger = spdlog::stdout_color_mt("temp_logger");
    spdlog::set_default_logger(tempLogger);
    spdlog::set_level(spdlog::level::info);
}

PF_AppBase::~PF_AppBase()
{
    if (spdlog::get("PF_Collect_logger"))
    {
        spdlog::drop("PF_Collect_logger");
    }
    spdlog::drop("temp_logger");
    if (original_logger_)
    {
        spdlog::set_default_logger(original_logger_);
    }
}

void PF_AppBase::ConfigureLogging()
{
    if (!log_file_path_name_.empty())
    {
        fs::path log_dir = log_file_path_name_.parent_path();
        if (!fs::exists(log_dir))
        {
            fs::create_directories(log_dir);
        }

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file_path_name_, true);
        auto app_logger = std::make_shared<spdlog::logger>("PF_Collect_logger", file_sink);
        spdlog::set_default_logger(app_logger);
    }
    else
    {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto app_logger = std::make_shared<spdlog::logger>("PF_Collect_logger", console_sink);
        spdlog::set_default_logger(app_logger);
    }

    const std::map<std::string, spdlog::level::level_enum> levels{{"none", spdlog::level::off},
                                                                   {"error", spdlog::level::err},
                                                                   {"information", spdlog::level::info},
                                                                   {"debug", spdlog::level::debug}};

    auto which_level = levels.find(logging_level_);
    if (which_level != levels.end())
    {
        spdlog::set_level(which_level->second);
    }
    else
    {
        spdlog::set_level(spdlog::level::info);
    }
}

void PF_AppBase::ParseProgramOptions(const std::vector<std::string> &tokens)
{
    try
    {
        if (tokens.empty())
        {
            auto args = std::views::counted(argv_, argc_) |
                        std::views::transform([](char *arg) { return std::string_view(arg); });
            auto cmd_line_vw = rng::views::join_with(rng::views::drop(args, 1), " "sv);
            std::string cmd_line = rng::to<std::string>(cmd_line_vw);
            spdlog::info("cmd line: {}", cmd_line);
            app_.parse(argc_, argv_);
        }
        else
        {
            // Convert token-style args to argv-like format for CLI11
            std::vector<char*> argv_ptrs;
            std::vector<std::unique_ptr<std::vector<char>>> arg_buffers;
            for (const auto &token : tokens)
            {
                auto buf = std::make_unique<std::vector<char>>(token.size() + 1);
                std::copy(token.begin(), token.end(), buf->begin());
                buf->back() = '\0';
                argv_ptrs.push_back(buf->data());
                arg_buffers.push_back(std::move(buf));
            }
            int argc = static_cast<int>(argv_ptrs.size());
            spdlog::info("tokens: {}",
                         rng::views::join_with(rng::views::drop(tokens, 1), " "sv) | rng::to<std::string>());
            app_.parse(argc, argv_ptrs.data());
        }
    }
    catch (const CLI::CallForHelp &e)
    {
        app_.exit(e);
        throw std::runtime_error("Someone callled for Help.");
    }
    catch (const CLI::ParseError &e)
    {
        throw std::runtime_error(std::format("Command line parse error: {}", e.what()));
    }
}

void PF_AppBase::WaitForTimer(const std::chrono::zoned_seconds &stop_at)
{
    while (true)
    {
        if (PF_AppBase::had_signal_)
        {
            std::cout << "\n*** User interrupted. ***" << std::endl;
            break;
        }

        const std::chrono::zoned_seconds now = std::chrono::zoned_seconds(
            std::chrono::current_zone(), floor<std::chrono::seconds>(std::chrono::system_clock::now()));
        if (now.get_sys_time() < stop_at.get_sys_time())
        {
            std::this_thread::sleep_for(1min);
        }
        else
        {
            std::cout << "\n*** Timer expired. ***" << std::endl;
            PF_AppBase::had_signal_ = true;
            break;
        }
    }
}

void PF_AppBase::HandleSignal(int signal)
{
    PF_AppBase::had_signal_ = true;
}
