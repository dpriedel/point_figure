// =====================================================================================
//
//       Filename:  ConstructChartGraphic.cpp
//
//    Description:  Code to generate graphic representation of PF_Chart
//
//        Version:  1.0
//        Created:  05/13/2023 08:52:30 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//   Organization:
//
// =====================================================================================

#include <algorithm>
#include <chrono>
#include <memory>
#include <ranges>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

// #include <pybind11/embed.h>    // everything needed for embedding
// #include <pybind11/gil.h>
// #include <pybind11/stl.h>
#include <spdlog/spdlog.h>

#include <chartdir.h>

// namespace py = pybind11;
// using namespace py::literals;

#include "ConstructChartGraphic.h"

// void ConstructChartGraphAndWriteToFile(const PF_Chart& the_chart, const fs::path& output_filename, const streamed_prices& streamed_prices,
//                                        const std::string& show_trend_lines, PF_Chart::X_AxisFormat date_or_time)
// {
//     BOOST_ASSERT_MSG(!the_chart.empty(),
//                      std::format("Chart for symbol: {} contains no data. Unable to draw graphic.", the_chart.GetSymbol()).c_str());
//
//     std::vector<double> highData;
//     std::vector<double> lowData;
//     std::vector<double> openData;
//     std::vector<double> closeData;
//     std::vector<bool> had_step_back;
//     std::vector<bool> direction_is_up;
//
//     // for x-axis label, we use the begin date for each column
//     // the chart software wants an array of const char*.
//     // this will take several steps.
//
//     std::vector<std::string> x_axis_labels;
//
//     // limit the number of columns shown on graphic if requested
//
//     const auto max_columns_for_graph = the_chart.GetMaxGraphicColumns();
//     size_t skipped_columns =
//         max_columns_for_graph < 1 || the_chart.size() <= max_columns_for_graph ? 0 : the_chart.size() - max_columns_for_graph;
//
//     // we want to mark the openning value on the chart so change can be seen when drawing only most recent columns.
//
//     const auto& first_col = the_chart[0];
//     double openning_price =
//         first_col.GetDirection() == PF_Column::Direction::e_Up ? dec2dbl(first_col.GetBottom()) : dec2dbl(first_col.GetTop());
//
//     for (const auto& col : the_chart | vws::drop(skipped_columns))
//     {
//         lowData.push_back(dec2dbl(col.GetBottom()));
//         highData.push_back(dec2dbl(col.GetTop()));
//
//         if (col.GetDirection() == PF_Column::Direction::e_Up)
//         {
//             openData.push_back(dec2dbl(col.GetBottom()));
//             closeData.push_back(dec2dbl(col.GetTop()));
//             direction_is_up.push_back(true);
//         }
//         else
//         {
//             openData.push_back(dec2dbl(col.GetTop()));
//             closeData.push_back(dec2dbl(col.GetBottom()));
//             direction_is_up.push_back(false);
//         }
//         if (date_or_time == PF_Chart::X_AxisFormat::e_show_date)
//         {
//             x_axis_labels.push_back(std::format("{:%F}", col.GetTimeSpan().first));
//         }
//         else
//         {
//             x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first));
//         }
//         had_step_back.push_back(col.GetHadReversal());
//     }
//
//     // lowData.push_back(dec2dbl(the_chart.back().GetBottom()));
//     // highData.push_back(dec2dbl(the_chart.back().GetTop()));
//     //
//     // if (the_chart.back().GetDirection() == PF_Column::Direction::e_Up)
//     // {
//     //     openData.push_back(dec2dbl(the_chart.back().GetBottom()));
//     //     closeData.push_back(dec2dbl(the_chart.back().GetTop()));
//     //     direction_is_up.push_back(true);
//     // }
//     // else
//     // {
//     //     openData.push_back(dec2dbl(the_chart.back().GetTop()));
//     //     closeData.push_back(dec2dbl(the_chart.back().GetBottom()));
//     //     direction_is_up.push_back(false);
//     // }
//     // if (date_or_time == PF_Chart::X_AxisFormat::e_show_date)
//     // {
//     //     x_axis_labels.push_back(std::format("{:%F}", the_chart.back().GetTimeSpan().first));
//     // }
//     // else
//     // {
//     //     x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(the_chart.back().GetTimeSpan().first));
//     // }
//     //
//     // had_step_back.push_back(the_chart.back().GetHadReversal());
//
//     // extract and format the Signals, if any, from the chart. The drawing code can plot
//     // them or not.
//     // NOTE: we need to zap the column number with the skipped columns offset
//
//     // let's try to do lower level setup for signals drawing here.
//     // we want 1 signal per column and that should be the signal with highest priority
//     // data is formated as needed by mplfinance library to build lists of marks to overlay
//     // on graphic. this saves a bunch of python code
//
//     const auto& sngls = the_chart.GetSignals();
//     std::vector<double> dt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> db_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> tt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> tb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> btt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> btb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> tt_cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
//     std::vector<double> tb_cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());->
//
//     int had_dt_buy = 0;
//     int had_tt_buy = 0;
//     int had_db_sell = 0;
//     int had_tb_sell = 0;
//     int had_bullish_tt_buy = 0;
//     int had_bearish_tb_sell = 0;
//     int had_catapult_buy = 0;
//     int had_catapult_sell = 0;
//     int had_tt_catapult_buy = 0;
//     int had_tb_catapult_sell = 0;
//
//     for (const auto& sigs : sngls | vws::filter([skipped_columns](const auto& s) { return s.column_number_ >= skipped_columns; }) |
//                                 vws::chunk_by([](const auto& a, const auto& b) { return a.column_number_ == b.column_number_; }))
//     {
//         const auto most_important = rng::max_element(sigs, {}, [](const auto& s) { return std::to_underlying(s.priority_); });
//         switch (most_important->signal_type_)
//         {
//             using enum PF_SignalType;
//             case e_unknown:
//                 break;
//             case e_double_top_buy:
//                 dt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_dt_buy += 1;
//                 break;
//             case e_double_bottom_sell:
//                 db_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_db_sell += 1;
//                 break;
//             case e_triple_top_buy:
//                 tt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_tt_buy += 1;
//                 break;
//             case e_triple_bottom_sell:
//                 tb_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_tb_sell += 1;
//                 break;
//             case e_bullish_tt_buy:
//                 btt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_bullish_tt_buy += 1;
//                 break;
//             case e_bearish_tb_sell:
//                 btb_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_bearish_tb_sell += 1;
//                 break;
//             case e_catapult_buy:
//                 cat_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_catapult_buy += 1;
//                 break;
//             case e_catapult_sell:
//                 cat_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_catapult_sell += 1;
//                 break;
//             case e_ttop_catapult_buy:
//                 tt_cat_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_tt_catapult_buy += 1;
//                 break;
//             case e_tbottom_catapult_sell:
//                 tb_cat_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
//                 had_tb_catapult_sell += 1;
//                 break;
//         }
//     }
//
//     // want to show approximate overall change in value (computed from boxes, not actual prices)
//
//     decimal::Decimal first_value = 0;
//     decimal::Decimal last_value = 0;
//
//     if (the_chart.size() > 1)
//     {
//         const auto& first_col = the_chart[0];
//         first_value = first_col.GetDirection() == PF_Column::Direction::e_Up ? first_col.GetBottom() : first_col.GetTop();
//         // apparently, this can happen
//
//         if (first_value == sv2dec("0.0"))
//         {
//             first_value = sv2dec("0.01");
//         }
//     }
//     else
//     {
//         first_value =
//             the_chart.back().GetDirection() == PF_Column::Direction::e_Up ? the_chart.back().GetBottom() : the_chart.back().GetTop();
//     }
//     last_value = the_chart.back().GetDirection() == PF_Column::Direction::e_Up ? the_chart.back().GetTop() : the_chart.back().GetBottom();
//
//     decimal::Decimal overall_pct_chg = ((last_value - first_value) / first_value * 100).rescale(-2);
//
//     std::string skipped_columns_text;
//     if (skipped_columns > 0)
//     {
//         skipped_columns_text = std::format(" (last {} cols)", max_columns_for_graph);
//     }
//     // some explanation for custom box colors.
//
//     std::string explanation_text;
//     if (the_chart.GetReversalboxes() == 1)
//     {
//         explanation_text = "Orange: 1-step Up then reversal Down. Green: 1-step Down then reversal Up.";
//     }
//     auto chart_title =
//         std::format("\n{}{} X {} for {} {}. Overall % change: {}{}\nLast change: {:%a, %b %d, %Y at %I:%M:%S %p %Z}\n{}",
//                     the_chart.GetChartBoxSize().format("f"), (the_chart.IsPercent() ? "%" : ""), the_chart.GetReversalboxes(),
//                     the_chart.GetSymbol(), (the_chart.IsPercent() ? "percent" : ""), overall_pct_chg.format("f"), skipped_columns_text,
//                     std::chrono::zoned_time(std::chrono::current_zone(),
//                                             std::chrono::clock_cast<std::chrono::system_clock>(the_chart.GetLastChangeTime())),
//                     explanation_text);
//
//     py::dict locals =
//         py::dict{"the_data"_a =
//                      py::dict{"Date"_a = x_axis_labels, "Open"_a = openData, "High"_a = highData, "Low"_a = lowData, "Close"_a = closeData},
//                  "ReversalBoxes"_a = the_chart.GetReversalboxes(),
//                  "IsUp"_a = direction_is_up,
//                  "StepBack"_a = had_step_back,
//                  "ChartTitle"_a = chart_title,
//                  "ChartFileName"_a = output_filename.string(),
//                  "DateTimeFormat"_a = date_or_time == PF_Chart::X_AxisFormat::e_show_date ? "%Y-%m-%d" : "%H:%M:%S",
//                  "Y_min"_a = dec2dbl(the_chart.GetYLimits().first),
//                  "Y_max"_a = dec2dbl(the_chart.GetYLimits().second),
//                  "openning_price"_a = openning_price,
//                  "UseLogScale"_a = the_chart.IsPercent(),
//                  "ShowTrendLines"_a = show_trend_lines,
//                  "the_signals"_a = py::dict{"dt_buys"_a = had_dt_buy != 0 ? dt_buys : std::vector<double>{},
//                                             "db_sells"_a = had_db_sell != 0 ? db_sells : std::vector<double>{},
//                                             "tt_buys"_a = had_tt_buy != 0 ? tt_buys : std::vector<double>{},
//                                             "tb_sells"_a = had_tb_sell != 0 ? tb_sells : std::vector<double>{},
//                                             "bullish_tt_buys"_a = had_bullish_tt_buy != 0 ? btt_buys : std::vector<double>{},
//                                             "bearish_tb_sells"_a = had_bearish_tb_sell != 0 ? btb_sells : std::vector<double>{},
//                                             "catapult_buys"_a = had_catapult_buy != 0 ? cat_buys : std::vector<double>{},
//                                             "catapult_sells"_a = had_catapult_sell != 0 ? cat_sells : std::vector<double>{},
//                                             "tt_catapult_buys"_a = had_tt_catapult_buy != 0 ? tt_cat_buys : std::vector<double>{},
//                                             "tb_catapult_sells"_a = had_tb_catapult_sell != 0 ? tb_cat_sells : std::vector<double>{}},
//                  "streamed_prices"_a = py::dict{"the_time"_a = streamed_prices.timestamp_, "price"_a = streamed_prices.price_,
//                                                 "signal_type"_a = streamed_prices.signal_type_}};
//
//     // Execute Python code, using the variables saved in `locals`
//
//     //        py::gil_scoped_acquire gil{};
//     py::exec(R"(
//         PF_DrawChart.DrawChart(the_data, ReversalBoxes, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat,
//         ShowTrendLines, UseLogScale, Y_min, Y_max, openning_price, the_signals, streamed_prices)
//         )",
//              py::globals(), locals);
// }


