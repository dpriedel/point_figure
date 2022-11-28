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
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/reverse.hpp>

#include "Boxes.h"
#include "PF_Chart.h"
#include "PF_Signals.h"

Json::Value PF_SignalToJSON(const PF_Signal& signal)
{
    Json::Value result;
    switch(signal.signal_category_)
    {
        using enum PF_SignalCategory;
        case e_Unknown:
            result["category"] = "unknown";
            break;

        case e_PF_Buy:
            result["category"] = "buy";
            break;

        case e_PF_Sell:
            result["category"] = "sell";
            break;
    };

    switch(signal.signal_type_)
    {
        using enum PF_SignalType;
        case e_Unknown:
            result["type"] = "unknown";
            break;

        case e_DoubleTop_Buy:
            result["type"] = "dt_buy";
            break;

        case e_TripleTop_Buy:
            result["type"] = "tt_buy";
            break;

        case e_DoubleBottom_Sell:
            result["type"] = "db_sell";
            break;

        case e_TripleBottom_Sell:
            result["type"] = "tb_sell";
            break;

        case e_Bullish_TT_Buy:
            result["type"] = "bullish_tt_buy";
            break;

        case e_Bearish_TB_Sell:
            result["type"] = "bearish_tb_sell";
            break;

        case e_Catapult_Up_Buy:
            result["type"] = "catapult_buy";
            break;

        case e_Catapult_Down_Sell:
            result["type"] = "catapult_sell";
            break;
    };

    result["priority"] = std::to_underlying(signal.priority_);

    result["time"] = signal.tpt_.time_since_epoch().count();
    result["column"] = signal.column_number_;
    result["price"] = signal.signal_price_.ToStr();
    result["box"] = signal.box_.ToStr();

    return result;
}		// -----  end of method PF_SignalToJSON  ----- 

PF_Signal PF_SignalFromJSON(const Json::Value& new_data)
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
        new_sig.signal_category_ = PF_SignalCategory::e_Unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid category provided: {}. Must be 'buy', 'sell', 'unknown'.", category)};
    }

    if (const auto type = new_data["type"].asString(); type == "dt_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_DoubleTop_Buy;
    }
    else if (type == "tt_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_TripleTop_Buy;
    }
    else if (type == "db_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_DoubleBottom_Sell;
    }
    else if (type == "tb_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_TripleBottom_Sell;
    }
    else if (type == "bullish_tt_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_Bullish_TT_Buy;
    }
    else if (type == "bearish_tb_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_Bearish_TB_Sell;
    }
    else if (type == "catapult_buy")
    {
        new_sig.signal_type_ = PF_SignalType::e_Catapult_Up_Buy;
    }
    else if (type == "catapult_sell")
    {
        new_sig.signal_type_ = PF_SignalType::e_Catapult_Down_Sell;
    }
    else if (type == "unknown")
    {
        new_sig.signal_type_ = PF_SignalType::e_Unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid signal type provided: {}. Must be 'dt_buy', 'tt_buy' 'db_sell', 'tb_sell', 'unknown'.", type)};
    }

    new_sig.priority_ = static_cast<PF_SignalPriority>(new_data["priority"].asInt());
    new_sig.tpt_ = date::utc_time<date::utc_clock::duration>{date::utc_clock::duration{new_data["time"].asInt64()}};
    new_sig.column_number_ = new_data["column"].asInt();
    new_sig.signal_price_ = DprDecimal::DDecQuad{new_data["price"].asString()};
    new_sig.box_ = DprDecimal::DDecQuad{new_data["box"].asString()};

    return new_sig;
}		// -----  end of method PF_SignalFromJSON  ----- 

bool CanApplySignal(const PF_Chart& the_chart, PF_SignalType signal_type, PF_Column::Direction direction, int32_t minimum_cols, PF_CanUse1BoxReversal use1box)
{
	// so far, all signals expect 3-box reversal but will work correctly for any reversal 
	// size > 1 box.
	
	if (use1box == PF_CanUse1BoxReversal::e_Yes && the_chart.GetReversalboxes() != 1)
	{
		return false;
	}

	if (use1box == PF_CanUse1BoxReversal::e_No && the_chart.GetReversalboxes() == 1)
	{
		return false;
	}

	if (the_chart.GetCurrentColumn().GetDirection() != direction)
	{
	    return false;
	}
	int32_t number_cols = the_chart.GetNumberOfColumns();
	if (number_cols < minimum_cols)
	{
	    // too few columns

	    return false;
	}

    // see if we already have this signal for this column

	if (auto found_it = ranges::find_if(the_chart.GetSignals(), [number_cols, signal_type] (const PF_Signal& sig)
	    { return sig.column_number_ == number_cols - 1 && sig.signal_type_ == signal_type; });
	    found_it != the_chart.GetSignals().end())
	{
        return false;
	}

    return true;
}		// -----  end of method CanApplySignal  ----- 

