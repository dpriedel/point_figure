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

#include <range/v3/algorithm/find_if.hpp>

#include "PF_Chart.h"
#include "PF_Signals.h"

Json::Value PF_SignalToJSON(const PF_Signal& signal)
{
    Json::Value result;
    switch(signal.signal_category_)
    {
        using enum PF_SignalCategory;
        case e_PF_Unknown:
            result["category"] = "unknown";
            break;

        case e_PF_Buy:
            result["category"] = "buy";
            break;

        case e_PF_Sell:
            result["category"] = "sell";
    };

    switch(signal.signal_type_)
    {
        using enum PF_SignalType;
        case e_PF_Unknown:
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
    };

    result["time"] = signal.tpt_.time_since_epoch().count();
    result["column"] = signal.column_number_;
    result["price"] = signal.signal_price_.ToStr();
    result["box"] = signal.box_.ToStr();

    return result;
}		// -----  end of method PF_SignalToJSON  ----- 

PF_Signal PF_SignalFromJSON(const Json::Value& new_data)
{
    PF_Signal new_sig;

    const auto category = new_data["category"].asString();
    if (category == "buy")
    {
        new_sig.signal_category_ = PF_SignalCategory::e_PF_Buy;
    }
    else if (category == "sell")
    {
        new_sig.signal_category_ = PF_SignalCategory::e_PF_Sell;
    }
    else if (category == "unknown")
    {
        new_sig.signal_category_ = PF_SignalCategory::e_PF_Unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid category provided: {}. Must be 'buy', 'sell', 'unknown'.", category)};
    }

    const auto type = new_data["type"].asString();
    if (type == "dt_buy")
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
    else if (type == "unknown")
    {
        new_sig.signal_type_ = PF_SignalType::e_PF_Unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid signal type provided: {}. Must be 'dt_buy', 'tt_buy' 'db_sell', 'tb_sell', 'unknown'.", type)};
    }

    new_sig.tpt_ = date::utc_time<date::utc_clock::duration>{date::utc_clock::duration{new_data["first_entry"].asInt64()}};
    new_sig.signal_price_ = DprDecimal::DDecQuad{new_data["price"].asString()};
    new_sig.box_ = DprDecimal::DDecQuad{new_data["box"].asString()};

    return new_sig;
}		// -----  end of method PF_SignalFromJSON  ----- 

 
std::optional<PF_Signal> PF_DoubleTopBuy::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (the_chart.GetCurrentColumn().GetDirection() != PF_Column::Direction::e_up)
	{
	    return {};
	}
	int32_t number_cols = the_chart.GetNumberOfColumns();
	if (number_cols < 3)
	{
	    // too few columns

	    return {};
	}

    // see if we already have this signal for this column

	if (auto found_it = ranges::find_if(the_chart.GetSignals(), [number_cols] (const PF_Signal& sig)
	    { return sig.column_number_ == number_cols - 1 && sig.signal_type_ == PF_SignalType::e_DoubleTop_Buy; });
	    found_it != the_chart.GetSignals().end())
	{
        return {};
	}

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top = the_chart[number_cols - 3].GetTop();
    if (the_chart.GetCurrentColumn().GetTop() > previous_top)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Buy,
            .signal_type_=PF_SignalType::e_DoubleTop_Buy,
            .tpt_=the_time,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindNextBox(previous_top)
        }};
    }
	return {};
}		// -----  end of method PF_DoubleTopBuy::operator()  ----- 

std::optional<PF_Signal> PF_TripleTopBuy::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (the_chart.GetCurrentColumn().GetDirection() != PF_Column::Direction::e_up)
	{
	    return {};
	}
	int32_t number_cols = the_chart.GetNumberOfColumns();
	if (number_cols < 5)
	{
	    // too few columns

	    return {};
	}

    // see if we already have this signal for this column

	if (auto found_it = ranges::find_if(the_chart.GetSignals(), [number_cols] (const PF_Signal& sig)
	    { return sig.column_number_ == number_cols - 1 && sig.signal_type_ == PF_SignalType::e_TripleTop_Buy; });
	    found_it != the_chart.GetSignals().end())
	{
        return {};
	}

    // we finally get to apply our rule
    // remember: column numbers count from zero.

    auto previous_top_1 = the_chart[number_cols - 3].GetTop();
    auto previous_top_0 = the_chart[number_cols - 5].GetTop();
    if (the_chart.GetCurrentColumn().GetTop() > previous_top_1 && the_chart.GetCurrentColumn().GetTop() > previous_top_0)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.

        return {PF_Signal{.signal_category_=PF_SignalCategory::e_PF_Buy,
            .signal_type_=PF_SignalType::e_TripleTop_Buy,
            .column_number_=static_cast<int32_t>(number_cols - 1),
            .signal_price_=new_value,
            .box_=the_chart.GetBoxes().FindNextBox(previous_top_1)
        }};
    }
	return {};
}		// -----  end of method PF_DoubleTopBuy::operator()  ----- 


