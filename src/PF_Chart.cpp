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
#include <chrono>
#include <cstdint>
#include <date/date.h>
#include <iostream>
#include <fstream>
#include <utility>

#include<date/tz.h>

//#include <chartdir.h>
#include <fmt/format.h>
#include <fmt/chrono.h>

#include <range/v3/algorithm/for_each.hpp>
#include <range/v3/view/drop.hpp>


#include <pybind11/embed.h> // everything needed for embedding
#include <pybind11/gil.h>
#include <pybind11/stl.h>

#include <spdlog/spdlog.h>

namespace py = pybind11;
using namespace py::literals;
using namespace std::string_literals;

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
    : boxes_{rhs.boxes_}, columns_{rhs.columns_}, current_column_{rhs.current_column_}, symbol_{rhs.symbol_},
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
    : boxes_{std::move(rhs.boxes_)}, columns_{std::move(rhs.columns_)}, current_column_{std::move(rhs.current_column_)},
    symbol_{std::move(rhs.symbol_)}, fname_box_size_{std::move(rhs.fname_box_size_)}, atr_{std::move(rhs.atr_)},
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
	current_column_ = PF_Column{&boxes_, reversal_boxes}; 
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
    Json::Value chart_data = chart_db.GetPFChartData(PF_Chart::ChartName(vals, interval, "json"));
    PF_Chart chart_from_db{chart_data};
	return chart_from_db;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  ----- 

PF_Chart& PF_Chart::operator= (const PF_Chart& rhs)
{
    if (this != &rhs)
    {
        boxes_ = rhs.boxes_;
        columns_ = rhs.columns_;
        current_column_ = rhs.current_column_;
        symbol_ = rhs.symbol_;
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
        columns_ = std::move(rhs.columns_);
        current_column_ = std::move(rhs.current_column_);
        symbol_ = rhs.symbol_;
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

    if (! IsEmpty())
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
    }
    else if (status == PF_Column::Status::e_reversal)
    {
        columns_.push_back(current_column_);
        current_column_ = std::move(new_col.value());

        // now continue on processing the value.
        
        status = current_column_.AddValue(new_value, the_time).first;

        last_change_date_ = the_time;
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

std::string PF_Chart::ChartName (std::string_view interval, std::string_view suffix) const
{
    std::string chart_name = fmt::format("{}_{}{}X{}_{}{}.{}",
            symbol_,
            fname_box_size_,
            (IsPercent() ? "%" : ""),
            GetReversalboxes(),
            (IsPercent() ? "percent" : "linear"),
            (! interval.empty() ? "_"s += interval : ""),
            suffix);
    return chart_name;
}		// -----  end of method PF_Chart::ChartName  ----- 

std::string PF_Chart::ChartName (const PF_ChartParams& vals, std::string_view interval, std::string_view suffix)
{
    std::string chart_name = fmt::format("{}_{}{}X{}_{}{}.{}",
            std::get<e_symbol>(vals),
            std::get<e_box_size>(vals),
            (std::get<e_box_scale>(vals) == Boxes::BoxScale::e_percent ? "%" : ""),
            std::get<e_reversal>(vals),
            (std::get<e_box_scale>(vals) == Boxes::BoxScale::e_linear ? "linear" : "percent"),
            (! interval.empty() ? "_"s += interval : ""),
            suffix);
    return chart_name;
}		// -----  end of method PF_Chart::ChartName  ----- 

void PF_Chart::ConstructChartGraphAndWriteToFile (const fs::path& output_filename, const std::string& show_trend_lines, X_AxisFormat date_or_time) const
{
	BOOST_ASSERT_MSG(! IsEmpty(), fmt::format("Chart for symbol: {} contains no data. Unable to draw graphic.", symbol_).c_str());

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
	
	size_t skipped_columns = max_columns_for_graph_ < 1 || GetNumberOfColumns() <= max_columns_for_graph_ ? 0 : GetNumberOfColumns() - max_columns_for_graph_; 

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

	// want to show approximate overall change in value (computed from boxes, not actual prices)
	
	DprDecimal::DDecQuad first_value = 0;
	DprDecimal::DDecQuad last_value = 0;

	if (GetNumberOfColumns() > 1)
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
        explanation_text = "Orange: 1-step Up then reversal Down. Blue: 1-step Down then reversal Up.";
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
        "IsUp"_a = direction_is_up,
        "StepBack"_a = had_step_back,
        "ChartTitle"_a = chart_title,
        "ChartFileName"_a = output_filename.string(),
        "DateTimeFormat"_a = date_or_time == X_AxisFormat::e_show_date ? "%F" : "%H:%M:%S",
        "Y_min"_a = GetYLimits().first.ToDouble(),
        "Y_max"_a = GetYLimits().second.ToDouble(),
        "UseLogScale"_a = IsPercent(),
        "ShowTrendLines"_a = show_trend_lines
    };

        // Execute Python code, using the variables saved in `locals`

//        py::gil_scoped_acquire gil{};
        py::exec(R"(
        PF_DrawChart.DrawChart(the_data, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat, ShowTrendLines, UseLogScale, Y_min, Y_max)
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
	
	size_t skipped_columns = max_columns_for_graph_ < 1 || GetNumberOfColumns() <= max_columns_for_graph_ ? 0 : GetNumberOfColumns() - max_columns_for_graph_; 

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
    result["boxes"] = boxes_.ToJSON();
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
    boxes_ = new_data["boxes"];

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