std::optional<PF_Signal> PF_Catapult_Up::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_Catapult_Up_Buy, PF_Column::Direction::e_up, 4, PF_CanUse1BoxReversal::e_Yes))
	{
        return {};
	}

    // these patterns can be wide in 1-box reversal charts.  Set a leftmost boundary
    // by looking for any column that was higher than this one.

    int32_t boundary_column{-1};
	int32_t number_cols = the_chart.GetNumberOfColumns();

    // remember: column numbers count from zero.

    auto current_top = the_chart[number_cols - 1].GetTop();

    // fmt::print("col num: {} top: {}\n", number_cols - 1, current_top);

    for (int32_t index = number_cols - 2; index > -1; --index)
    {
       // fmt::print("index1: {}\n", index);
        if (the_chart[index].GetTop() >= current_top)
        {
            boundary_column = index;
            break;
        }
    }

   // fmt::print("boundary column: {}\n", boundary_column);

    // we finally get to apply our rule
    // first, we need to find the previous column in our direction with a top
    // below ours.

    int32_t which_prev_col{-1};
    auto previous_top = the_chart.GetBoxes().FindPrevBox(current_top);

    for (int32_t index = number_cols - 2; index > boundary_column; --index)
    {
       // fmt::print("index2: {}\n", index);
        if (the_chart[index].GetDirection() == PF_Column::Direction::e_up && the_chart[index].GetTop() == previous_top)
        {
            which_prev_col = index;
            break;
        }
    }

   // fmt::print("prev col: {} prev top: {}\n", which_prev_col, previous_top);

    if (which_prev_col > -1)
    {
        int32_t ctr{0};

        for (int32_t index = which_prev_col - 1; index > boundary_column; --index)
        {
//            fmt::print("index2: {}\n", index);
            if (the_chart[index].GetDirection() == PF_Column::Direction::e_up && the_chart[index].GetTop() == previous_top)
            {
                ++ctr;
            }
        }

        if (ctr > 0)
        {
            // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

            return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Buy,
                .signal_type_=PF_SignalType::e_Catapult_Up_Buy,
                .priority_=PF_SignalPriority::e_Catapult_Up_Buy,
                .tpt_=the_time,
                .column_number_=static_cast<int32_t>(number_cols - 1),
                .signal_price_=new_value,
                .box_=the_chart.GetBoxes().FindNextBox(previous_top)
            }};
        }
    }
	return {};
}		// -----  end of method PF_Catapult_Up::operator()  ----- 

std::optional<PF_Signal> PF_Catapult_Down::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_Catapult_Down_Sell, PF_Column::Direction::e_down, 4, PF_CanUse1BoxReversal::e_Yes))
	{
        return {};
	}

    // these patterns can be wide in 1-box reversal charts.  Set a leftmost boundary
    // by looking for any column that was higher than this one.

    int32_t boundary_column{-1};
	int32_t number_cols = the_chart.GetNumberOfColumns();

    // remember: column numbers count from zero.

    auto current_bottom = the_chart[number_cols - 1].GetBottom();

    // fmt::print("col num: {} top: {}\n", number_cols - 1, current_top);

    for (int32_t index = number_cols - 2; index > -1; --index)
    {
       // fmt::print("index1: {}\n", index);
        if (the_chart[index].GetBottom() <= current_bottom)
        {
            boundary_column = index;
            break;
        }
    }

   // fmt::print("boundary column: {}\n", boundary_column);

    // we finally get to apply our rule
    // first, we need to find the previous column in our direction with a bottom
    // above ours.

    int32_t which_prev_col{-1};
    auto previous_bottom = the_chart.GetBoxes().FindNextBox(current_bottom);

    for (int32_t index = number_cols - 2; index > boundary_column; --index)
    {
       // fmt::print("index2: {}\n", index);
        if (the_chart[index].GetDirection() == PF_Column::Direction::e_down && the_chart[index].GetBottom() == previous_bottom)
        {
            which_prev_col = index;
            break;
        }
    }

   // fmt::print("prev col: {} prev top: {}\n", which_prev_col, previous_top);

    if (which_prev_col > -1)
    {
        int32_t ctr{0};

        for (int32_t index = which_prev_col - 1; index > boundary_column; --index)
        {
//            fmt::print("index2: {}\n", index);
            if (the_chart[index].GetDirection() == PF_Column::Direction::e_down && the_chart[index].GetBottom() == previous_bottom)
            {
                ++ctr;
            }
        }

        if (ctr > 0)
        {
            // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

            return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Sell,
                .signal_type_=PF_SignalType::e_Catapult_Down_Sell,
                .priority_=PF_SignalPriority::e_Catapult_Down_Sell,
                .tpt_=the_time,
                .column_number_=static_cast<int32_t>(number_cols - 1),
                .signal_price_=new_value,
                .box_=the_chart.GetBoxes().FindPrevBox(previous_bottom)
            }};
        }
    }
	return {};
}		// -----  end of method PF_DoubleTopBuy::operator()  ----- 

