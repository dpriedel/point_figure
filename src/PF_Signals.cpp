// =====================================================================================
//
//       Filename:  PF_Signals.cpp
//
//    Description:  Place for code related to finding trading 'signals'
//                  in PF_Chart data.
//
//        Version:  1.0
//        Created:  09/21/2022 09:27:22 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
//
// =====================================================================================

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <ranges>
#include <utility>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <spdlog/spdlog.h>

#include "Boxes.h"
#include "PF_Chart.h"
#include "PF_Signals.h"

// common code to determine whether can test for a signal

bool CanApplySignal(const PF_Chart &the_chart, const auto &signal);

using signal_function = std::function<std::optional<PF_Signal>(
    const PF_Chart &, const decimal::Decimal &, std::chrono::utc_time<std::chrono::utc_clock::duration>)>;

// order functions in table by decreasing priority

const std::vector<signal_function> sig_funcs = {PF_TTopCatapult_Buy(), PF_TBottom_Catapult_Sell(), PF_Bullish_TT_Buy(),
                                                PF_Bearish_TB_Sell(),  PF_Catapult_Buy(),          PF_Catapult_Sell(),
                                                PF_TripleTopBuy(),     PF_TripleBottomSell(),      PF_DoubleTopBuy(),
                                                PF_DoubleBottomSell()};

// ===  FUNCTION
// ======================================================================
//         Name:  CanApplySignal
//  Description:
// =====================================================================================
bool CanApplySignal(const PF_Chart &the_chart, const auto &signal)
{
    if (signal.use1box_ == PF_CanUse1BoxReversal::e_Yes && the_chart.GetReversalboxes() != 1)
    {
        return false;
    }

    if (signal.use1box_ == PF_CanUse1BoxReversal::e_No && the_chart.GetReversalboxes() == 1)
    {
        return false;
    }

    if (the_chart.back().GetDirection() != signal.direction_)
    {
        return false;
    }

    auto number_cols = the_chart.size();
    if (std::cmp_less(number_cols, signal.minimum_cols_))
    {
        // too few columns

        return false;
    }

    // see if we already have this signal for this column

    if (auto found_it =
            rng::find_if(the_chart.GetSignals(), [number_cols, signal](const PF_Signal &sig)
                         { return sig.column_number_ == number_cols - 1 && sig.signal_type_ == signal.signal_type_; });
        found_it != the_chart.GetSignals().end())
    {
        // already have a signal of this type

        return false;
    }

    return true;
}  // -----  end of method CanApplySignal  -----

// ===  FUNCTION
// ======================================================================
//         Name:  AddSignalsToChart
//  Description:
// =====================================================================================
std::optional<PF_Signal> LookForNewSignal(PF_Chart &the_chart, const decimal::Decimal &new_value,
                                          PF_Column::TmPt the_time)
{
    for (const auto &sig : sig_funcs)
    {
        if (auto new_sig = sig(the_chart, new_value, the_time); new_sig)
        {
            spdlog::debug(std::format("Found signal: {}", new_sig.value()));

            // since signal checks are ordered in decreasing priority,
            // stop after the first match since it will be the highest priorit
            // signal at this point

            return {new_sig};
        }
    }
    return {};
}  // -----  end of function AddSignalsToChart  -----

Json::Value PF_SignalToJSON(const PF_Signal &signal)
{
    Json::Value result;
    switch (signal.signal_category_)
    {
        using enum PF_SignalCategory;
        case e_unknown:
            result["category"] = "unknown";
            break;

        case e_PF_Buy:
            result["category"] = "buy";
            break;

        case e_PF_Sell:
            result["category"] = "sell";
            break;
    };

    switch (signal.signal_type_)
    {
        using enum PF_SignalType;
        case e_unknown:
            result["type"] = "unknown";
            break;

        case e_double_top_buy:
            result["type"] = "dt_buy";
            break;

        case e_triple_top_buy:
            result["type"] = "tt_buy";
            break;

        case e_double_bottom_sell:
            result["type"] = "db_sell";
            break;

        case e_triple_bottom_sell:
            result["type"] = "tb_sell";
            break;

        case e_bullish_tt_buy:
            result["type"] = "bullish_tt_buy";
            break;

        case e_bearish_tb_sell:
            result["type"] = "bearish_tb_sell";
            break;

        case e_catapult_buy:
            result["type"] = "catapult_buy";
            break;

        case e_catapult_sell:
            result["type"] = "catapult_sell";
            break;

        case e_ttop_catapult_buy:
            result["type"] = "ttop_catapult_buy";
            break;

        case e_tbottom_catapult_sell:
            result["type"] = "tbot_catapult_sell";
            break;
    };

    result["priority"] = std::to_underlying(signal.priority_);

    result["time"] = signal.tpt_.time_since_epoch().count();
    result["column"] = signal.column_number_;
    result["price"] = signal.signal_price_.format(".2f");
    result["box"] = signal.box_.format("f");

    return result;
}  // -----  end of method PF_SignalToJSON  -----

