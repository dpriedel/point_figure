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

#include "Streamer.h"
#include <json/json.h> // Ensure jsoncpp is available

class Tiingo : public RemoteDataSource
{
public:
    Tiingo() = default;
    Tiingo(const Host &host, const Port &port, const APIKey &api_key, const Prefix &prefix);
    ~Tiingo() override = default;

    TopOfBookList GetTopOfBookAndLastClose() override;
    std::vector<StockDataRecord> GetMostRecentTickerData(const std::string &symbol,
                                                         std::chrono::year_month_day start_from, int how_many_previous,
                                                         UseAdjusted use_adjusted,
                                                         const US_MarketHolidays *holidays) override;

    PF_Data ExtractStreamedData(const std::string &buffer) override;

    // Async Hook
    void OnConnected() override;
    void StopStreaming(StreamerContext &streamer_context) override;

protected:
    std::string GetTickerData(std::string_view symbol, std::chrono::year_month_day start_date,
                              std::chrono::year_month_day end_date, UpOrDown sort_asc);

    // Async Handlers for Subscription
    void on_write_subscribe(beast::error_code ec, std::size_t bytes_transferred);
    void on_read_subscribe(beast::error_code ec, std::size_t bytes_transferred);

private:
    std::string subscription_id_;
};

#endif
