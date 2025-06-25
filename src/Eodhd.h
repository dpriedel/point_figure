// =====================================================================================
//
//       Filename:  Eodhd.h
//
//    Description:  class to live stream ticker updatas using CRTP approach
//
//        Version:  3.0
//        Created:  2024-04-01 10:10 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef _EODHD_INC_
#define _EODHD_INC_

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "Streamer.h"

// =====================================================================================
//        Class:  Eodhd
//  Description:  live stream ticker updates and retrieve other ticker data -- look like a generator
// =====================================================================================
class Eodhd : public RemoteDataSource
{
public:
    // ====================  LIFECYCLE     =======================================

    Eodhd() = default;
    Eodhd(const Eodhd &rhs) = delete;
    Eodhd(Eodhd &&rhs) = delete;
    Eodhd(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix);

    ~Eodhd() override = default;

    // ====================  ACCESSORS     =======================================

    TopOfBookList GetTopOfBookAndLastClose() override;
    std::vector<StockDataRecord> GetMostRecentTickerData(const std::string &symbol,
                                                         std::chrono::year_month_day start_from, int how_many_previous,
                                                         UseAdjusted use_adjusted,
                                                         const US_MarketHolidays *holidays) override;

    PF_Data ExtractStreamedData(const std::string &buffer) override;

    // ====================  MUTATORS      =======================================

    void StartStreaming() override;
    void StopStreaming() override;

    // ====================  OPERATORS     =======================================

    Eodhd &operator=(const Eodhd &rhs) = delete;
    Eodhd &operator=(Eodhd &&rhs) = delete;

protected:
    // ====================  METHODS       =======================================

    std::string GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                              std::chrono::year_month_day end_date, UpOrDown sort_asc);

    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

}; // -----  end of class Eodhd  -----

#endif // ----- #ifndef _EODHD_INC_  -----
