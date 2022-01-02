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

//#include <iterator>
#include <chrono>
#include <iostream>

#include <chartdir.h>
#include <fmt/format.h>

#include <range/v3/algorithm/for_each.hpp>

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
    first_date_{rhs.first_date_}, last_change_date_{rhs.last_change_date_}, last_checked_date_{rhs.last_checked_date_},
    box_size_{rhs.box_size_}, reversal_boxes_{rhs.reversal_boxes_}, y_min_{rhs.y_min_}, y_max_{rhs.y_max_},
    current_direction_{rhs.current_direction_}, box_type_{rhs.box_type_}, box_scale_{rhs.box_scale_}

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

PF_Chart::PF_Chart (PF_Chart&& rhs)
    : boxes_{std::move(rhs.boxes_)}, columns_{std::move(rhs.columns_)}, current_column_{std::move(rhs.current_column_)},
    symbol_{rhs.symbol_}, first_date_{rhs.first_date_}, last_change_date_{rhs.last_change_date_},
    last_checked_date_{rhs.last_checked_date_}, box_size_{std::move(rhs.box_size_)},
    reversal_boxes_{std::move(rhs.reversal_boxes_)}, y_min_{std::move(rhs.y_min_)}, y_max_{std::move(rhs.y_max_)},
    current_direction_{rhs.current_direction_}, box_type_{rhs.box_type_}, box_scale_{rhs.box_scale_}

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

PF_Chart::PF_Chart (const std::string& symbol, DprDecimal::DDecQuad box_size, int32_t reversal_boxes,
        Boxes::BoxType box_type, Boxes::BoxScale box_scale)
    : boxes_{box_size, box_type, box_scale}, current_column_{&boxes_, reversal_boxes}, symbol_{symbol},
    box_size_{box_size}, reversal_boxes_{reversal_boxes}, box_type_{box_type},
    box_scale_{box_scale}

