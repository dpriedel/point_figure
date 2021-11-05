// =====================================================================================
// 
//       Filename:  p_f_data.cpp
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
#include <iostream>

#include <chartdir.h>
#include <fmt/format.h>

#include <range/v3/algorithm/for_each.hpp>

#include "DDecQuad.h"
#include "PF_Chart.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart (const std::string& symbol, DprDecimal::DDecQuad box_size, int32_t reversal_boxes,
        PF_Column::FractionalBoxes fractional_boxes, bool use_logarithms)
    : current_column_{box_size, reversal_boxes, fractional_boxes}, symbol_{symbol},
    box_size_{box_size}, reversal_boxes_{reversal_boxes}, fractional_boxes_{fractional_boxes},
    use_logarithms_{use_logarithms}

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
    if (fractional_boxes_ != rhs.fractional_boxes_)
    {
        return false;
    }

    if (use_logarithms_ != rhs.use_logarithms_)
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
//    std::cout << "Adding value: " << new_value << '\n';
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

        AddValue(DprDecimal::DDecQuad(fields[1]), the_time);
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

void PF_Chart::ConstructChartAndWriteToFile (const fs::path& output_filename) const
{
    // this code comes pretty much right out of the cppdemo code
    // with some modifications for good memory management practices.

    // NOTE: there are a bunch of numeric constants here related to 
    // chart configuration. These should be paramets eventually.

    std::vector<double> highData;
    std::vector<double> lowData;
    std::vector<double> openData;
    std::vector<double> closeData;

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
    std::unique_ptr<XYChart> c = std::make_unique<XYChart>(600, 350);

    c->setPlotArea(50, 25, 500, 250)->setGridColor(0xc0c0c0, 0xc0c0c0);

    c->addTitle(fmt::format("{}X{} for {}", box_size_.ToDouble(), reversal_boxes_, symbol_).c_str());

//    c->xAxis()->setTitle("Jan 2001");

    // we don't need any labels on the x-axis now.
    // Set the labels on the x axis. Rotate the labels by 45 degrees.
//    c->xAxis()->setLabels(StringArray(labels, labels_size))->setFontAngle(45);

    // Add a title to the y axis
    c->yAxis()->setTitle("Tick data");

    c->yAxis()->setTickDensity(20, 10);
    c->yAxis2()->copyAxis(c->yAxis());

    auto* layer = c->addCandleStickLayer(DoubleArray(highData.data(), highData.size()),
        DoubleArray(lowData.data(), lowData.size()), DoubleArray(openData.data(), openData.size()),
        DoubleArray( closeData.data(), closeData.size()), 0x00ff00, 0xff0000);

    // Set the line width to 2 pixels
    layer->setLineWidth(2);

    // Output the chart
    c->makeChart(output_filename.c_str());

}		// -----  end of method PF_Chart::ConstructChartAndWriteToFile  ----- 


Json::Value PF_Chart::ToJSON () const
{
    Json::Value result;
    result["symbol"] = symbol_;
    result["box_size"] = box_size_.ToStr();
    result["reversal_boxes"] = reversal_boxes_;
    result["y_min"] = y_min_.ToStr();
    result["y_max"] = y_max_.ToStr();
    result["use_logarithms"] = use_logarithms_;

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

    switch(fractional_boxes_)
    {
        case PF_Column::FractionalBoxes::e_integral:
            result["fractional_boxes"] = "integral";
            break;

        case PF_Column::FractionalBoxes::e_fractional:
            result["fractional_boxes"] = "fractional";
            break;
    };
    result["first_date"] = first_date_.time_since_epoch().count();
    result["last_change_date"] = last_change_date_.time_since_epoch().count();
    result["last_check_date"] = last_checked_date_.time_since_epoch().count();
    
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
    box_size_ = DprDecimal::DDecQuad{new_data["box_size"].asString()};
    reversal_boxes_ = new_data["reversal_boxes"].asInt();
    y_min_ = DprDecimal::DDecQuad{new_data["y_min"].asString()};
    y_max_ = DprDecimal::DDecQuad{new_data["y_max"].asString()};
    use_logarithms_ = new_data["use_logarithms"].asBool();

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

    const auto fractional = new_data["fractional_boxes"].asString();
    if (fractional  == "integral")
    {

        fractional_boxes_ = PF_Column::FractionalBoxes::e_integral;
    }
    else if (direction == "fractional")
    {
        fractional_boxes_ = PF_Column::FractionalBoxes::e_fractional;
    }
    else
    {
        throw std::invalid_argument{fmt::format("Invalid fractional_boxes provided: {}. Must be 'integral' or 'fractional'.", fractional)};
    }

    first_date_ = PF_Column::tpt{std::chrono::nanoseconds{new_data["first_date"].asInt64()}};
    last_change_date_ = PF_Column::tpt{std::chrono::nanoseconds{new_data["last_change_date"].asInt64()}};
    last_checked_date_ = PF_Column::tpt{std::chrono::nanoseconds{new_data["last_check_date"].asInt64()}};

    // lastly, we can do our columns 

    const auto& cols = new_data["columns"];
    ranges::for_each(cols, [this](const auto& next_val) { this->columns_.emplace_back(next_val); });

    current_column_ = {new_data["current_column"]};

}		// -----  end of method PF_Chart::FromJSON  ----- 


    // ===  FUNCTION  ======================================================================
    //         Name:  ComputeATR
    //  Description:  Expects the input data is in descending order by date
    // =====================================================================================

DprDecimal::DDecQuad ComputeATR(std::string_view symbol, const Json::Value& the_data, int32_t how_many_days, bool use_adjusted)
{
    BOOST_ASSERT_MSG(the_data.size() > how_many_days, fmt::format("Not enough data provided. Need at least: {} values", how_many_days).c_str());

    DprDecimal::DDecQuad total;

    DprDecimal::DDecQuad high_minus_low;
    DprDecimal::DDecQuad high_minus_prev_close;
    DprDecimal::DDecQuad low_minus_prev_close;

    for (int i = 0; i < how_many_days; ++i)
    {
//        std::cout << "data: " << the_data[i] << '\n';

        if (use_adjusted)
        {
            high_minus_low = DprDecimal::DDecQuad{the_data[i]["adjHigh"].asString()} - DprDecimal::DDecQuad{the_data[i]["adjLow"].asString()};
            high_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["adjHigh"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["adjClose"].asString()}).abs();
            low_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["adjLow"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["adjClose"].asString()}).abs();
        }
        else
        {
            high_minus_low = DprDecimal::DDecQuad{the_data[i]["high"].asString()} - DprDecimal::DDecQuad{the_data[i]["low"].asString()};
            high_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["high"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["close"].asString()}).abs();
            low_minus_prev_close = (DprDecimal::DDecQuad{the_data[i]["low"].asString()} - DprDecimal::DDecQuad{the_data[i + 1]["close"].asString()}).abs();
        }

        DprDecimal::DDecQuad max = DprDecimal::max(high_minus_low, DprDecimal::max(high_minus_prev_close, low_minus_prev_close));

//        std::cout << "h - l: " << high_minus_low << " h - pc: " << high_minus_prev_close << " l - pc: " << low_minus_prev_close << " max: " << max << '\n';
        
        total += max;
    }

    std::cout << "total: " << total << '\n';
    return total /= how_many_days;
}		// -----  end of function ComputeATR  -----

