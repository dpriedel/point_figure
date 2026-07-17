#ifndef PF_SCANNERAPP_INC
#define PF_SCANNERAPP_INC

#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "common/PF_AppBase.h"

class PF_ScannerApp : public PF_AppBase
{
public:
    PF_ScannerApp(int argc, char *argv[]);
    explicit PF_ScannerApp(const std::vector<std::string> &tokens);

    PF_ScannerApp() = delete;
    PF_ScannerApp(const PF_ScannerApp &) = delete;
    PF_ScannerApp(PF_ScannerApp &&) = delete;

    bool Startup();
    std::tuple<int, int, int> Run();
    void Shutdown();

    PF_ScannerApp &operator=(const PF_ScannerApp &) = delete;
    PF_ScannerApp &operator=(PF_ScannerApp &&) = delete;

private:
    void SetupProgramOptions();
    bool CheckArgs();

    std::tuple<int, int, int> Run_DailyScan();
    std::pair<int, int> CountChartReversalsUpAndDown() const;
    std::pair<int, int> CountChartTrendsContinueUpAndDown() const;
    std::pair<int, int> CountChartTrendsUnanimousUpAndDown() const;

    std::vector<std::string> exchange_list_;
    std::string min_dollar_volume_;
    std::string begin_date_;
    std::string end_date_;
    std::string price_fld_name_;
};

#endif
