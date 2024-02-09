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

#ifndef PF_SIGNALS_INC
#define PF_SIGNALS_INC

#include <json/json.h>

#include <chrono>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "PF_Column.h"
#include "utilities.h"

class PF_Chart;

enum class PF_SignalCategory : int32_t
{
    e_unknown,
    e_PF_Buy,
    e_PF_Sell
};  // NOLINT
enum class PF_CanUse1BoxReversal : int32_t
{
    e_Yes,
    e_No
};  // NOLINT

// NOLINTBEGIN(readability-identifier-naming.*)
enum class PF_SignalType : int32_t
{
    e_unknown = 0,
    e_double_top_buy,
    e_double_bottom_sell,
    e_triple_top_buy,
    e_triple_bottom_sell,
    e_bullish_tt_buy,
    e_bearish_tb_sell,
    e_catapult_buy,
    e_catapult_sell,
    e_ttop_catapult_buy,
    e_tbottom_catapult_sell
};

// if we have multiple signals at the same point, show highest priority signal.

enum class PF_SignalPriority : int32_t
{
    e_unknown = -1,
    e_double_top_buy = 1,
    e_double_bottom_sell = 1,
    e_triple_top_buy = 5,
    e_triple_bottom_sell = 5,
    e_bullish_tt_buy = 10,
    e_bearish_tb_sell = 10,
    e_catapult_buy = 5,
    e_catapult_sell = 5,
    e_ttop_catapult_buy = 15,
    e_tbottom_catapult_sell = 15
};

inline int32_t CmpSigPriority(const PF_SignalPriority lhs, const PF_SignalPriority rhs)
{
    return (lhs < rhs ? -1 : lhs > rhs ? 1 : 0);
};

struct PF_Signal
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_unknown;
    PF_SignalType signal_type_ = PF_SignalType::e_unknown;
    PF_SignalPriority priority_ = PF_SignalPriority::e_unknown;
    std::chrono::utc_time<std::chrono::utc_clock::duration> tpt_ = {};
    int32_t column_number_ = -1;
    decimal::Decimal signal_price_ = -1;
    decimal::Decimal box_ = -1;
};

// for Python

inline int32_t CmpSignalsByPriority(const PF_Signal &lhs, const PF_Signal &rhs)
{
    return CmpSigPriority(lhs.priority_, rhs.priority_);
};

using PF_SignalList = std::vector<PF_Signal>;

[[nodiscard]] Json::Value PF_SignalToJSON(const PF_Signal &signal);
[[nodiscard]] PF_Signal PF_SignalFromJSON(const Json::Value &new_data);

// here are some signals we can look for.

struct PF_Catapult_Buy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_catapult_buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_catapult_buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_Yes;
    int32_t minimum_cols_ = 4;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_Catapult_Sell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_catapult_sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_catapult_sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_Yes;
    int32_t minimum_cols_ = 4;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_DoubleTopBuy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_double_top_buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_double_top_buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 3;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TripleTopBuy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_triple_top_buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_triple_top_buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_DoubleBottomSell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_double_bottom_sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_double_bottom_sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 3;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TripleBottomSell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_triple_bottom_sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_triple_bottom_sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_Bullish_TT_Buy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_bullish_tt_buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_bullish_tt_buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_Bearish_TB_Sell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_bearish_tb_sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_bearish_tb_sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 5;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TTopCatapult_Buy
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Buy;
    PF_SignalType signal_type_ = PF_SignalType::e_ttop_catapult_buy;
    PF_SignalPriority priority_ = PF_SignalPriority::e_ttop_catapult_buy;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Up;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 7;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

struct PF_TBottom_Catapult_Sell
{
    PF_SignalCategory signal_category_ = PF_SignalCategory::e_PF_Sell;
    PF_SignalType signal_type_ = PF_SignalType::e_tbottom_catapult_sell;
    PF_SignalPriority priority_ = PF_SignalPriority::e_tbottom_catapult_sell;
    PF_Column::Direction direction_ = PF_Column::Direction::e_Down;
    PF_CanUse1BoxReversal use1box_ = PF_CanUse1BoxReversal::e_No;
    int32_t minimum_cols_ = 7;

    std::optional<PF_Signal> operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                        std::chrono::utc_time<std::chrono::utc_clock::duration> the_time);
};

// this code will update the chart with any signals found for the current inputs
// and report if any were found

std::optional<PF_Signal> LookForNewSignal(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                          PF_Column::TmPt the_time);

// custom formatter

template <>
struct std::formatter<PF_SignalType> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_SignalType &signal_type, std::format_context &ctx) const
    {
        std::string s;
        std::string sig_type;

        switch (signal_type)
        {
            using enum PF_SignalType;
            case e_unknown:
                sig_type = "unknown";
                break;
            case e_double_top_buy:
                sig_type = "double_top_buy";
                break;
            case e_double_bottom_sell:
                sig_type = "double_bottom_sell";
                break;
            case e_triple_top_buy:
                sig_type = "triple_top_buy";
                break;
            case e_triple_bottom_sell:
                sig_type = "triple_bottom_sell";
                break;
            case e_bullish_tt_buy:
                sig_type = "bullish_tt_buy";
                break;
            case e_bearish_tb_sell:
                sig_type = "bearish_tb_sell";
                break;
            case e_catapult_buy:
                sig_type = "catapult_buy";
                break;
            case e_catapult_sell:
                sig_type = "catapult_sell";
                break;
            case e_ttop_catapult_buy:
                sig_type = "ttop_catapult_buy";
                break;
            case e_tbottom_catapult_sell:
                sig_type = "tbottom_catapult_sell";
                break;
        }

        std::format_to(std::back_inserter(s), "{}", sig_type);
        return formatter<std::string>::format(s, ctx);
    }
};

template <>
struct std::formatter<PF_Signal> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Signal &signal, std::format_context &ctx) const
    {
        std::string s;

        std::format_to(std::back_inserter(s),
                       "category: {}. type: {}. priority: {}. time: {:%F %X}. col: {}. "
                       "price "
                       "{} box: {}.",
                       (signal.signal_category_ == PF_SignalCategory::e_PF_Buy    ? "Buy"
                        : signal.signal_category_ == PF_SignalCategory::e_PF_Sell ? "Sell"
                                                                                  : "Unknown"),
                       signal.signal_type_, std::to_underlying(signal.priority_), signal.tpt_, signal.column_number_,
                       signal.signal_price_.format(".2f"), signal.box_.format("f"));

        return formatter<std::string>::format(s, ctx);
    }
};
// NOLINTEND(readability-identifier-naming.*)

#endif  // ----- #ifndef PF_SIGNALS_INC  -----
