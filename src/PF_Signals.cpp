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
    return {};
}		// -----  end of method PF_SignalToJSON  ----- 

PF_Signal PF_SignalFromJSON(const Json::Value& json)
{
    return {};
}		// -----  end of method PF_SignalFromJSON  ----- 

 
std::optional<PF_Signal> PF_DoubleTopBuy::operator() (const PF_Chart& the_chart, const DprDecimal::DDecQuad& new_value, date::utc_time<date::utc_clock::duration> the_time)
{
	if (the_chart.GetCurrentDirection() != PF_Column::Direction::e_up)
	{
	    return {};
	}
	auto number_cols = the_chart.GetNumberOfColumns();
	if (number_cols < 3)
	{
	    // too few columns

	    return {};
	}
	if (auto found_it = ranges::find_if(the_chart.GetSignals(), [number_cols] (const PF_Signal& sig)
	            { return sig.column_number_ == number_cols && sig.signal_type_ == PF_SignalType::e_PF_Buy; }); found_it != the_chart.GetSignals().end())
	{
        // see if we already have this signal for this column

        return {};
	}

    // we finally get to apply our rule

    auto previous_top = the_chart[number_cols -2].GetTop();
    if (the_chart[number_cols].GetTop() > previous_top)
    {
        // price could jump several boxes but we want to set the signal at the next box higher than the last column top.
        return {PF_Signal{.signal_type_=PF_SignalType::e_PF_Buy, .tpt_=the_time, .column_number_=number_cols, .signal_price_=new_value, .box_=the_chart.GetBoxes().FindNextBox(previous_top)}};
    }
	return {};
}		// -----  end of method PF_DoubleTopBuy::operator()  ----- 


