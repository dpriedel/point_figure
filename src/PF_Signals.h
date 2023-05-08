// =====================================================================================
//
//       Filename:  PF_Signals.h
//
//    Description:  Place for code related to finding trading 'signals'
//                  in PF_Chart data.
//
//        Version:  1.0
//        Created:  09/21/2022 09:26:12 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================


#ifndef  PF_SIGNALS_INC
#define  PF_SIGNALS_INC

#include <chrono>
#include <format>
#include <optional>
#include <utility>
#include <vector>

// #include <date/date.h>
// #include <date/tz.h>

// #include <fmt/format.h>
// #include <fmt/chrono.h>
#include <json/json.h>

#include "DDecQuad.h"
#include "PF_Column.h"
#include "utilities.h"

class PF_Chart;

enum class PF_SignalCategory {e_Unknown, e_PF_Buy, e_PF_Sell};
enum class PF_CanUse1BoxReversal {e_Yes, e_No};
enum class PF_SignalType {
    e_Unknown,
    e_DoubleTop_Buy,
    e_DoubleBottom_Sell,
    e_TripleTop_Buy,
    e_TripleBottom_Sell,
    e_Bullish_TT_Buy,
    e_Bearish_TB_Sell,
    e_Catapult_Buy,
    e_Catapult_Sell,
    e_TTop_Catapult_Buy,
    e_TBottom_Catapult_Sell
};


// if we have multiple signals at the same point, show highest priority signal.

enum class PF_SignalPriority {
    e_Unknown = -1,
    e_DoubleTop_Buy = 1,
    e_DoubleBottom_Sell = 1,
    e_TripleTop_Buy = 5,
    e_TripleBottom_Sell = 5,
    e_Bullish_TT_Buy = 10,
    e_Bearish_TB_Sell = 10,
    e_Catapult_Buy = 5,
    e_Catapult_Sell = 5,
    e_TTop_Catapult_Buy = 15,
    e_TBottom_Catapult_Sell = 15
};

struct PF_Signal
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_Unknown;
    PF_SignalType signal_type_ = PF_SignalType::e_Unknown;
    PF_SignalPriority priority_ = PF_SignalPriority::e_Unknown;
    std::chrono::utc_time<std::chrono::utc_clock::duration> tpt_ = {};
    int32_t column_number_ = -1;
    DprDecimal::DDecQuad signal_price_ = -1;
    DprDecimal::DDecQuad box_ = -1;
};

using PF_SignalList = std::vector<PF_Signal>;

[[nodiscard]] Json::Value PF_SignalToJSON(const PF_Signal& signal);
[[nodiscard]] PF_Signal PF_SignalFromJSON(const Json::Value& new_data);

// here are some signals we can look for.

struct PF_Catapult_Buy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_Catapult_Buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_Catapult_Buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_Yes;
    int32_t minimum_cols_ = 4;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_Catapult_Sell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_Catapult_Sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_Catapult_Sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_Yes;
    int32_t minimum_cols_ = 4;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_DoubleTopBuy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_DoubleTop_Buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_DoubleTop_Buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 3;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TripleTopBuy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_TripleTop_Buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_TripleTop_Buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_DoubleBottomSell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_DoubleBottom_Sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_DoubleBottom_Sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 3;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TripleBottomSell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_TripleBottom_Sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_TripleBottom_Sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_Bullish_TT_Buy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_Bullish_TT_Buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_Bullish_TT_Buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_Bearish_TB_Sell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_Bearish_TB_Sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_Bearish_TB_Sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TTopCatapult_Buy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_TTop_Catapult_Buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_TTop_Catapult_Buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 7;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TBottom_Catapult_Sell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_TBottom_Catapult_Sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_TBottom_Catapult_Sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 7;

    std::optional<PF_Signal> operator()(const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

// this code will update the chart with any signals found for the current inputs
// and report if any were found

bool AddSignalsToChart(PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, PF_Column::TmPt the_time);

// custom formatter 

template <> struct std::formatter<PF_Signal>: std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Signal& signal, std::format_context& ctx) const
    {
        std::string s;
        std::string sig_type;
        
        switch (signal.signal_type_)
        {
            using enum PF_SignalType;
            case e_Unknown:
                sig_type = "Unknown";
                break;
            case e_DoubleTop_Buy:
                sig_type = "DblTop Buy";
                break;
            case e_DoubleBottom_Sell:
                sig_type = "DblBtm Sell";
                break;
            case e_TripleTop_Buy:
                sig_type = "TripleTop Buy";
                break;
            case e_TripleBottom_Sell:
                sig_type = "TripleBtm Sell";
                break;
            case e_Bullish_TT_Buy:
                sig_type = "Bullish TT Buy";
                break;
            case e_Bearish_TB_Sell:
                sig_type = "Bearish TB Sell";
                break;
            case e_Catapult_Buy:
                sig_type = "Catapult Buy";
                break;
            case e_Catapult_Sell:
                sig_type = "Catapult Sell";
                break;
            case e_TTop_Catapult_Buy:
                sig_type = "TTop Catapult Buy";
                break;
            case e_TBottom_Catapult_Sell:
                sig_type = "TBtm Catapult Sell";
                break;
        }

        std::format_to(std::back_inserter(s), "category: {}. type: {}. priority: {}. time: {:%F %X}. col: {}. price {} box: {}.",
           (signal.signal_category_ == PF_SignalCategory::e_PF_Buy ? "Buy" : signal.signal_category_ == PF_SignalCategory::e_PF_Sell ? "Sell" : "Unknown"),
           sig_type,
           std::to_underlying(signal.priority_),
           signal.tpt_,
           signal.column_number_,
           signal.signal_price_,
           signal.box_
       );

        return formatter<std::string>::format(s, ctx);
    }
};

#endif   // ----- #ifndef PF_SIGNALS_INC  ----- 
