// =====================================================================================
//
//       Filename:  ConstructChartGraphic.h
//
//    Description:  Code to generate graphic representation of PF_Chart
//
//        Version:  1.0
//        Created:  05/13/2023 08:46:15 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//   Organization:
//
// =====================================================================================

#ifndef _CONSTRUCTCHARTGRAPHIC_INC_
#define _CONSTRUCTCHARTGRAPHIC_INC_

#include <memory>

class XYChart;

#include "PF_Chart.h"

// void ConstructChartGraphAndWriteToFile(const PF_Chart& the_chart, const fs::path& output_filename, const
// streamed_prices& streamed_prices,
//                                        const std::string& show_trend_lines, PF_Chart::X_AxisFormat
//                                        date_or_time=PF_Chart::X_AxisFormat::e_show_date);

void ConstructCDChartGraphicAndWriteToFile(const PF_Chart& the_chart, const fs::path& output_filename,
                                           const StreamedPrices& streamed_prices, const std::string& show_trend_lines,
                                           PF_Chart::X_AxisFormat date_or_time = PF_Chart::X_AxisFormat::e_show_date);

std::unique_ptr<XYChart> ConstructCDChartGraphicAddPFSignals(const PF_Chart& the_chart, size_t skipped_columns,
                                                           std::unique_ptr<XYChart> the_graphic);

std::unique_ptr<XYChart> ConstructCDChartGraphicPricesChart(
                            const PF_Chart& the_chart, const StreamedPrices& streamed_prices,
                            PF_Chart::X_AxisFormat date_or_time = PF_Chart::X_AxisFormat::e_show_date);

#endif  // ----- #ifndef _CONSTRUCTCHARTGRAPHIC_INC_  -----
