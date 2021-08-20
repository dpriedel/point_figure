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
	: symbol_{symbol}, boxsize_{boxsize}, reversal_boxes_{reversal_boxes},
        current_direction_{PF_Column::Direction::e_unknown}, fractional_boxes_{fractional_boxes}

{
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

void PF_Chart::ExportData (std::ostream* output_data)
{
    // for now, just print our column info.

    for (const auto& a_col : columns_)
    {
        std::cout << "bottom: " << a_col.GetBottom() << " top: " << a_col.GetTop()
            << " direction: " << a_col.GetDirection()
            << (a_col.GetHadReversal() ? " one step back reversal" : "") << '\n';
    }
    std::cout << "Chart y limits: <" << y_min_ << ", " << y_max_ << ">\n";
}		// -----  end of method PF_Chart::ExportData  ----- 

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

    c->addTitle(fmt::format("{}X{} for {}", boxsize_.ToDouble(), reversal_boxes_, symbol_).c_str());

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