PF_Signal PF_SignalFromJSON(const Json::Value &new_data)
{
    PF_Signal new_sig;

    if (const auto category = new_data["category"].asString(); category == "buy")
    {
        new_sig.signal_category_ = PF_SignalCategory::e_PF_Buy;
    }
    else if (category == "sell")
    {
        new_sig.signal_category_ = PF_SignalCategory::e_PF_Sell;
    }
    else if (category == "unknown")
    {
        new_sig.signal_category_ = PF_SignalCategory::e_unknown;
    }
    else
    {
        throw std::invalid_argument{
            std::format("Invalid category provided: {}. Must be 'buy', 'sell', 'unknown'.", category)};
    }

    if (const auto type = new_data["type"].asString(); type == "dt_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_double_top_buy;
    }
    else if (type == "tt_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_triple_top_buy;
    }
    else if (type == "db_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_double_bottom_sell;
    }
    else if (type == "tb_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_triple_bottom_sell;
    }
    else if (type == "bullish_tt_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_bullish_tt_buy;
    }
    else if (type == "bearish_tb_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_bearish_tb_sell;
    }
    else if (type == "catapult_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_catapult_buy;
    }
    else if (type == "catapult_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_catapult_sell;
    }
    else if (type == "ttop_catapult_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_ttop_catapult_buy;
    }
    else if (type == "tbot_catapult_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_tbottom_catapult_sell;
    }
    else if (type == "unknown")
    {
        new_sig.signal_type_ = PF_SignalType::e_unknown;
    }
    else
    {
        throw std::invalid_argument{
            std::format("Invalid signal type provided: {}. Must be 'dt_buy', "
                        "'tt_buy' 'db_sell', 'tb_sell', 'unknown'.",
                        type)};
    }

    new_sig.priority_ = static_cast<PF_SignalPriority>(new_data["priority"].asInt());
    new_sig.tpt_ = std::chrono::utc_time<std::chrono::utc_clock::duration>{
        std::chrono::utc_clock::duration{new_data["time"].asInt64()}};
    new_sig.column_number_ = new_data["column"].asInt();
    new_sig.signal_price_ = decimal::Decimal{new_data["price"].asString()};
    new_sig.box_ = decimal::Decimal{new_data["box"].asString()};

    return new_sig;
}  // -----  end of method PF_SignalFromJSON  -----

std::optional<PF_Signal> PF_Catapult_Buy::operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                                     std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // remember: column numbers count from zero.

    auto current_top = the_chart.back().GetTop();

    // std::print("col num: {} top: {}\n", number_cols - 1, current_top);

    // these patterns can be wide in 1-box reversal charts.  Set a leftmost
    // boundary by looking for any column that was higher than this one.

    int32_t boundary_column{-1};
    for (int32_t index = number_cols - 2; index > -1; --index)
    {
        // std::print("index1: {}\n", index);
        if (the_chart[index].GetTop() >= current_top)
        {
            boundary_column = index;
            break;
        }
    }

    // std::print("boundary column: {}\n", boundary_column);

    // we finally get to apply our rule
    // first, we need to find the previous column in our direction with a top
    // below ours.

    int32_t which_prev_col{-1};
    auto previous_top = the_chart.GetBoxes().FindPrevBox(current_top);

    for (int32_t index = number_cols - 2; index > boundary_column; --index)
    {
        // std::print("index2: {}\n", index);
        if (the_chart[index].GetDirection() == PF_Column::Direction::e_Up && the_chart[index].GetTop() == previous_top)
        {
            which_prev_col = index;
            break;
        }
    }

    // std::print("prev col: {} prev top: {}\n", which_prev_col, previous_top);

    if (which_prev_col > -1)
    {
        int32_t ctr{0};

        for (int32_t index = which_prev_col - 1; index > boundary_column; --index)
        {
            //            std::print("index2: {}\n", index);
            if (the_chart[index].GetDirection() == PF_Column::Direction::e_Up &&
                the_chart[index].GetTop() == previous_top)
            {
                ++ctr;
            }
        }

        if (ctr > 0)
        {
            // price could jump several boxes but we want to set the signal at the
            // next box higher than the last column top.

            return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Buy,
                              .signal_type_ = PF_SignalType::e_catapult_buy,
                              .priority_ = PF_SignalPriority::e_catapult_buy,
                              .tpt_ = the_time,
                              .column_number_ = static_cast<int32_t>(number_cols - 1),
                              .signal_price_ = new_value,
                              .box_ = the_chart.GetBoxes().FindNextBox(previous_top)}};
        }
    }

    return {};
}  // -----  end of method PF_Catapult_Up::operator()  -----