std::optional<PF_Signal> PF_DoubleTopBuy::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_DoubleTop_Buy, PF_Column::Direction::e_up, 3, PF_CanUse1BoxReversal::e_No))
	{
        return {};
	}

	int32_t number_cols = the_chart.GetNumberOfColumns();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top = the_chart[number_cols - 3].GetTop();
    if (the_chart.GetCurrentColumn().GetTop() > previous_top)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Buy,
            .signal_type_=PF_SignalType::e_DoubleTop_Buy,
            .priority_=PF_SignalPriority::e_DoubleTop_Buy,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindNextBox(previous_top)
        }};
    }
	return {};
}		// -----  end of method PF_Catapult_Down::operator()  ----- 

std::optional<PF_Signal> PF_TripleTopBuy::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_TripleTop_Buy, PF_Column::Direction::e_up, 5, PF_CanUse1BoxReversal::e_No))
	{
        return {};
	}

	int32_t number_cols = the_chart.GetNumberOfColumns();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top_1 = the_chart[number_cols - 3].GetTop();
    auto previous_top_0 = the_chart[number_cols - 5].GetTop();
    if (the_chart.GetCurrentColumn().GetTop() > previous_top_1 &&  previous_top_0 == previous_top_1)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Buy,
            .signal_type_=PF_SignalType::e_TripleTop_Buy,
            .priority_=PF_SignalPriority::e_TripleTop_Buy,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindNextBox(previous_top_1)
        }};
    }
	return {};
}		// -----  end of method PF_TripleTopBuy::operator()  ----- 

std::optional<PF_Signal> PF_DoubleBottomSell::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_DoubleBottom_Sell, PF_Column::Direction::e_down, 3, PF_CanUse1BoxReversal::e_No))
	{
        return {};
	}

	int32_t number_cols = the_chart.GetNumberOfColumns();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_bottom = the_chart[number_cols - 3].GetBottom();
    if (the_chart.GetCurrentColumn().GetBottom() < previous_bottom)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Sell,
            .signal_type_=PF_SignalType::e_DoubleBottom_Sell,
            .priority_=PF_SignalPriority::e_DoubleBottom_Sell,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindPrevBox(previous_bottom)
        }};
    }
	return {};
}		// -----  end of method PF_DoubleBottomSell::operator()  ----- 

std::optional<PF_Signal> PF_TripleBottomSell::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_TripleBottom_Sell, PF_Column::Direction::e_down, 5, PF_CanUse1BoxReversal::e_No))
	{
        return {};
	}

	int32_t number_cols = the_chart.GetNumberOfColumns();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_bottom_1 = the_chart[number_cols - 3].GetBottom();
    auto previous_bottom_0 = the_chart[number_cols - 5].GetBottom();
    if (the_chart.GetCurrentColumn().GetBottom() < previous_bottom_1 && previous_bottom_0 == previous_bottom_1)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Sell,
            .signal_type_=PF_SignalType::e_TripleBottom_Sell,
            .priority_=PF_SignalPriority::e_TripleBottom_Sell,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindPrevBox(previous_bottom_1)
        }};
    }
	return {};
}		// -----  end of method PF_TripleBottomSell::operator()  ----- 

std::optional<PF_Signal> PF_Bullish_TT_Buy::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_Bullish_TT_Buy, PF_Column::Direction::e_up, 5, PF_CanUse1BoxReversal::e_No))
	{
        return {};
	}

	int32_t number_cols = the_chart.GetNumberOfColumns();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top_1 = the_chart[number_cols - 3].GetTop();
    auto previous_top_0 = the_chart[number_cols - 5].GetTop();
    if ((the_chart.GetCurrentColumn().GetTop() > previous_top_1) && (previous_top_1 > previous_top_0)
            && (the_chart.GetCurrentColumn().GetBottom() > the_chart[number_cols - 3].GetBottom()) && (the_chart[number_cols - 3].GetBottom() > the_chart[number_cols - 5].GetBottom()))
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Buy,
            .signal_type_=PF_SignalType::e_Bullish_TT_Buy,
            .priority_=PF_SignalPriority::e_Bullish_TT_Buy,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindNextBox(previous_top_1)
        }};
    }
	return {};
}		// -----  end of method PF_Bullish_TT_Buy::operator()  ----- 

std::optional<PF_Signal> PF_Bearish_TB_Sell::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (! CanApplySignal(the_chart, PF_SignalType::e_Bearish_TB_Sell, PF_Column::Direction::e_down, 5, PF_CanUse1BoxReversal::e_No))
	{
        return {};
	}

	int32_t number_cols = the_chart.GetNumberOfColumns();

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_bottom_1 = the_chart[number_cols - 3].GetBottom();
    auto previous_bottom_0 = the_chart[number_cols - 5].GetBottom();
    if ((the_chart.GetCurrentColumn().GetBottom() < previous_bottom_1) && (previous_bottom_1 < previous_bottom_0)
            && (the_chart.GetCurrentColumn().GetTop() < the_chart[number_cols - 3].GetTop()) && (the_chart[number_cols - 3].GetTop() < the_chart[number_cols - 5].GetTop()))
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Sell,
            .signal_type_=PF_SignalType::e_Bearish_TB_Sell,
            .priority_=PF_SignalPriority::e_Bearish_TB_Sell,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindPrevBox(previous_bottom_1)
        }};
    }
	return {};
}		// -----  end of method PF_Bearish_TB_Sell::operator()  ----- 