void ConstructCDChartGraphicAndWriteToFile(const PF_Chart& the_chart, const fs::path& output_filename, const streamed_prices& streamed_prices,
                                       const std::string& show_trend_lines, PF_Chart::X_AxisFormat date_or_time)
{
    BOOST_ASSERT_MSG(!the_chart.empty(),
                     std::format("Chart for symbol: {} contains no data. Unable to draw graphic.", the_chart.GetSymbol()).c_str());

    const auto columns_in_PF_Chart = the_chart.size();

    // for our chart graphic there are 4 types of columns: up, down, reversed-to-up and reversed-to-down.
    // there will be a separate layer for each of these types so a different color can be assigned to each.
    // in order for eveything to line up correctly each layer must contain all the points whether they are
    // used in that layor or not.  'NoValue' values are used. So, fill each layer with 'NoValue' then
    // over-write the values that are used in that layer.

    PF_Chart::ColumnTopBottomList up_layer{columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};
    PF_Chart::ColumnTopBottomList down_layer{columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};
    PF_Chart::ColumnTopBottomList reversed_to_up_layer{columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};
    PF_Chart::ColumnTopBottomList reversed_to_down_layer{columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};

    // collect data for each graphic layer.

    auto up_cols = the_chart.GetTopBottomForColumns(PF_ColumnFilter::e_up_column);
    auto down_cols = the_chart.GetTopBottomForColumns(PF_ColumnFilter::e_down_column);
    PF_Chart::ColumnTopBottomList rev_to_up_cols;
    PF_Chart::ColumnTopBottomList rev_to_down_cols;
    if (the_chart.GetReversalboxes() == 1 && the_chart.HasReversedColumns())
    {
        rev_to_up_cols = the_chart.GetTopBottomForColumns(PF_ColumnFilter::e_reversed_to_up);
        rev_to_down_cols = the_chart.GetTopBottomForColumns(PF_ColumnFilter::e_reversed_to_down);
    }
    // next, merge to data for each column type into the layer list
   
    rng::for_each(up_cols, [&up_layer](const auto& col) { up_layer[col.col_nbr_] = col; });
    rng::for_each(down_cols, [&down_layer](const auto& col) { down_layer[col.col_nbr_] = col; });
    if (! rev_to_up_cols.empty())
    {
        rng::for_each(rev_to_up_cols, [&reversed_to_up_layer](const auto& col) {reversed_to_up_layer[col.col_nbr_] = col; });
    }
    if (! rev_to_down_cols.empty())
    {
        rng::for_each(rev_to_down_cols, [&reversed_to_down_layer](const auto& col) { reversed_to_down_layer[col.col_nbr_] = col; });
    }

    // for x-axis label, we use the begin date for each column
    // the chart software wants an array of const char*.
    // this will take several steps.

    std::vector<std::string> x_axis_labels;
    x_axis_labels.reserve(columns_in_PF_Chart);

    for (const auto& col : the_chart)
    {
        if (date_or_time == PF_Chart::X_AxisFormat::e_show_date)
        {
            x_axis_labels.push_back(std::format("{:%F}", col.GetTimeSpan().first));
        }
        else
        {
            x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first));
        }
    }

    // now, make our data arrays for the graphic software
    // but first, limit the number of columns shown on graphic if requested

    const auto max_columns_for_graph = the_chart.GetMaxGraphicColumns();
    size_t skipped_columns =
        max_columns_for_graph < 1 || the_chart.size() <= max_columns_for_graph ? 0 : the_chart.size() - max_columns_for_graph;

    std::vector<double> up_data_top;
    up_data_top.reserve(columns_in_PF_Chart - skipped_columns);
    std::vector<double> up_data_bot;
    up_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
    for (const auto& col : up_layer | vws::drop(skipped_columns))
    {
        up_data_top.push_back(col.col_top_);
        up_data_bot.push_back(col.col_bot_);
    }

    std::vector<double> down_data_top;
    down_data_top.reserve(columns_in_PF_Chart - skipped_columns);
    std::vector<double> down_data_bot;
    down_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
    for (const auto& col : down_layer | vws::drop(skipped_columns))
    {
        down_data_top.push_back(col.col_top_);
        down_data_bot.push_back(col.col_bot_);
    }

    std::vector<double> reversed_to_up_data_top;
    std::vector<double> reversed_to_up_data_bot;
    if (! rev_to_up_cols.empty())
    {
        reversed_to_up_data_top.reserve(columns_in_PF_Chart - skipped_columns);
        reversed_to_up_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
        for (const auto& col : reversed_to_up_layer | vws::drop(skipped_columns))
        {
            reversed_to_up_data_top.push_back(col.col_top_);
            reversed_to_up_data_bot.push_back(col.col_bot_);
        }
    }

    std::vector<double> reversed_to_down_data_top;
    std::vector<double> reversed_to_down_data_bot;
    if (! rev_to_down_cols.empty())
    {
        reversed_to_down_data_top.reserve(columns_in_PF_Chart - skipped_columns);
        reversed_to_down_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
        for (const auto& col : reversed_to_down_layer | vws::drop(skipped_columns))
        {
            reversed_to_down_data_top.push_back(col.col_top_);
            reversed_to_down_data_bot.push_back(col.col_bot_);
        }
    }

    std::vector<const char*> x_axis_label_data;
    x_axis_label_data.reserve(columns_in_PF_Chart - skipped_columns);
    rng::for_each(x_axis_labels | vws::drop(skipped_columns), [&x_axis_label_data](const auto& label) { x_axis_label_data.push_back(label.c_str()); });

    // NOLINTBEGIN
    uint32_t RED = 0xFF0000; // for down columns
    uint32_t GREEN = 0x00FF00; // for up columns
    uint32_t BLUE = 0x0000FF; // for reversed to up columns
    uint32_t ORANGE = 0xFFA500; // for reversed to down columns
    // NOLINTEND

    // want to show approximate overall change in value (computed from boxes, not actual prices)

    decimal::Decimal first_value = 0;
    decimal::Decimal last_value = 0;

    const auto& first_col = the_chart[0];
    first_value = first_col.GetDirection() == PF_Column::Direction::e_Up ? first_col.GetBottom() : first_col.GetTop();
    // apparently, this can happen

    if (first_value == sv2dec("0.0"))
    {
        first_value = sv2dec("0.01");
    }
    last_value = the_chart.back().GetDirection() == PF_Column::Direction::e_Up ? the_chart.back().GetTop() : the_chart.back().GetBottom();

    decimal::Decimal overall_pct_chg = ((last_value - first_value) / first_value * 100).rescale(-2);

    std::string skipped_columns_text;
    if (skipped_columns > 0)
    {
        skipped_columns_text = std::format(" (last {} cols)", max_columns_for_graph);
    }
    // some explanation for custom box colors.

    std::string explanation_text;
    // if (the_chart.GetReversalboxes() == 1 && the_chart.HasReversedColumns())
    // {
    //     explanation_text = "Orange: 1-step Up then reversal Down. Blue: 1-step Down then reversal Up.";
    // }
    auto chart_title =
        std::format("\n{}{} X {} for {} {}. Overall % change: {}{}\nLast change: {:%a, %b %d, %Y at %I:%M:%S %p %Z}\n{}",
                    the_chart.GetChartBoxSize().format("f"), (the_chart.IsPercent() ? "%" : ""), the_chart.GetReversalboxes(),
                    the_chart.GetSymbol(), (the_chart.IsPercent() ? "percent" : ""), overall_pct_chg.format("f"), skipped_columns_text,
                    std::chrono::zoned_time(std::chrono::current_zone(),
                                            std::chrono::clock_cast<std::chrono::system_clock>(the_chart.GetLastChangeTime())),
                    explanation_text);

    std::unique_ptr<XYChart> c;
    if (streamed_prices.price_.empty())
    {
        c.reset(new XYChart((14 * 72), (14 * 72)));
        c->setPlotArea(50, 100, (14 * 72 - 50), (14 * 72 - 200))->setGridColor(0xc0c0c0, 0xc0c0c0);
    }
    else
    {
        c.reset(new XYChart((14 * 72), (11 * 72)));
        c->setPlotArea(50, 100, (14 * 72 - 50), (11 * 72 - 200))->setGridColor(0xc0c0c0, 0xc0c0c0);

    }

    // Add a title to the chart
    c->addTitle(chart_title.c_str());

    c->addLegend(50, 90, false, "Times New Roman Bold Italic", 12)->setBackground(Chart::Transparent);
    // Set the labels on the x axis and the font to Arial Bold
    c->xAxis()->setLabels(StringArray(x_axis_label_data.data(), x_axis_label_data.size()))->setFontAngle(45.);
    c->xAxis()->setLabelStep(int32_t((columns_in_PF_Chart - skipped_columns) / 40), 0);
    // Set the font for the y axis labels to Arial Bold
    c->yAxis()->setLabelStyle("Arial Bold");

    c->addBoxLayer(DoubleArray(up_data_top.data(), up_data_top.size()), DoubleArray(up_data_bot.data(), up_data_bot.size()), GREEN, "Up");
    c->addBoxLayer(DoubleArray(down_data_top.data(), down_data_top.size()), DoubleArray(down_data_bot.data(), down_data_bot.size()), RED, "Down");

    // reversal layes do not always occur

    if (! reversed_to_up_data_top.empty())
    {
        c->addBoxLayer(DoubleArray(reversed_to_up_data_top.data(), reversed_to_up_data_top.size()), DoubleArray(reversed_to_up_data_bot.data(), reversed_to_up_data_bot.size()), BLUE, "Revse2Up");
    }
    if (! reversed_to_down_data_top.empty())
    {
        c->addBoxLayer(DoubleArray(reversed_to_down_data_top.data(), reversed_to_down_data_top.size()), DoubleArray(reversed_to_down_data_bot.data(), reversed_to_down_data_bot.size()), ORANGE, "Revse2Down");
    }

    // let's show where we started from

    const auto dash_color = c->dashLineColor(RED, Chart::DotLine);
    c->yAxis()->addMark(dec2dbl(first_value), dash_color)->setLineWidth(3);

    // Output the chart
    c->makeChart(output_filename.c_str());

    //free up resources
    // delete c;
    // extract and format the Signals, if any, from the chart. The drawing code can plot
    // them or not.
    // NOTE: we need to zap the column number with the skipped columns offset

    // let's try to do lower level setup for signals drawing here.
    // we want 1 signal per column and that should be the signal with highest priority
    // data is formated as needed by mplfinance library to build lists of marks to overlay
    // on graphic. this saves a bunch of python code

    // const auto& sngls = the_chart.GetSignals();
    // std::vector<double> dt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> db_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> tt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> tb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> btt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> btb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> tt_cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    // std::vector<double> tb_cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    //
    // int had_dt_buy = 0;
    // int had_tt_buy = 0;
    // int had_db_sell = 0;
    // int had_tb_sell = 0;
    // int had_bullish_tt_buy = 0;
    // int had_bearish_tb_sell = 0;
    // int had_catapult_buy = 0;
    // int had_catapult_sell = 0;
    // int had_tt_catapult_buy = 0;
    // int had_tb_catapult_sell = 0;
    //
    // for (const auto& sigs : sngls | vws::filter([skipped_columns](const auto& s) { return s.column_number_ >= skipped_columns; }) |
    //                             vws::chunk_by([](const auto& a, const auto& b) { return a.column_number_ == b.column_number_; }))
    // {
    //     const auto most_important = rng::max_element(sigs, {}, [](const auto& s) { return std::to_underlying(s.priority_); });
    //     switch (most_important->signal_type_)
    //     {
    //         using enum PF_SignalType;
    //         case e_unknown:
    //             break;
    //         case e_double_top_buy:
    //             dt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_dt_buy += 1;
    //             break;
    //         case e_double_bottom_sell:
    //             db_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_db_sell += 1;
    //             break;
    //         case e_triple_top_buy:
    //             tt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_tt_buy += 1;
    //             break;
    //         case e_triple_bottom_sell:
    //             tb_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_tb_sell += 1;
    //             break;
    //         case e_bullish_tt_buy:
    //             btt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_bullish_tt_buy += 1;
    //             break;
    //         case e_bearish_tb_sell:
    //             btb_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_bearish_tb_sell += 1;
    //             break;
    //         case e_catapult_buy:
    //             cat_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_catapult_buy += 1;
    //             break;
    //         case e_catapult_sell:
    //             cat_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_catapult_sell += 1;
    //             break;
    //         case e_ttop_catapult_buy:
    //             tt_cat_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_tt_catapult_buy += 1;
    //             break;
    //         case e_tbottom_catapult_sell:
    //             tb_cat_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
    //             had_tb_catapult_sell += 1;
    //             break;
    //     }
    // }

}
