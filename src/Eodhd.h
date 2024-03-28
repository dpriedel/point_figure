// =====================================================================================
//
//       Filename:  Eodhd.h
//
//    Description:  class to live stream ticker updatas using CRTP approach
//
//        Version:  2.0
//        Created:  03/23/2024 09:26:57 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#ifndef _EODHD_INC_
#define _EODHD_INC_

#include <cstdint>
#include <format>
#include <mutex>
#include <queue>
#include <sys/types.h>
#include <vector>

#include <decimal.hh>
#include <json/json.h>

// #pragma GCC diagnostic push
// #pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "Streamer.h"

// =====================================================================================
//        Class:  Eodhd
//  Description:  live stream ticker updates and retrieve other ticker data -- look like a generator
// =====================================================================================
class Eodhd : public Streamer<Eodhd>
{
   public:
    enum class EodMktStatus : int32_t
    {
        e_unknown,
        e_open,
        e_closed,
        e_extended_hours
    };

    struct PF_Data
    {
        std::string ticker_;               // Ticker
        std::string time_stamp_;           // Date
        TmPt time_stamp_nanoseconds_utc_;  // time_stamp
        decimal::Decimal last_price_ = 0;  // Last Price
        int32_t last_size_{-1};            // Last Size
        bool dark_pool_{false};
        EodMktStatus market_status_{EodMktStatus::e_unknown};
    };

    // using StreamedData = std::vector<PF_Data>;

    // ====================  LIFECYCLE     =======================================

    Eodhd() = default;
    Eodhd(const Eodhd& rhs) = delete;
    Eodhd(Eodhd&& rhs) = delete;
    Eodhd(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix);

    ~Eodhd() = default;

    // ====================  ACCESSORS     =======================================

    TopOfBookList GetTopOfBookAndLastClose();
    std::vector<StockDataRecord> GetMostRecentTickerData(const std::string& symbol,
                                                         std::chrono::year_month_day start_from, int how_many_previous,
                                                         UseAdjusted use_adjusted,
                                                         const US_MarketHolidays* holidays = nullptr);

    PF_Data ExtractData(const std::string& buffer);

    // ====================  MUTATORS      =======================================

    void StartStreaming();
    void StopStreaming();

    // ====================  OPERATORS     =======================================

    Eodhd& operator=(const Eodhd& rhs) = delete;
    Eodhd& operator=(Eodhd&& rhs) = delete;

   protected:
    // ====================  METHODS       =======================================

    std::string GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                              std::chrono::year_month_day end_date, UpOrDown sort_asc);

    // ====================  DATA MEMBERS  =======================================

   private:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

};  // -----  end of class Eodhd  -----

template <>
struct std::formatter<Eodhd::PF_Data> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Eodhd::PF_Data& pdata, std::format_context& ctx) const
    {
        std::string record;
        std::format_to(std::back_inserter(record), "ticker: {}, price: {}, shares: {}, time: {:%F %T}", pdata.ticker_,
                       pdata.last_price_, pdata.last_size_, pdata.time_stamp_nanoseconds_utc_);
        return formatter<std::string>::format(record, ctx);
    }
};

#endif  // ----- #ifndef _EODHD_INC_  -----