std::optional<PF_Signal> PF_Catapult_Sell::operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                                      std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    // these patterns can be wide in 1-box reversal charts.  Set a leftmost
    // boundary by looking for any column that was higher than this one.

    int32_t boundary_column{-1};
    auto number_cols = the_chart.size();

    // remember: column numbers count from zero.

    auto current_bottom = the_chart[number_cols - 1].GetBottom();

    // std::print("col num: {} top: {}\n", number_cols - 1, current_top);

    for (int32_t index = number_cols - 2; index > -1; --index)
    {
        // std::print("index1: {}\n", index);
        if (the_chart[index].GetBottom() <= current_bottom)
        {
            boundary_column = index;
            break;
        }
    }

    // std::print("boundary column: {}\n", boundary_column);

    // we finally get to apply our rule
    // first, we need to find the previous column in our direction with a bottom
    // above ours.

    int32_t which_prev_col{-1};
    auto previous_bottom = the_chart.GetBoxes().FindNextBox(current_bottom);

    for (int32_t index = number_cols - 2; index > boundary_column; --index)
    {
        // std::print("index2: {}\n", index);
        if (the_chart[index].GetDirection() == PF_Column::Direction::e_Down &&
            the_chart[index].GetBottom() == previous_bottom)
        {
            which_prev_col = index;
            break;
        }
    }

    // std::print("prev col: {} prev top: {}\n", which_prev_col, previous_top);

    if (which_prev_col > -1)
    {
        int32_t ctr{0};

        for (int32_t index = which_prev_col - 1; index > boundary_column; --index)
        {
            //            std::print("index2: {}\n", index);
            if (the_chart[index].GetDirection() == PF_Column::Direction::e_Down &&
                the_chart[index].GetBottom() == previous_bottom)
            {
                ++ctr;
            }
        }

        if (ctr > 0)
        {
            // price could jump several boxes but we want to set the signal at the
            // next box higher than the last column top.

            return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Sell,
                              .signal_type_ = PF_SignalType::e_catapult_sell,
                              .priority_ = PF_SignalPriority::e_catapult_sell,
                              .tpt_ = the_time,
                              .column_number_ = static_cast<int32_t>(number_cols - 1),
                              .signal_price_ = new_value,
                              .box_ = the_chart.GetBoxes().FindPrevBox(previous_bottom)}};
        }
    }
    return {};
}  // -----  end of method PF_DoubleTopBuy::operator()  -----

std::optional<PF_Signal> PF_DoubleTopBuy::operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                                     std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top = the_chart[number_cols - 3].GetTop();
    if (the_chart.back().GetTop() > previous_top)
    {
        // price could jump several boxes but we want to set the signal at the next
        // box higher than the last column top.

        return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Buy,
                          .signal_type_ = PF_SignalType::e_double_top_buy,
                          .priority_ = PF_SignalPriority::e_double_top_buy,
                          .tpt_ = the_time,
                          .column_number_ = static_cast<int32_t>(number_cols - 1),
                          .signal_price_ = new_value,
                          .box_ = the_chart.GetBoxes().FindNextBox(previous_top)}};
    }
    return {};
}  // -----  end of method PF_Catapult_Down::operator()  -----

std::optional<PF_Signal> PF_TripleTopBuy::operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                                     std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top_1 = the_chart[number_cols - 3].GetTop();
    auto previous_top_0 = the_chart[number_cols - 5].GetTop();
    if (the_chart.back().GetTop() > previous_top_1 && previous_top_0 == previous_top_1)
    {
        // price could jump several boxes but we want to set the signal at the next
        // box higher than the last column top.

        return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Buy,
                          .signal_type_ = PF_SignalType::e_triple_top_buy,
                          .priority_ = PF_SignalPriority::e_triple_top_buy,
                          .tpt_ = the_time,
                          .column_number_ = static_cast<int32_t>(number_cols - 1),
                          .signal_price_ = new_value,
                          .box_ = the_chart.GetBoxes().FindNextBox(previous_top_1)}};
    }
    return {};
}  // -----  end of method PF_TripleTopBuy::operator()  -----

