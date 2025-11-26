// =====================================================================================
//
//       Filename:  Tiingo.h
//
//    Description:  class to live stream ticker updatas
//
//        Version:  3.0
//        Created:  2024-04-01 10:09 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef _TIINGO_INC_
#define _TIINGO_INC_

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "Streamer.h"

// =====================================================================================
//        Class:  Tiingo
//  Description:  live stream ticker updates -- look like a generator
// =====================================================================================

class Tiingo : public RemoteDataSource
{
public:
    // ====================  LIFECYCLE     =======================================

    Tiingo() = default;
    Tiingo(const Tiingo &rhs) = delete;
    Tiingo(Tiingo &&rhs) = delete;
    Tiingo(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix);

    ~Tiingo() override = default;

    // ====================  ACCESSORS     =======================================

    TopOfBookList GetTopOfBookAndLastClose() override;
    std::vector<StockDataRecord> GetMostRecentTickerData(const std::string &symbol,
                                                         std::chrono::year_month_day start_from, int how_many_previous,
                                                         UseAdjusted use_adjusted,
                                                         const US_MarketHolidays *holidays) override;

    PF_Data ExtractStreamedData(const std::string &buffer) override;

    // ====================  MUTATORS      =======================================

    void StartStreaming() override;
    void StopStreaming(StreamerContext *streamer_context) override;

    // ====================  OPERATORS     =======================================

    Tiingo &operator=(const Tiingo &rhs) = delete;
    Tiingo &operator=(Tiingo &&rhs) = delete;

protected:
    // ====================  METHODS       =======================================

    std::string GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                              std::chrono::year_month_day end_date, UpOrDown sort_asc);

    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

    std::string subscription_id_;

}; // -----  end of class Tiingo  -----

#endif // ----- #ifndef _TIINGO_INC_  -----
