// =====================================================================================
// 
//       Filename:  PF_Chart.cpp
// 
//    Description:  Implementation of class which contains Point & Figure data for a symbol.
// 
//        Version:  2.0
//        Created:  2021-07-29 08:47 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================


	/* This file is part of PF_CollectData. */

	/* PF_CollectData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* PF_CollectData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with PF_CollectData.  If not, see <http://www.gnu.org/licenses/>. */

//#include <iterator>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <date/date.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <utility>

#include<date/tz.h>

//#include <chartdir.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/algorithm/max_element.hpp>
#include <range/v3/view/chunk_by.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/filter.hpp>


#include <pybind11/embed.h> // everything needed for embedding
#include <pybind11/gil.h>
#include <pybind11/stl.h>

#include <spdlog/spdlog.h>

namespace py = pybind11;
using namespace py::literals;
using namespace std::string_literals;

#include "PF_Signals.h"
#include "DDecQuad.h"
#include "PF_Chart.h"
#include "PF_Column.h"
#include "utilities.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart (const PF_Chart& rhs)
    : boxes_{rhs.boxes_}, signals_{rhs.signals_}, columns_{rhs.columns_},
    current_column_{rhs.current_column_}, symbol_{rhs.symbol_}, chart_base_name_{rhs.chart_base_name_},
    fname_box_size_{rhs.fname_box_size_}, atr_{rhs.atr_},
    first_date_{rhs.first_date_}, last_change_date_{rhs.last_change_date_}, last_checked_date_{rhs.last_checked_date_},
    y_min_{rhs.y_min_}, y_max_{rhs.y_max_},
    current_direction_{rhs.current_direction_}, max_columns_for_graph_{rhs.max_columns_for_graph_}

