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
#include <vector>

class XYChart;

struct Signals_1
{
    std::vector<double> dt_buys_price_;
    std::vector<double> tt_buys_price_;
    std::vector<double> db_sells_price_;
    std::vector<double> tb_sells_price_;
    std::vector<double> bullish_tt_buys_price_;
    std::vector<double> bearish_tb_sells_price_;
    std::vector<double> cat_buys_price_;
    std::vector<double> cat_sells_price_;
    std::vector<double> tt_cat_buys_price_;
    std::vector<double> tb_cat_sells_price_;

    std::vector<double> dt_buys_x_;
    std::vector<double> tt_buys_x_;
    std::vector<double> db_sells_x_;
    std::vector<double> tb_sells_x_;
    std::vector<double> bullish_tt_buys_x_;
    std::vector<double> bearish_tb_sells_x_;
    std::vector<double> cat_buys_x_;
    std::vector<double> cat_sells_x_;
    std::vector<double> tt_cat_buys_x_;
    std::vector<double> tb_cat_sells_x_;
};

struct Signals_2
{
    std::vector<double> dt_buys_price_;
    std::vector<double> tt_buys_price_;
    std::vector<double> db_sells_price_;
    std::vector<double> tb_sells_price_;
    std::vector<double> bullish_tt_buys_price_;
    std::vector<double> bearish_tb_sells_price_;
    std::vector<double> cat_buys_price_;
    std::vector<double> cat_sells_price_;
    std::vector<double> tt_cat_buys_price_;
    std::vector<double> tb_cat_sells_price_;

    std::vector<double> dt_buys_x_;
    std::vector<double> tt_buys_x_;
    std::vector<double> db_sells_x_;
    std::vector<double> tb_sells_x_;
    std::vector<double> bullish_tt_buys_x_;
    std::vector<double> bearish_tb_sells_x_;
    std::vector<double> cat_buys_x_;
    std::vector<double> cat_sells_x_;
    std::vector<double> tt_cat_buys_x_;
    std::vector<double> tb_cat_sells_x_;
};

#include "PF_Chart.h"

// void ConstructChartGraphAndWriteToFile(const PF_Chart& the_chart, const fs::path& output_filename, const
// streamed_prices& streamed_prices,
//                                        const std::string& show_trend_lines, X_AxisFormat
//                                        date_or_time=X_AxisFormat::e_show_date);

void ConstructCDPFChartGraphicAndWriteToFile(const PF_Chart &the_chart, const fs::path &output_filename,
                                             const StreamedPrices &streamed_prices, const std::string &show_trend_lines,
                                             X_AxisFormat date_or_time = X_AxisFormat::e_show_date);

void ConstructCDPFChartGraphicAddPFSignals(const PF_Chart &the_chart, Signals_1 &data_arrays, size_t skipped_columns,
                                           std::unique_ptr<XYChart> &the_graphic);

void ConstructCDPricesGraphicAddSignals(const PF_Chart &the_chart, Signals_2 &data_arrays, size_t skipped_price_cols,
                                        const StreamedPrices &streamed_prices, std::unique_ptr<XYChart> &the_graphic);

void ConstructCDSummaryGraphic(const PF_StreamedSummary &streamed_summary, const fs::path &output_filename);

#endif // ----- #ifndef _CONSTRUCTCHARTGRAPHIC_INC_  -----