std::optional<PF_Signal> PF_DoubleBottomSell::operator()(
    const PF_Chart &the_chart, const decimal::Decimal &new_value,
    std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_bottom = the_chart[number_cols - 3].GetBottom();
    if (the_chart.back().GetBottom() < previous_bottom)
    {
        // price could jump several boxes but we want to set the signal at the next
        // box higher than the last column top.

        return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Sell,
                          .signal_type_ = PF_SignalType::e_double_bottom_sell,
                          .priority_ = PF_SignalPriority::e_double_bottom_sell,
                          .tpt_ = the_time,
                          .column_number_ = static_cast<int32_t>(number_cols - 1),
                          .signal_price_ = new_value,
                          .box_ = the_chart.GetBoxes().FindPrevBox(previous_bottom)}};
    }
    return {};
}  // -----  end of method PF_DoubleBottomSell::operator()  -----

std::optional<PF_Signal> PF_TripleBottomSell::operator()(
    const PF_Chart &the_chart, const decimal::Decimal &new_value,
    std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_bottom_1 = the_chart[number_cols - 3].GetBottom();
    auto previous_bottom_0 = the_chart[number_cols - 5].GetBottom();
    if (the_chart.back().GetBottom() < previous_bottom_1 && previous_bottom_0 == previous_bottom_1)
    {
        // price could jump several boxes but we want to set the signal at the next
        // box higher than the last column top.

        return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Sell,
                          .signal_type_ = PF_SignalType::e_triple_bottom_sell,
                          .priority_ = PF_SignalPriority::e_triple_bottom_sell,
                          .tpt_ = the_time,
                          .column_number_ = static_cast<int32_t>(number_cols - 1),
                          .signal_price_ = new_value,
                          .box_ = the_chart.GetBoxes().FindPrevBox(previous_bottom_1)}};
    }
    return {};
}  // -----  end of method PF_TripleBottomSell::operator()  -----

std::optional<PF_Signal> PF_Bullish_TT_Buy::operator()(const PF_Chart &the_chart, const decimal::Decimal &new_value,
                                                       std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top_1 = the_chart[number_cols - 3].GetTop();
    auto previous_top_0 = the_chart[number_cols - 5].GetTop();
    if ((the_chart.back().GetTop() > previous_top_1) && (previous_top_1 > previous_top_0) &&
        (the_chart.back().GetBottom() > the_chart[number_cols - 3].GetBottom()) &&
        (the_chart[number_cols - 3].GetBottom() > the_chart[number_cols - 5].GetBottom()))
    {
        // price could jump several boxes but we want to set the signal at the next
        // box higher than the last column top.

        return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Buy,
                          .signal_type_ = PF_SignalType::e_bullish_tt_buy,
                          .priority_ = PF_SignalPriority::e_bullish_tt_buy,
                          .tpt_ = the_time,
                          .column_number_ = static_cast<int32_t>(number_cols - 1),
                          .signal_price_ = new_value,
                          .box_ = the_chart.GetBoxes().FindNextBox(previous_top_1)}};
    }
    return {};
}  // -----  end of method PF_Bullish_TT_Buy::operator()  -----

std::optional<PF_Signal> PF_Bearish_TB_Sell::operator()(
    const PF_Chart &the_chart, const decimal::Decimal &new_value,
    std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    auto number_cols = the_chart.size();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_bottom_1 = the_chart[number_cols - 3].GetBottom();
    auto previous_bottom_0 = the_chart[number_cols - 5].GetBottom();
    if ((the_chart.back().GetBottom() < previous_bottom_1) && (previous_bottom_1 < previous_bottom_0) &&
        (the_chart.back().GetTop() < the_chart[number_cols - 3].GetTop()) &&
        (the_chart[number_cols - 3].GetTop() < the_chart[number_cols - 5].GetTop()))
    {
        // price could jump several boxes but we want to set the signal at the next
        // box higher than the last column top.

        return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Sell,
                          .signal_type_ = PF_SignalType::e_bearish_tb_sell,
                          .priority_ = PF_SignalPriority::e_bearish_tb_sell,
                          .tpt_ = the_time,
                          .column_number_ = static_cast<int32_t>(number_cols - 1),
                          .signal_price_ = new_value,
                          .box_ = the_chart.GetBoxes().FindPrevBox(previous_bottom_1)}};
    }
    return {};
}  // -----  end of method PF_Bearish_TB_Sell::operator()  -----