{
    // now, the reason for doing this explicitly is to fix the column box pointers.

    ranges::for_each(columns_, [this] (auto& col) { col.boxes_ = &this->boxes_; });
    current_column_.boxes_ = &boxes_;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart (PF_Chart&& rhs) noexcept
    : boxes_{std::move(rhs.boxes_)}, signals_{std::move(rhs.signals_)},
    columns_{std::move(rhs.columns_)}, current_column_{std::move(rhs.current_column_)},
    symbol_{std::move(rhs.symbol_)}, chart_base_name_{std::move(rhs.chart_base_name_)},
    fname_box_size_{std::move(rhs.fname_box_size_)}, atr_{std::move(rhs.atr_)},
    first_date_{rhs.first_date_}, last_change_date_{rhs.last_change_date_},
    last_checked_date_{rhs.last_checked_date_}, 
    y_min_{std::move(rhs.y_min_)}, y_max_{std::move(rhs.y_max_)},
    current_direction_{rhs.current_direction_}, max_columns_for_graph_{rhs.max_columns_for_graph_} 

{
    // now, the reason for doing this explicitly is to fix the column box pointers.

    ranges::for_each(columns_, [this] (auto& col) { col.boxes_ = &this->boxes_; });
    current_column_.boxes_ = &boxes_;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart (std::string symbol, DprDecimal::DDecQuad box_size, int32_t reversal_boxes,
        Boxes::BoxScale box_scale, DprDecimal::DDecQuad atr, int64_t max_columns_for_graph)
    : symbol_{std::move(symbol)}, fname_box_size_{std::move(box_size)}, atr_{std::move(atr)}, max_columns_for_graph_{max_columns_for_graph}

{
	// stock prices are listed to 2 decimals.  If we are doing integral scale, then
	// we limit box size to that.
	
	DprDecimal::DDecQuad runtime_box_size = fname_box_size_;
    if (atr_ != 0.0)
    {
    	runtime_box_size = (fname_box_size_ * atr_).Rescale(std::min(fname_box_size_.GetExponent(), atr_.GetExponent()) - 1);

    	// it seems that the rescaled box size value can turn out to be zero. If that 
    	// is the case, then go with the unscaled box size. 

    	if (runtime_box_size == 0.0)
    	{
    		runtime_box_size = fname_box_size_ * atr_;
    	}

    	if (box_scale == Boxes::BoxScale::e_linear)
    	{
    		if (runtime_box_size.GetExponent() < -2)
    		{
    			runtime_box_size.Rescale(-2);
    		}
    	}
    }
	boxes_ = Boxes{runtime_box_size, box_scale};
	current_column_ = PF_Column(&boxes_, columns_.size(), reversal_boxes); 

	chart_base_name_ = MakeChartBaseName();

}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Chart::PF_Chart (const Json::Value& new_data)
{
    if (! new_data.empty())
    {
        this->FromJSON(new_data);
    }
    else
    {
        spdlog::debug("Trying to construct PF_Chart from empty JSON value.");
    }
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  ----- 

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Chart PF_Chart::MakeChartFromDB(const PF_DB& chart_db, PF_ChartParams vals, std::string_view interval)
{
    Json::Value chart_data = chart_db.GetPFChartData(MakeChartNameFromParams(vals, interval, "json"));
    PF_Chart chart_from_db{chart_data};
	return chart_from_db;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  ----- 

PF_Chart& PF_Chart::operator= (const PF_Chart& rhs)
{
    if (this != &rhs)
    {
        boxes_ = rhs.boxes_;
        signals_ = rhs.signals_;
        columns_ = rhs.columns_;
        current_column_ = rhs.current_column_;
        symbol_ = rhs.symbol_;
        chart_base_name_ = rhs.chart_base_name_;
        fname_box_size_ = rhs.fname_box_size_;
        atr_ = rhs.atr_;
        first_date_ = rhs.first_date_;
        last_change_date_ = rhs.last_change_date_;
        last_checked_date_ = rhs.last_checked_date_;
        y_min_ = rhs.y_min_;
        y_max_ = rhs.y_max_;
        current_direction_ = rhs.current_direction_;
        max_columns_for_graph_ = rhs.max_columns_for_graph_;

        // now, the reason for doing this explicitly is to fix the column box pointers.

        ranges::for_each(columns_, [this] (auto& col) { col.boxes_ = &this->boxes_; });
        current_column_.boxes_ = &boxes_;
    }
    return *this;
}		// -----  end of method PF_Chart::operator=  ----- 

PF_Chart& PF_Chart::operator= (PF_Chart&& rhs) noexcept
{
    if (this != &rhs)
    {
        boxes_ = std::move(rhs.boxes_);
        signals_ = std::move(rhs.signals_);
        columns_ = std::move(rhs.columns_);
        current_column_ = std::move(rhs.current_column_);
        symbol_ = rhs.symbol_;
        chart_base_name_ = rhs.chart_base_name_;
        fname_box_size_ = rhs.fname_box_size_;
        atr_ = rhs.atr_;
        first_date_ = rhs.first_date_;
        last_change_date_ = rhs.last_change_date_;
        last_checked_date_ = rhs.last_checked_date_;
        y_min_ = std::move(rhs.y_min_);
        y_max_ = std::move(rhs.y_max_);
        current_direction_ = rhs.current_direction_;
        max_columns_for_graph_ = rhs.max_columns_for_graph_;

        // now, the reason for doing this explicitly is to fix the column box pointers.

        ranges::for_each(columns_, [this] (auto& col) { col.boxes_ = &this->boxes_; });
        current_column_.boxes_ = &boxes_;
    }
    return *this;
}		// -----  end of method PF_Chart::operator=  ----- 

PF_Chart& PF_Chart::operator= (const Json::Value& new_data)
{
    this->FromJSON(new_data);
    return *this;
}		// -----  end of method PF_Chart::operator=  ----- 

bool PF_Chart::operator== (const PF_Chart& rhs) const
{
    if (symbol_ != rhs.symbol_)
    {
        return false;
    }
    if (GetChartBoxSize() != rhs.GetChartBoxSize())
    {
        return false;
    }
    if (GetReversalboxes() != rhs.GetReversalboxes())
    {
        return false;
    }
    if (y_min_ != rhs.y_min_)
    {
        return false;
    }
    if (y_max_ != rhs.y_max_)
    {
        return false;
    }
    if (current_direction_ != rhs.current_direction_)
    {
        return false;
    }
    if (GetBoxType() != rhs.GetBoxType())
    {
        return false;
    }

    if (GetBoxScale() != rhs.GetBoxScale())
    {
        return false;
    }

    // if we got here, then we can look at our data 

    if (columns_ != rhs.columns_)
    {
        return false;
    }
    if (current_column_ != rhs.current_column_)
    {
        return false;
    }

    return true;
}		// -----  end of method PF_Chart::operator==  ----- 

PF_Column::Status PF_Chart::AddValue(const DprDecimal::DDecQuad& new_value, PF_Column::TmPt the_time)
{
    // when extending the chart, don't add 'old' data.

    if (! empty())
    {
    	if (the_time <= last_checked_date_)
    	{
    		return PF_Column::Status::e_ignored;
    	}
    }
    else
    {
		first_date_ = the_time;
    }

    auto [status, new_col] = current_column_.AddValue(new_value, the_time);

    if (status == PF_Column::Status::e_accepted)
    {
        if (current_column_.GetTop() > y_max_)
        {
            y_max_ = current_column_.GetTop();
        }
        if (current_column_.GetBottom() < y_min_)
        {
            y_min_ = current_column_.GetBottom();
        }
        last_change_date_ = the_time;

        // if (auto had_signal = PF_Chart::LookForSignals(*this, new_value, the_time); had_signal)
        if (auto had_signal = AddSignalsToChart(*this, new_value, the_time); had_signal)
        {

        	status = PF_Column::Status::e_accepted_with_signal;
        }
    }
    else if (status == PF_Column::Status::e_reversal)
    {
        columns_.push_back(current_column_);
        current_column_ = std::move(new_col.value());

        // now continue on processing the value.
        
        status = current_column_.AddValue(new_value, the_time).first;
        last_change_date_ = the_time;

        // if (auto had_signal = PF_Chart::LookForSignals(*this, new_value, the_time); had_signal)
        if (auto had_signal = AddSignalsToChart(*this, new_value, the_time); had_signal)
        {
        	status = PF_Column::Status::e_accepted_with_signal;
        }
    }
    current_direction_ = current_column_.GetDirection();
    last_checked_date_ = the_time;
    return status;
}		// -----  end of method PF_Chart::AddValue  ----- 

void PF_Chart::LoadData (std::istream* input_data, std::string_view date_format, char delim)
{
    std::string buffer;
    while ( ! input_data->eof())
    {
        buffer.clear();
        std::getline(*input_data, buffer);
        if (input_data->fail())
        {
            continue;
        }
        auto fields = split_string<std::string_view>(buffer, delim);

        AddValue(DprDecimal::DDecQuad{std::string{fields[1]}}, StringToUTCTimePoint(date_format, fields[0]));
    }

    // make sure we keep the last column we were working on 

    if (current_column_.GetTop() > y_max_)
    {
        y_max_ = current_column_.GetTop();
    }
    if (current_column_.GetBottom() < y_min_)
    {
        y_min_ = current_column_.GetBottom();
    }
    current_direction_ = current_column_.GetDirection();
}

std::string PF_Chart::MakeChartBaseName() const
{
    std::string chart_name = fmt::format("{}_{}{}X{}_{}",
            symbol_,
            fname_box_size_,
            (IsPercent() ? "%" : ""),
            GetReversalboxes(),
            (IsPercent() ? "percent" : "linear"));
    return chart_name;
}		// -----  end of method PF_Chart::ChartName  ----- 

std::string PF_Chart::MakeChartFileName (std::string_view interval, std::string_view suffix) const
{
    std::string chart_name = fmt::format("{}{}.{}",
            chart_base_name_,
            (! interval.empty() ? "_"s += interval : ""),
            suffix);
    return chart_name;
}		// -----  end of method PF_Chart::ChartName  ----- 

void PF_Chart::ConstructChartGraphAndWriteToFile (const fs::path& output_filename, const streamed_prices& streamed_prices, const std::string& show_trend_lines, X_AxisFormat date_or_time) const
{
	BOOST_ASSERT_MSG(! empty(), fmt::format("Chart for symbol: {} contains no data. Unable to draw graphic.", symbol_).c_str());

    std::vector<double> highData;
    std::vector<double> lowData;
    std::vector<double> openData;
    std::vector<double> closeData;
    std::vector<bool> had_step_back;
    std::vector<bool> direction_is_up;

    // for x-axis label, we use the begin date for each column 
    // the chart software wants an array of const char*.
    // this will take several steps.

    std::vector<std::string> x_axis_labels;

	// limit the number of columns shown on graphic if requested 
	
	size_t skipped_columns = max_columns_for_graph_ < 1 || size() <= max_columns_for_graph_ ? 0 : size() - max_columns_for_graph_; 

    // we want to mark the openning value on the chart so change can be seen when drawing only most recent columns. 
    
    const auto& first_col = this->operator[](0);
    double openning_price = first_col.GetDirection() == PF_Column::Direction::e_up ? first_col.GetBottom().ToDouble() : first_col.GetTop().ToDouble();

    for (const auto& col : columns_ | ranges::views::drop(skipped_columns))
    {
        lowData.push_back(col.GetBottom().ToDouble());
        highData.push_back(col.GetTop().ToDouble());

        if (col.GetDirection() == PF_Column::Direction::e_up)
        {
            openData.push_back(col.GetBottom().ToDouble());
            closeData.push_back(col.GetTop().ToDouble());
            direction_is_up.push_back(true);
        }
        else
        {
            openData.push_back(col.GetTop().ToDouble());
            closeData.push_back(col.GetBottom().ToDouble());
            direction_is_up.push_back(false);
        }
		if (date_or_time == X_AxisFormat::e_show_date)
        {
	        x_axis_labels.push_back(fmt::format("{:%F}", col.GetTimeSpan().first));
        }
		else
        {
            x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first));
        }
        had_step_back.push_back(col.GetHadReversal());
    }

    lowData.push_back(current_column_.GetBottom().ToDouble());
    highData.push_back(current_column_.GetTop().ToDouble());

    if (current_column_.GetDirection() == PF_Column::Direction::e_up)
    {
        openData.push_back(current_column_.GetBottom().ToDouble());
        closeData.push_back(current_column_.GetTop().ToDouble());
        direction_is_up.push_back(true);
    }
    else
    {
        openData.push_back(current_column_.GetTop().ToDouble());
        closeData.push_back(current_column_.GetBottom().ToDouble());
        direction_is_up.push_back(false);
    }
    if (date_or_time == X_AxisFormat::e_show_date)
    {
        x_axis_labels.push_back(fmt::format("{:%F}", current_column_.GetTimeSpan().first));
    }
    else
    {
        x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(current_column_.GetTimeSpan().first));
    }

    had_step_back.push_back(current_column_.GetHadReversal());

	// extract and format the Signals, if any, from the chart. The drawing code can plot
	// them or not.
	// NOTE: we need to zap the column number with the skipped columns offset 

	// let's try to do lower level setup for signals drawing here.
    // we want 1 signal per column and that should be the signal with highest priority
    // data is formated as needed by mplfinance library to build lists of marks to overlay
    // on graphic. this saves a bunch of python code

	const auto& sngls = GetSignals();
    std::vector<double> dt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> db_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> btt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> btb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tt_cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tb_cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());

    int had_dt_buy = 0;
    int had_tt_buy = 0;
    int had_db_sell = 0;
    int had_tb_sell = 0;
    int had_bullish_tt_buy = 0;
    int had_bearish_tb_sell = 0;
    int had_catapult_buy = 0;
    int had_catapult_sell = 0;
    int had_tt_catapult_buy = 0;
    int had_tb_catapult_sell = 0;

    for (const auto& sigs : sngls
            | ranges::views::filter([skipped_columns] (const auto& s) { return s.column_number_ >= skipped_columns; })
            | ranges::views::chunk_by([](const auto& a, const auto& b) { return a.column_number_ == b.column_number_; }))
    {
        const auto most_important = ranges::max_element(sigs, {}, [](const auto& s) { return std::to_underlying(s.priority_); }) ;
        switch (most_important->signal_type_)
        {
            using enum PF_SignalType;
            case e_Unknown:
                break;
            case e_DoubleTop_Buy:
                dt_buys[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_dt_buy += 1;
                break;
            case e_DoubleBottom_Sell:
                db_sells[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_db_sell += 1;
                break;
            case e_TripleTop_Buy:
                tt_buys[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_tt_buy += 1;
                break;
            case e_TripleBottom_Sell:
                tb_sells[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_tb_sell += 1;
                break;
            case e_Bullish_TT_Buy:
                btt_buys[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_bullish_tt_buy += 1;
                break;
            case e_Bearish_TB_Sell:
                btb_sells[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_bearish_tb_sell += 1;
                break;
            case e_Catapult_Buy:
                cat_buys[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_catapult_buy += 1;
                break;
            case e_Catapult_Sell:
                cat_sells[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_catapult_sell += 1;
                break;
            case e_TTop_Catapult_Buy:
                tt_cat_buys[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_tt_catapult_buy += 1;
                break;
            case e_TBottom_Catapult_Sell:
                tb_cat_sells[most_important->column_number_ - skipped_columns] = most_important->box_.ToDouble();
                had_tb_catapult_sell += 1;
                break;
        }
    }

	// want to show approximate overall change in value (computed from boxes, not actual prices)
	
	DprDecimal::DDecQuad first_value = 0;
	DprDecimal::DDecQuad last_value = 0;

	if (size() > 1)
	{
		auto first_col = columns_[0];
		first_value = first_col.GetDirection() == PF_Column::Direction::e_up ? first_col.GetBottom() : first_col.GetTop();
		// apparently, this can happen 

		if (first_value == 0.0)
		{
			first_value = 0.01;
		}
	}
	else
	{
		first_value = current_column_.GetDirection() == PF_Column::Direction::e_up ? current_column_.GetBottom() : current_column_.GetTop();
	}
	last_value = current_column_.GetDirection() == PF_Column::Direction::e_up ? current_column_.GetTop() : current_column_.GetBottom();

	DprDecimal::DDecQuad overall_pct_chg = ((last_value - first_value) / first_value * 100).Rescale(-2);

	std::string skipped_columns_text;
	if (skipped_columns > 0)
	{
		skipped_columns_text = fmt::format(" (last {} cols)", max_columns_for_graph_);
	}
    // some explanation for custom box colors. 

    std::string explanation_text;
    if (GetReversalboxes() == 1)
    {
        explanation_text = "Orange: 1-step Up then reversal Down. Green: 1-step Down then reversal Up.";
    }
    auto chart_title = fmt::format("\n{}{} X {} for {} {}. Overall % change: {}{}\nLast change: {:%a, %b %d, %Y at %I:%M:%S %p %Z}\n{}", GetChartBoxSize(),
                (IsPercent() ? "%" : ""), GetReversalboxes(), symbol_,
                (IsPercent() ? "percent" : ""), overall_pct_chg, skipped_columns_text, date::clock_cast<std::chrono::system_clock>(last_change_date_), explanation_text);

    py::dict locals = py::dict{
        "the_data"_a = py::dict{
            "Date"_a = x_axis_labels,
            "Open"_a = openData,
            "High"_a = highData,
            "Low"_a = lowData,
            "Close"_a = closeData
        },
        "ReversalBoxes"_a = GetReversalboxes(),
        "IsUp"_a = direction_is_up,
        "StepBack"_a = had_step_back,
        "ChartTitle"_a = chart_title,
        "ChartFileName"_a = output_filename.string(),
        "DateTimeFormat"_a = date_or_time == X_AxisFormat::e_show_date ? "%Y-%m-%d" : "%H:%M:%S",
        "Y_min"_a = GetYLimits().first.ToDouble(),
        "Y_max"_a = GetYLimits().second.ToDouble(),
        "openning_price"_a = openning_price,
        "UseLogScale"_a = IsPercent(),
        "ShowTrendLines"_a = show_trend_lines,
        "the_signals"_a = py::dict{
            "dt_buys"_a = had_dt_buy ? dt_buys : std::vector<double>{},
            "db_sells"_a = had_db_sell ? db_sells : std::vector<double>{},
            "tt_buys"_a = had_tt_buy ? tt_buys : std::vector<double>{},
            "tb_sells"_a = had_tb_sell ? tb_sells : std::vector<double>{},
            "bullish_tt_buys"_a = had_bullish_tt_buy ? btt_buys : std::vector<double>{},
            "bearish_tb_sells"_a = had_bearish_tb_sell ? btb_sells : std::vector<double>{},
            "catapult_buys"_a = had_catapult_buy ? cat_buys : std::vector<double>{},
            "catapult_sells"_a = had_catapult_sell ? cat_sells : std::vector<double>{},
            "tt_catapult_buys"_a = had_tt_catapult_buy ? tt_cat_buys : std::vector<double>{},
            "tb_catapult_sells"_a = had_tb_catapult_sell ? tb_cat_sells : std::vector<double>{}
        },
        "streamed_prices"_a = py::dict{
            "the_time"_a = streamed_prices.timestamp_,
            "price"_a = streamed_prices.price_,
            "signal_type"_a = streamed_prices.signal_type_
        }
    };

        // Execute Python code, using the variables saved in `locals`

//        py::gil_scoped_acquire gil{};
        py::exec(R"(
        PF_DrawChart.DrawChart(the_data, ReversalBoxes, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat,
        ShowTrendLines, UseLogScale, Y_min, Y_max, openning_price, the_signals, streamed_prices)
        )", py::globals(), locals);
}		// -----  end of method PF_Chart::ConstructChartAndWriteToFile  ----- 

void PF_Chart::ConvertChartToJsonAndWriteToFile (const fs::path& output_filename) const
{
	std::ofstream out{output_filename, std::ios::out | std::ios::binary};
	BOOST_ASSERT_MSG(out.is_open(), fmt::format("Unable to open file: {} for chart output.", output_filename).c_str());
	ConvertChartToJsonAndWriteToStream(out);
	out.close();
}		// -----  end of method PF_Chart::ConvertChartToJsonAndWriteToFile  ----- 

void PF_Chart::ConvertChartToJsonAndWriteToStream (std::ostream& stream) const
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";        // compact printing and string formatting 
    std::unique_ptr<Json::StreamWriter> const writer( builder.newStreamWriter());
    writer->write(this->ToJSON(), &stream);
    stream << std::endl;  // add lf and flush
}		// -----  end of method PF_Chart::ConvertChartToJsonAndWriteToStream  ----- 

void PF_Chart::ConvertChartToTableAndWriteToFile (const fs::path& output_filename, X_AxisFormat date_or_time) const
{
	std::ofstream out{output_filename, std::ios::out | std::ios::binary};
	BOOST_ASSERT_MSG(out.is_open(), fmt::format("Unable to open file: {} for graphics data output.", output_filename).c_str());
	ConvertChartToTableAndWriteToStream(out);
	out.close();
}		// -----  end of method PF_Chart::ConvertChartToTableAndWriteToFile  ----- 

void PF_Chart::ConvertChartToTableAndWriteToStream (std::ostream& stream, X_AxisFormat date_or_time) const
{
	// generate a delimited 'csv' file for use by external programs
	// format is: date, open, low, high, close, color, color index
	// where 'color' means column direction:
	//  up ==> green
	//  down ==> red
	//  reverse up ==> blue
	//  reverse down ==> orange
	//  color index can be used with custom palettes such as in gnuplot
	//
	
	size_t skipped_columns = max_columns_for_graph_ < 1 || size() <= max_columns_for_graph_ ? 0 : size() - max_columns_for_graph_; 

	constexpr auto row_template = "{}\t{}\t{}\t{}\t{}\t{}\n";

	auto compute_color = [](const PF_Column& c)
	{
		if (c.GetDirection() == PF_Column::Direction::e_up)
		{
			return (c.GetHadReversal() ? "blue\t3" : "green\t1");
		}
		if (c.GetDirection() == PF_Column::Direction::e_down)
		{
			return (c.GetHadReversal() ? "orange\t2" : "red\t0");
		}
		return "black\t4";
	};

	// we'll provide a header record 
	
	std::string header_record{"date\topen\tlow\thigh\tclose\tcolor\tindex\n"};
	stream.write(header_record.data(), header_record.size());

    for (const auto& col : columns_ | ranges::views::drop(skipped_columns))
    {
        auto next_row = fmt::format(row_template,
				date_or_time == X_AxisFormat::e_show_date ? fmt::format("{:%F}", col.GetTimeSpan().first) : UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first),
				col.GetDirection() == PF_Column::Direction::e_up ? col.GetBottom().ToStr() : col.GetTop().ToStr(),
				col.GetBottom().ToStr(),
				col.GetTop().ToStr(),
				col.GetDirection() == PF_Column::Direction::e_up ? col.GetTop().ToStr() : col.GetBottom().ToStr(),
				compute_color(col)
        		);
		stream.write(next_row.data(), next_row.size());
    }

    auto last_row = fmt::format(row_template,
			date_or_time == X_AxisFormat::e_show_date ? fmt::format("{:%F}", current_column_.GetTimeSpan().first) : UTCTimePointToLocalTZHMSString(current_column_.GetTimeSpan().first),
			current_column_.GetDirection() == PF_Column::Direction::e_up ? current_column_.GetBottom().ToStr() : current_column_.GetTop().ToStr(),
			current_column_.GetBottom().ToStr(),
			current_column_.GetTop().ToStr(),
			current_column_.GetDirection() == PF_Column::Direction::e_up ? current_column_.GetTop().ToStr() : current_column_.GetBottom().ToStr(),
			compute_color(current_column_)
        	);
	stream.write(last_row.data(), last_row.size());
}		// -----  end of method PF_Chart::ConvertChartToTableAndWriteToStream  ----- 

void PF_Chart::StoreChartInChartsDB(const PF_DB& chart_db, std::string_view interval, X_AxisFormat date_or_time, bool store_cvs_graphics) const
{
	std::string cvs_graphics;
	if (store_cvs_graphics)
	{
		std::ostringstream oss{};
		ConvertChartToTableAndWriteToStream(oss, date_or_time);
		cvs_graphics = oss.str();
	}
    chart_db.StorePFChartDataIntoDB(*this, interval, cvs_graphics);
}		// -----  end of method PF_Chart::StoreChartInChartsDB  ----- 

void PF_Chart::UpdateChartInChartsDB(const PF_DB& chart_db, std::string_view interval, X_AxisFormat date_or_time, bool store_cvs_graphics) const
{
	std::string cvs_graphics;
	if (store_cvs_graphics)
	{
		std::ostringstream oss{};
		ConvertChartToTableAndWriteToStream(oss, date_or_time);
		cvs_graphics = oss.str();
	}
    chart_db.UpdatePFChartDataInDB(*this, interval, cvs_graphics);
}		// -----  end of method PF_Chart::StoreChartInChartsDB  ----- 


Json::Value PF_Chart::ToJSON () const
{
    Json::Value result;
    result["symbol"] = symbol_;
    result["base_name"] = chart_base_name_;
    result["boxes"] = boxes_.ToJSON();

    Json::Value signals{Json::arrayValue};
    for (const auto& sig : signals_)
    {
        signals.append(PF_SignalToJSON(sig));
    }
    result["signals"] = signals;

    result["first_date"] = first_date_.time_since_epoch().count();
    result["last_change_date"] = last_change_date_.time_since_epoch().count();
    result["last_check_date"] = last_checked_date_.time_since_epoch().count();

    result["fname_box_size"] = fname_box_size_.ToStr();
    result["atr"] = atr_.ToStr();
    result["y_min"] = y_min_.ToStr();
    result["y_max"] = y_max_.ToStr();

    switch(current_direction_)
    {
        using enum PF_Column::Direction;
        case e_unknown:
            result["current_direction"] = "unknown";
            break;

        case e_down:
            result["current_direction"] = "down";
            break;

        case e_up:
            result["current_direction"] = "up";
            break;
    };
    result["max_columns"] = max_columns_for_graph_;

    Json::Value cols{Json::arrayValue};
    for (const auto& col : columns_)
    {
        cols.append(col.ToJSON());
    }
    result["columns"] = cols;
    result["current_column"] = current_column_.ToJSON();

    return result;

}		// -----  end of method PF_Chart::ToJSON  ----- 


void PF_Chart::FromJSON (const Json::Value& new_data)
{
    symbol_ = new_data["symbol"].asString();
    chart_base_name_ = new_data["base_name"].asString();
    boxes_ = new_data["boxes"];

    const auto& signals = new_data["signals"];
    signals_.clear();
    ranges::for_each(signals, [this](const auto& next_val) { this->signals_.push_back(PF_SignalFromJSON(next_val)); });

    first_date_ = PF_Column::TmPt{std::chrono::nanoseconds{new_data["first_date"].asInt64()}};
    last_change_date_ = PF_Column::TmPt{std::chrono::nanoseconds{new_data["last_change_date"].asInt64()}};
    last_checked_date_ = PF_Column::TmPt{std::chrono::nanoseconds{new_data["last_check_date"].asInt64()}};

    fname_box_size_ = DprDecimal::DDecQuad{new_data["fname_box_size"].asString()};
    atr_ = DprDecimal::DDecQuad{new_data["atr"].asString()};
    y_min_ = DprDecimal::DDecQuad{new_data["y_min"].asString()};
    y_max_ = DprDecimal::DDecQuad{new_data["y_max"].asString()};

    const auto direction = new_data["current_direction"].asString();
    if (direction == "up")
    {
        current_direction_ = PF_Column::Direction::e_up;
    }
    else if (direction == "down")
    {
        current_direction_ = PF_Column::Direction::e_down;
    }
    else if (direction == "unknown")
    {
        current_direction_ = PF_Column::Direction::e_unknown;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid direction provided: {}. Must be 'up', 'down', 'unknown'.", direction)};
    }

	max_columns_for_graph_ = new_data["max_columns"].asInt64();

    // lastly, we can do our columns 
    // need to hook them up with current boxes_ data

    const auto& cols = new_data["columns"];
    columns_.clear();
    ranges::for_each(cols, [this](const auto& next_val) { this->columns_.emplace_back(&boxes_, next_val); });

    current_column_ = PF_Column{&boxes_, new_data["current_column"]};
}		// -----  end of method PF_Chart::FromJSON  ----- 


    // ===  FUNCTION  ======================================================================
    //         Name:  ComputeATR
    //  Description:  Expects the input data is in descending order by date
    // =====================================================================================

DprDecimal::DDecQuad ComputeATR(std::string_view symbol, const std::vector<StockDataRecord>& the_data, int32_t how_many_days)
{
    BOOST_ASSERT_MSG(the_data.size() > how_many_days, fmt::format("Not enough data provided for: {}. Need at least: {} values. Got {}.", symbol, how_many_days, the_data.size()).c_str());

    DprDecimal::DDecQuad total{0};

    DprDecimal::DDecQuad high_minus_low;
    DprDecimal::DDecQuad high_minus_prev_close;
    DprDecimal::DDecQuad low_minus_prev_close;

    for (int32_t i = 0; i < how_many_days; ++i)
    {
        high_minus_low = the_data[i].high_ - the_data[i].low_;
        high_minus_prev_close = (the_data[i].high_ - the_data[i + 1].close_).abs();
        low_minus_prev_close = (the_data[i].low_ - the_data[i + 1].close_).abs();

        DprDecimal::DDecQuad max = DprDecimal::max(high_minus_low, DprDecimal::max(high_minus_prev_close, low_minus_prev_close));
        
		// fmt::print("i: {} hml: {} hmpc: {} lmpc: {} max: {}\n", i, high_minus_low, high_minus_prev_close, low_minus_prev_close, max);
        total += max;
    }

//    std::cout << "total: " << total << '\n';
    return total /= how_many_days;
}		// -----  end of function ComputeATRUsingJSON  -----

std::string MakeChartNameFromParams (const PF_Chart::PF_ChartParams& vals, std::string_view interval, std::string_view suffix)
{
    std::string chart_name = fmt::format("{}_{}{}X{}_{}{}.{}",
            std::get<PF_Chart::e_symbol>(vals),
            std::get<PF_Chart::e_box_size>(vals),
            (std::get<PF_Chart::e_box_scale>(vals) == Boxes::BoxScale::e_percent ? "%" : ""),
            std::get<PF_Chart::e_reversal>(vals),
            (std::get<PF_Chart::e_box_scale>(vals) == Boxes::BoxScale::e_linear ? "linear" : "percent"),
            (! interval.empty() ? "_"s += interval : ""),
            suffix);
    return chart_name;
}		// -----  end of method MakeChartNameFromParams  ----- 

PF_Chart::iterator PF_Chart::begin()
{
    return iterator{this};
}		// -----  end of method PF_Chart::begin  ----- 

PF_Chart::const_iterator PF_Chart::begin() const
{
    return const_iterator{this};
}		// -----  end of method PF_Chart::begin  ----- 

PF_Chart::iterator PF_Chart::end()
{
    return iterator(this, this->size());
}		// -----  end of method PF_Chart::end  ----- 

PF_Chart::const_iterator PF_Chart::end() const
{
    return const_iterator(this, this->size());
}		// -----  end of method PF_Chart::end  ----- 

bool PF_Chart::PF_Chart_Iterator::operator==(const PF_Chart_Iterator& rhs) const
{
    return (chart_ == rhs.chart_ && index_ == rhs.index_);
}		// -----  end of method PF_Chart::PF_Chart_Iterator::operator==  ----- 

PF_Chart::PF_Chart_Iterator& PF_Chart::PF_Chart_Iterator::operator++()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(++index_, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_Iterator::operator++  ----- 

PF_Chart::PF_Chart_Iterator& PF_Chart::PF_Chart_Iterator::operator+=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(index_+=n, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_Iterator::operator+=  ----- 

PF_Chart::PF_Chart_Iterator& PF_Chart::PF_Chart_Iterator::operator--()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_less(--index_, 0))
    {
        index_ = 0;
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_Iterator::operator--  ----- 

PF_Chart::PF_Chart_Iterator& PF_Chart::PF_Chart_Iterator::operator-=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_less(index_-=n, 0))
    {
        index_ = 0;
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_Iterator::operator-+  ----- 

PF_Chart::reverse_iterator PF_Chart::rbegin()
{
    return reverse_iterator{this, static_cast<int32_t>(this->size() -1)};
}		// -----  end of method PF_Chart::rbegin  ----- 

PF_Chart::const_reverse_iterator PF_Chart::rbegin() const
{
    return const_reverse_iterator{this, static_cast<int32_t>(this->size() -1)};
}		// -----  end of method PF_Chart::rbegin  ----- 

PF_Chart::reverse_iterator PF_Chart::rend()
{
    return reverse_iterator(this, -1);
}		// -----  end of method PF_Chart::rend  ----- 

PF_Chart::const_reverse_iterator PF_Chart::rend() const
{
    return const_reverse_iterator(this, -1);
}		// -----  end of method PF_Chart::rend  ----- 

bool PF_Chart::PF_Chart_ReverseIterator::operator==(const PF_Chart_ReverseIterator& rhs) const
{
    return (chart_ == rhs.chart_ && index_ == rhs.index_);
}		// -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator==  ----- 

PF_Chart::PF_Chart_ReverseIterator& PF_Chart::PF_Chart_ReverseIterator::operator++()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (--index_ < 0)
    {
        index_ = -1;
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator++  ----- 

PF_Chart::PF_Chart_ReverseIterator& PF_Chart::PF_Chart_ReverseIterator::operator+=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_less(index_-=n, 0))
    {
        fmt::print("index in between: {}\n", index_);
        index_ = -1;
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator+=  ----- 

PF_Chart::PF_Chart_ReverseIterator& PF_Chart::PF_Chart_ReverseIterator::operator--()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(++index_, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator--  ----- 

PF_Chart::PF_Chart_ReverseIterator& PF_Chart::PF_Chart_ReverseIterator::operator-=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(index_+=n, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}		// -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator-+  ----- 