{
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Chart::PF_Chart (const Json::Value& new_data)
{
    this->FromJSON(new_data);
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  ----- 

PF_Chart& PF_Chart::operator= (const PF_Chart& rhs)
{
    if (this != &rhs)
    {
        boxes_ = rhs.boxes_;
        columns_ = rhs.columns_;
        current_column_ = rhs.current_column_;
        symbol_ = rhs.symbol_;
        first_date_ = rhs.first_date_;
        last_change_date_ = rhs.last_change_date_;
        last_checked_date_ = rhs.last_checked_date_;
        box_size_ = rhs.box_size_;
        reversal_boxes_ = rhs.reversal_boxes_;
        y_min_ = rhs.y_min_;
        y_max_ = rhs.y_max_;
        current_direction_ = rhs.current_direction_;
        box_type_ = rhs.box_type_;
        box_scale_ = rhs.box_scale_;

        // now, the reason for doing this explicitly is to fix the column box pointers.

        ranges::for_each(columns_, [this] (auto& col) { col.boxes_ = &this->boxes_; });
        current_column_.boxes_ = &boxes_;
    }
    return *this;
}		// -----  end of method PF_Chart::operator=  ----- 

PF_Chart& PF_Chart::operator= (PF_Chart&& rhs)
{
    if (this != &rhs)
    {
        boxes_ = std::move(rhs.boxes_);
        columns_ = std::move(rhs.columns_);
        current_column_ = std::move(rhs.current_column_);
        symbol_ = rhs.symbol_;
        first_date_ = rhs.first_date_;
        last_change_date_ = rhs.last_change_date_;
        last_checked_date_ = rhs.last_checked_date_;
        box_size_ = std::move(rhs.box_size_);
        reversal_boxes_ = std::move(rhs.reversal_boxes_);
        y_min_ = std::move(rhs.y_min_);
        y_max_ = std::move(rhs.y_max_);
        current_direction_ = rhs.current_direction_;
        box_type_ = rhs.box_type_;
        box_scale_ = rhs.box_scale_;

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
    if (box_size_ != rhs.box_size_)
    {
        return false;
    }
    if (reversal_boxes_ != rhs.reversal_boxes_)
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
    if (box_type_ != rhs.box_type_)
    {
        return false;
    }

    if (box_scale_ != rhs.box_scale_)
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

PF_Column::Status PF_Chart::AddValue(const DprDecimal::DDecQuad& new_value, PF_Column::tpt the_time)
{
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
    }
    else if (status == PF_Column::Status::e_reversal)
    {
        columns_.push_back(current_column_);
        current_column_ = std::move(new_col.value());

        // now continue on processing the value.
        
        status = current_column_.AddValue(new_value, the_time).first;
        current_direction_ = current_column_.GetDirection();
    }
    return status;
}		// -----  end of method PF_Chart::AddValue  ----- 

void PF_Chart::LoadData (std::istream* input_data, std::string_view date_format, char delim)
{
    std::string buffer;
    while ( ! input_data->eof())
    {
        std::getline(*input_data, buffer);
        if (input_data->fail())
        {
            continue;
        }
        // need to split out fields

        auto fields = split_string<std::string_view>(buffer, delim);

        PF_Column::tpt the_time = StringToTimePoint(date_format, fields[0]);

        AddValue(DprDecimal::DDecQuad{std::string{fields[1]}}, the_time);
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
}

std::string PF_Chart::ChartName () const
{
//    std::string chart_name = fmt::format("{}_{}X{}.json", symbol_, GetBoxsize(), GetReversalboxes(), "json");
    std::string chart_name = fmt::format("{}.json", symbol_);
    return chart_name;
}		// -----  end of method PF_Chart::ChartName  ----- 

void PF_Chart::ConstructChartGraphAndWriteToFile (const fs::path& output_filename) const
{
    // this code comes pretty much right out of the cppdemo code
    // with some modifications for good memory management practices.

    // NOTE: there are a bunch of numeric constants here related to 
    // chart configuration. These should be paramets eventually.

    std::vector<double> highData;
    std::vector<double> lowData;
    std::vector<double> openData;
    std::vector<double> closeData;

    // for x-axis label, we use the begin date for each column 
    // the chart software wants an array of const char*.
    // this will take several steps.

    std::vector<std::string> x_axis_labels;

    for (const auto& col : columns_)
    {
        lowData.push_back(col.GetBottom().ToDouble());
        highData.push_back(col.GetTop().ToDouble());

        if (col.GetDirection() == PF_Column::Direction::e_up)
        {
            openData.push_back(col.GetBottom().ToDouble());
            closeData.push_back(col.GetTop().ToDouble());
        }
        else
        {
            openData.push_back(col.GetTop().ToDouble());
            closeData.push_back(col.GetBottom().ToDouble());
        }
        x_axis_labels.push_back(TimePointToYMDString(col.GetTimeSpan().first));
    }

    lowData.push_back(current_column_.GetBottom().ToDouble());
    highData.push_back(current_column_.GetTop().ToDouble());

    if (current_column_.GetDirection() == PF_Column::Direction::e_up)
    {
        openData.push_back(current_column_.GetBottom().ToDouble());
        closeData.push_back(current_column_.GetTop().ToDouble());
    }
    else
    {
        openData.push_back(current_column_.GetTop().ToDouble());
        closeData.push_back(current_column_.GetBottom().ToDouble());
    }
    x_axis_labels.push_back(TimePointToYMDString(current_column_.GetTimeSpan().first));

    std::vector<const char*> x_labels;

    ranges::for_each(x_axis_labels, [&x_labels](const auto& date) { x_labels.push_back(date.data()); });

    std::unique_ptr<XYChart> c = std::make_unique<XYChart>(600, 700);

    c->setPlotArea(50, 50, 500, 550)->setGridColor(0xc0c0c0, 0xc0c0c0);

    c->addTitle(fmt::format("{}{} X {} for {}  {}", box_size_.ToDouble(),
                (IsPercent() ? "%" : ""), reversal_boxes_, symbol_,
                (IsPercent() ? "percent" : "")).c_str());

//    c->xAxis()->setTitle("Jan 2001");

    // Set the labels on the x axis. Rotate the labels by 45 degrees.
//    if (x_labels.size() < 100)
//    {
        auto* textbox = c->xAxis()->setLabels(StringArray(x_labels.data(), x_labels.size()));
        textbox->setFontStyle("Times new Roman");
//        textbox->setFontAngle(45);
        c->xAxis()->setLabelStep(10);
//        c->xAxis()->setLabels(StringArray(x_labels.data(), x_labels.size()))->setFontAngle(180);
//    }

    // Add a title to the y axis
    c->yAxis()->setTitle("Tick data");

    if (! IsPercent())
    {
        c->yAxis()->setTickDensity(20, 10);
    }
    else
    {
        c->yAxis()->setLogScale(GetYLimits().first.ToDouble(), GetYLimits().second.ToDouble());
    }
    c->yAxis2()->copyAxis(c->yAxis());

    auto* layer = c->addCandleStickLayer(DoubleArray(highData.data(), highData.size()),
        DoubleArray(lowData.data(), lowData.size()), DoubleArray(openData.data(), openData.size()),
        DoubleArray( closeData.data(), closeData.size()), 0x00ff00, 0xff0000);

    // Set the line width to 2 pixels
    layer->setLineWidth(2);

    // Output the chart
    c->makeChart(output_filename.c_str());

}		// -----  end of method PF_Chart::ConstructChartAndWriteToFile  ----- 

void PF_Chart::ConvertChartToJsonAndWriteToStream (std::ostream& stream) const
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";        // compact printing and string formatting 
    std::unique_ptr<Json::StreamWriter> const writer( builder.newStreamWriter());
    writer->write(this->ToJSON(), &stream);
    stream << std::endl;  // add lf and flush
    return ;
}		// -----  end of method PF_Chart::ConvertChartToJsonAndWriteToStream  ----- 


Json::Value PF_Chart::ToJSON () const
{
    Json::Value result;
    result["symbol"] = symbol_;
    result["first_date"] = first_date_.time_since_epoch().count();
    result["last_change_date"] = last_change_date_.time_since_epoch().count();
    result["last_check_date"] = last_checked_date_.time_since_epoch().count();

    result["box_size"] = box_size_.ToStr();
    result["reversal_boxes"] = reversal_boxes_;
    result["y_min"] = y_min_.ToStr();
    result["y_max"] = y_max_.ToStr();

    switch(current_direction_)
    {
        case PF_Column::Direction::e_unknown:
            result["current_direction"] = "unknown";
            break;

        case PF_Column::Direction::e_down:
            result["current_direction"] = "down";
            break;

        case PF_Column::Direction::e_up:
            result["current_direction"] = "up";
            break;
    };

    result["box_type"] = box_type_ == Boxes::BoxType::e_integral ? "integral" : "fractional";
    
    result["box_scale"] = box_scale_ == Boxes::BoxScale::e_linear ? "linear" : "percent";

    result["boxes"] = boxes_.ToJSON();

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

    first_date_ = PF_Column::tpt{std::chrono::nanoseconds{new_data["first_date"].asInt64()}};
    last_change_date_ = PF_Column::tpt{std::chrono::nanoseconds{new_data["last_change_date"].asInt64()}};
    last_checked_date_ = PF_Column::tpt{std::chrono::nanoseconds{new_data["last_check_date"].asInt64()}};

    box_size_ = DprDecimal::DDecQuad{new_data["box_size"].asString()};
    reversal_boxes_ = new_data["reversal_boxes"].asInt();
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

    const auto box_type = new_data["box_type"].asString();
    if (box_type  == "integral")
    {
        box_type_ = Boxes::BoxType::e_integral;
    }
    else if (box_type == "fractional")
    {
        box_type_ = Boxes::BoxType::e_fractional;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid box_type provided: {}. Must be 'integral' or 'fractional'.", box_type)};
    }

    const auto box_scale = new_data["box_scale"].asString();
    if (box_scale == "linear")
    {
        box_scale_ = Boxes::BoxScale::e_linear;
    }
    else if (box_scale == "percent")
    {
        box_scale_ = Boxes::BoxScale::e_percent;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid box_scale provided: {}. Must be 'linear' or 'percent'.", box_scale)};
    }
    
    boxes_ = new_data["boxes"];

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

DprDecimal::DDecQuad ComputeATR(std::string_view symbol, const Json::Value& the_data, int32_t how_many_days, UseAdjusted use_adjusted)
{
    BOOST_ASSERT_MSG(the_data.size() > how_many_days, fmt::format("Not enough data provided. Need at least: {} values", how_many_days).c_str());

    DprDecimal::DDecQuad total;

    DprDecimal::DDecQuad high_minus_low;
    DprDecimal::DDecQuad high_minus_prev_close;
    DprDecimal::DDecQuad low_minus_prev_close;

    if (use_adjusted == UseAdjusted::e_Yes)
    {
        for (int i = 0; i < how_many_days; ++i)
        {
            high_minus_low = DprDecimal::DDecQuad{the_data[i]["adjHigh"].asString()} - DprDecimal::DDecQuad{the_data[i]["adjLow"].asString()};
            high_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["adjHigh"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["adjClose"].asString()}).abs();
            low_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["adjLow"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["adjClose"].asString()}).abs();

            DprDecimal::DDecQuad max = DprDecimal::max(high_minus_low, DprDecimal::max(high_minus_prev_close, low_minus_prev_close));

            total += max;
        }
    }
    else
    {
        for (int i = 0; i < how_many_days; ++i)
        {
            high_minus_low = DprDecimal::DDecQuad{the_data[i]["high"].asString()} - DprDecimal::DDecQuad{the_data[i]["low"].asString()};
            high_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["high"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["close"].asString()}).abs();
            low_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["low"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["close"].asString()}).abs();

            DprDecimal::DDecQuad max = DprDecimal::max(high_minus_low, DprDecimal::max(high_minus_prev_close, low_minus_prev_close));
            
            total += max;
        }
    }

//    std::cout << "total: " << total << '\n';
    return total /= how_many_days;
}		// -----  end of function ComputeATR  -----