std::optional<PF_Signal> PF_TTopCatapult_Buy::operator()(
    const PF_Chart &the_chart, const decimal::Decimal &new_value,
    std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    // this signal is basically a double-top buy immediately preceeded by a
    // triple-top buy with no intervening sell signal

    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    // first, do we have a double-top buy in this column

    auto number_cols = the_chart.size();

    PF_Signal dtop_buy;

    if (auto found_it = rng::find_if(
            the_chart.GetSignals(), [this_col = number_cols - 1](const PF_Signal &sig)
            { return sig.column_number_ == this_col && sig.signal_type_ == PF_SignalType::e_double_top_buy; });
        found_it == the_chart.GetSignals().end())
    {
        return {};
    }
    else
    {
        dtop_buy = *found_it;
    }

    // next, make sure there is no sell signal for previous column.

    if (auto found_it = rng::find_if(
            the_chart.GetSignals(), [prev_col = number_cols - 2](const PF_Signal &sig)
            { return sig.column_number_ == prev_col && sig.signal_category_ == PF_SignalCategory::e_PF_Sell; });
        found_it != the_chart.GetSignals().end())
    {
        return {};
    }

    // now, look for preceding triple-top buy

    if (auto found_it = rng::find_if(the_chart.GetSignals(),
                                     [prev_col = number_cols - 3](const PF_Signal &sig)
                                     {
                                         return sig.column_number_ == prev_col &&
                                                (sig.signal_type_ == PF_SignalType::e_triple_top_buy ||
                                                 sig.signal_type_ == PF_SignalType::e_bullish_tt_buy);
                                     });
        found_it == the_chart.GetSignals().end())
    {
        return {};
    }

    // price could jump several boxes but we want to set the signal at the next
    // box higher than the last column top.

    return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Buy,
                      .signal_type_ = PF_SignalType::e_ttop_catapult_buy,
                      .priority_ = PF_SignalPriority::e_ttop_catapult_buy,
                      .tpt_ = the_time,
                      .column_number_ = static_cast<int32_t>(number_cols - 1),
                      .signal_price_ = new_value,
                      .box_ = dtop_buy.box_}};
}  // -----  end of method PF_TTopCatapult_Buy::operator()  -----

std::optional<PF_Signal> PF_TBottom_Catapult_Sell::operator()(
    const PF_Chart &the_chart, const decimal::Decimal &new_value,
    std::chrono::utc_time<std::chrono::utc_clock::duration> the_time)
{
    // this signal is basically a double-bottom sell immediately preceeded by a
    // triple-bottom sell with no intervening buy signal

    if (!CanApplySignal(the_chart, *this))
    {
        return {};
    }

    // first, do we have a double-top sell in this column

    auto number_cols = the_chart.size();

    PF_Signal dbot_sell;

    if (auto found_it = rng::find_if(
            the_chart.GetSignals(), [this_col = number_cols - 1](const PF_Signal &sig)
            { return sig.column_number_ == this_col && sig.signal_type_ == PF_SignalType::e_double_bottom_sell; });
        found_it == the_chart.GetSignals().end())
    {
        return {};
    }
    else
    {
        dbot_sell = *found_it;
    }

    // next, make sure there is no buy signal for previous column.

    if (auto found_it = rng::find_if(
            the_chart.GetSignals(), [prev_col = number_cols - 2](const PF_Signal &sig)
            { return sig.column_number_ == prev_col && sig.signal_category_ == PF_SignalCategory::e_PF_Buy; });
        found_it != the_chart.GetSignals().end())
    {
        return {};
    }

    // now, look for preceding triple-bottom sell

    if (auto found_it = rng::find_if(the_chart.GetSignals(),
                                     [prev_col = number_cols - 3](const PF_Signal &sig)
                                     {
                                         return sig.column_number_ == prev_col &&
                                                (sig.signal_type_ == PF_SignalType::e_triple_bottom_sell ||
                                                 sig.signal_type_ == PF_SignalType::e_bearish_tb_sell);
                                     });
        found_it == the_chart.GetSignals().end())
    {
        return {};
    }

    // price could jump several boxes but we want to set the signal at the next
    // box higher than the last column top.

    return {PF_Signal{.signal_category_ = PF_SignalCategory::e_PF_Sell,
                      .signal_type_ = PF_SignalType::e_tbottom_catapult_sell,
                      .priority_ = PF_SignalPriority::e_tbottom_catapult_sell,
                      .tpt_ = the_time,
                      .column_number_ = static_cast<int32_t>(number_cols - 1),
                      .signal_price_ = new_value,
                      .box_ = dbot_sell.box_}};
}  // -----  end of method PF_TTopCatapult_Buy::operator()  -----
