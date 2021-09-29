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

#include "PF_Chart.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart (const std::string& symbol, DprDecimal::DDecDouble boxsize, int32_t reversal_boxes,
        PF_Column::FractionalBoxes fractional_boxes)
	: symbol_{symbol}, box_size_{boxsize}, reversal_boxes_{reversal_boxes},
        current_direction_{PF_Column::Direction::e_unknown}, fractional_boxes_{fractional_boxes}

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

    // if we got here, then we can look at our data 

    if (columns_ != rhs.columns_)
    {
        return false;
    }
    if (*current_column_ != *rhs.current_column_)
    {
        return false;
    }

    return true;
}		// -----  end of method PF_Chart::operator==  ----- 

void PF_Chart::ConstructChartAndWriteToFile (fs::path output_filename) const
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

    result["current_column"] = current_column_->ToJSON();

    return result;

}		// -----  end of method PF_Chart::ToJSON  ----- 


void PF_Chart::FromJSON (const Json::Value& new_data)
{
    symbol_ = new_data["symbol"].asString();
    box_size_ = DprDecimal::DDecDouble{new_data["box_size"].asString()};
    reversal_boxes_ = new_data["reversal_boxes"].asInt();
    y_min_ = DprDecimal::DDecDouble{new_data["y_min"].asString()};
    y_max_ = DprDecimal::DDecDouble{new_data["y_max"].asString()};

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
    auto cols_size = cols.size();
    for (int i = 0; i < cols_size; ++i)
    {
        columns_.emplace_back(PF_Column{cols[i]});
    }

    current_column_ = std::make_unique<PF_Column>(PF_Column(new_data["current_column"]));

}		// -----  end of method PF_Chart::FromJSON  ----- 

