// =====================================================================================
//
//       Filename:  Tiingo.h
//
//    Description:  class to live stream ticker updatas
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

#ifndef _TIINGO_INC_
#define _TIINGO_INC_

#include <chrono>
#include <deque>
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
//        Class:  Tiingo
//  Description:  live stream ticker updates -- look like a generator
// =====================================================================================

class Tiingo : public Streamer<Tiingo>
{
   public:
    struct PF_Data
    {
        std::string subscription_id_;
        std::string ticker_;                      // Ticker
        std::string time_stamp_;                  // Date
        int64_t time_stamp_nanoseconds_utc_ = 0;  // time_stamp
        decimal::Decimal last_price_ = 0;         // Last Price
        int32_t last_size_ = 0;                   // Last Size
    };

    // using StreamedData = std::vector<PF_Data>;

    // ====================  LIFECYCLE     =======================================

    Tiingo() = default;
    Tiingo(const Tiingo& rhs) = delete;
    Tiingo(Tiingo&& rhs) = delete;
    Tiingo(const Host& host, const Port& port, const APIKey& api_key, const Prefix& prefix);

    ~Tiingo() = default;

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

    Tiingo& operator=(const Tiingo& rhs) = delete;
    Tiingo& operator=(Tiingo&& rhs) = delete;

   protected:
    // ====================  METHODS       =======================================

    std::string GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                              std::chrono::year_month_day end_date, UpOrDown sort_asc);

    // ====================  DATA MEMBERS  =======================================

   private:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

    std::string subscription_id_;

};  // -----  end of class Tiingo  -----

template <>
struct std::formatter<Tiingo::PF_Data> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Tiingo::PF_Data& pdata, std::format_context& ctx) const
    {
        std::string record;
        std::format_to(std::back_inserter(record), "ticker: {}, price: {}, shares: {}, time: {}", pdata.ticker_,
                       pdata.last_price_, pdata.last_size_, pdata.time_stamp_);
        return formatter<std::string>::format(record, ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Tiingo::PF_Data pf_data)
{
    std::cout << "ticker: " << pf_data.ticker_ << " price: " << pf_data.last_price_ << " shares: " << pf_data.last_size_
              << " time:" << pf_data.time_stamp_;
    return os;
}

#endif  // ----- #ifndef _TIINGO_INC_  -----
