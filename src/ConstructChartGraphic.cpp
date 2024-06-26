//__ =====================================================================================
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
#include <cmath>
#include <cstddef>
#include <memory>
#include <ranges>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

// #include <pybind11/embed.h>    // everything needed for embedding
// #include <pybind11/gil.h>
// #include <pybind11/stl.h>
#include <spdlog/spdlog.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <chartdir.h>

// namespace py = pybind11;
// using namespace py::literals;

#include "ConstructChartGraphic.h"
#include "PF_Column.h"
#include "PF_Signals.h"

// NOLINTBEGIN
constexpr auto RED = 0xFF0000;     // for down columns
constexpr auto BLACK = 0x000000;   // for down columns
constexpr auto GREEN = 0x008000;   // for up columns
constexpr auto YELLOW = 0xFFFF00;  // for up columns
constexpr auto BLUE = 0x0000FF;    // for reversed to up columns
constexpr auto ORANGE = 0xFFA500;  // for reversed to down columns
constexpr auto LITEGRAY = 0xc0c0c0;

// NOLINTEND

constexpr uint32_t kDpi{72};
constexpr uint32_t kChartWidth{16};
constexpr uint32_t kChartHeight1{14};
constexpr uint32_t kChartHeight2{11};
constexpr uint32_t kChartHeight3{8};
constexpr uint32_t kChartHeight4{19};

// more misc constants to clear up some clang-tidy warnings

constexpr auto k4 = 4;
constexpr auto k10 = 10;
constexpr auto k12 = 12;
constexpr auto k13 = 13;
constexpr auto k40 = 40;
constexpr auto k50 = 50;
constexpr auto k80 = 80;
constexpr auto k100 = 100;
constexpr auto k120 = 120;
constexpr auto k200 = 200;

// define our shapes to be used for each signal type
// no particular thought given to which is used for which

// NOLINTBEGIN
const auto dt_buy_sym = Chart::SquareShape;
const auto tt_buy_sym = Chart::CircleShape;
const auto db_sell_sym = Chart::SquareShape;
const auto tb_sell_sym = Chart::CircleShape;
const auto bullish_tt_buy_sym = Chart::TriangleShape;
const auto bearish_tb_sell_sym = Chart::InvertedTriangleShape;
const auto cat_buy_sym = Chart::CrossShape(0.3);
const auto cat_sell_sym = Chart::CrossShape(0.3);
const auto tt_cat_buy_sym = Chart::ArrowShape();
const auto tb_cat_sell_sym = Chart::ArrowShape(180);
// NOLINTEND

void ConstructCDPFChartGraphicAndWriteToFile(const PF_Chart& the_chart, const fs::path& output_filename,
                                             const StreamedPrices& streamed_prices,
                                             const std::string& /*show_trend_lines*/, X_AxisFormat date_or_time)
{
    BOOST_ASSERT_MSG(
        !the_chart.empty(),
        std::format("Chart for symbol: {} contains no data. Unable to draw graphic.", the_chart.GetSymbol()).c_str());

    const auto columns_in_PF_Chart = the_chart.size();

    const auto max_columns_for_graph = the_chart.GetMaxGraphicColumns();
    size_t skipped_columns = max_columns_for_graph < 1 || columns_in_PF_Chart <= max_columns_for_graph
                                 ? 0
                                 : columns_in_PF_Chart - max_columns_for_graph;

    // std::cout << std::format("cols in chart: {}, max_cols: {}, skipped: {}\n", columns_in_PF_Chart,
    // max_columns_for_graph, skipped_columns); return;

    // for our chart graphic there are 4 types of columns: up, down, reversed-to-up and reversed-to-down.
    // there will be a separate layer for each of these types so a different color can be assigned to each.
    // in order for eveything to line up correctly each layer must contain all the points whether they are
    // used in that layor or not.  'NoValue' values are used. So, fill each layer with 'NoValue' then
    // over-write the values that are used in that layer.

    PF_Chart::ColumnTopBottomList up_layer{
        columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};
    PF_Chart::ColumnTopBottomList down_layer{
        columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};
    PF_Chart::ColumnTopBottomList reversed_to_up_layer{
        columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};
    PF_Chart::ColumnTopBottomList reversed_to_down_layer{
        columns_in_PF_Chart, PF_Chart::ColumnTopBottomInfo{.col_top_ = Chart::NoValue, .col_bot_ = Chart::NoValue}};

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

    rng::for_each(up_cols, [&up_layer](const auto& col_info) { up_layer[col_info.col_nbr_] = col_info; });
    rng::for_each(down_cols, [&down_layer](const auto& col_info) { down_layer[col_info.col_nbr_] = col_info; });
    if (!rev_to_up_cols.empty())
    {
        rng::for_each(rev_to_up_cols, [&reversed_to_up_layer](const auto& col_info)
                      { reversed_to_up_layer[col_info.col_nbr_] = col_info; });
    }
    if (!rev_to_down_cols.empty())
    {
        rng::for_each(rev_to_down_cols, [&reversed_to_down_layer](const auto& col_info)
                      { reversed_to_down_layer[col_info.col_nbr_] = col_info; });
    }

    // now, make our data arrays for the graphic software
    // but first, limit the number of columns shown on graphic if requested

    std::vector<double> up_data_top;
    up_data_top.reserve(columns_in_PF_Chart - skipped_columns);
    std::vector<double> up_data_bot;
    up_data_bot.reserve(columns_in_PF_Chart - skipped_columns);

    rng::for_each(up_layer | vws::drop(skipped_columns),
                  [&up_data_top, &up_data_bot](const auto& col_info)
                  {
                      up_data_top.push_back(col_info.col_top_);
                      up_data_bot.push_back(col_info.col_bot_);
                  });

    std::vector<double> down_data_top;
    down_data_top.reserve(columns_in_PF_Chart - skipped_columns);
    std::vector<double> down_data_bot;
    down_data_bot.reserve(columns_in_PF_Chart - skipped_columns);

    rng::for_each(down_layer | vws::drop(skipped_columns),
                  [&down_data_top, &down_data_bot](const auto& col_info)
                  {
                      down_data_top.push_back(col_info.col_top_);
                      down_data_bot.push_back(col_info.col_bot_);
                  });

    std::vector<double> reversed_to_up_data_top;
    std::vector<double> reversed_to_up_data_bot;
    if (!rev_to_up_cols.empty())
    {
        reversed_to_up_data_top.reserve(columns_in_PF_Chart - skipped_columns);
        reversed_to_up_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
        rng::for_each(reversed_to_up_layer | vws::drop(skipped_columns),
                      [&reversed_to_up_data_top, &reversed_to_up_data_bot](const auto& col_info)
                      {
                          reversed_to_up_data_top.push_back(col_info.col_top_);
                          reversed_to_up_data_bot.push_back(col_info.col_bot_);
                      });
    }

    std::vector<double> reversed_to_down_data_top;
    std::vector<double> reversed_to_down_data_bot;
    if (!rev_to_down_cols.empty())
    {
        reversed_to_down_data_top.reserve(columns_in_PF_Chart - skipped_columns);
        reversed_to_down_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
        rng::for_each(reversed_to_down_layer | vws::drop(skipped_columns),
                      [&reversed_to_down_data_top, &reversed_to_down_data_bot](const auto& col_info)
                      {
                          reversed_to_down_data_top.push_back(col_info.col_top_);
                          reversed_to_down_data_bot.push_back(col_info.col_bot_);
                      });
    }

    // want to show approximate overall change in value (computed from boxes, not actual prices)

    const auto& first_col = the_chart[0];
    decimal::Decimal first_value =
        first_col.GetDirection() == PF_Column::Direction::e_Up ? first_col.GetBottom() : first_col.GetTop();
    // apparently, this can happen

    if (first_value == sv2dec("0.0"))
    {
        first_value = sv2dec("0.01");
    }
    decimal::Decimal last_value = the_chart.back().GetDirection() == PF_Column::Direction::e_Up
                                      ? the_chart.back().GetTop()
                                      : the_chart.back().GetBottom();

    decimal::Decimal overall_pct_chg = ((last_value - first_value) / first_value * k100).rescale(-2);

    std::string skipped_columns_text;
    if (skipped_columns > 0)
    {
        skipped_columns_text = std::format(" (last {} cols)", max_columns_for_graph);
    }

    auto chart_title = std::format(
        "\n{}{} X {} for {} {}. Overall % change: {}{}\nLast change: {:%a, %b %d, %Y at %I:%M:%S %p %Z}\n",
        the_chart.GetChartBoxSize().format("f"), (the_chart.IsPercent() ? "%" : ""), the_chart.GetReversalboxes(),
        the_chart.GetSymbol(), (the_chart.IsPercent() ? "percent" : ""), overall_pct_chg.format("f"),
        skipped_columns_text,
        std::chrono::zoned_time(std::chrono::current_zone(),
                                std::chrono::clock_cast<std::chrono::system_clock>(the_chart.GetLastChangeTime())));

    // for x-axis label, we use the begin date for each column
    // the chart software wants an array of const char*.
    // this will take several steps.

    std::vector<std::string> x_axis_labels;
    x_axis_labels.reserve(columns_in_PF_Chart - skipped_columns);

    rng::for_each(the_chart | vws::drop(skipped_columns),
                  [&x_axis_labels, &date_or_time](const auto& col)
                  {
                      if (date_or_time == X_AxisFormat::e_show_date)
                      {
                          x_axis_labels.emplace_back(std::format("{:%F}", col.GetTimeSpan().first));
                      }
                      else
                      {
                          x_axis_labels.emplace_back(UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first));
                      }
                  });

    std::vector<const char*> x_axis_label_data;
    x_axis_label_data.reserve(columns_in_PF_Chart - skipped_columns);
    rng::for_each(x_axis_labels,
                  [&x_axis_label_data](const auto& label) { x_axis_label_data.push_back(label.c_str()); });

    std::unique_ptr<XYChart> c;
    if (streamed_prices.price_.empty())
    {
        c = std::make_unique<XYChart>((kChartWidth * kDpi), (kChartHeight1 * kDpi));
        c->setPlotArea(k50, k100, (kChartWidth * kDpi - k120), (kChartHeight1 * kDpi - k200))
            ->setGridColor(LITEGRAY, LITEGRAY);
    }
    else
    {
        c = std::make_unique<XYChart>((kChartWidth * kDpi), (kChartHeight2 * kDpi));
        c->setPlotArea(k50, k100, (kChartWidth * kDpi - k120), (kChartHeight2 * kDpi - k200))
            ->setGridColor(LITEGRAY, LITEGRAY);
    }

    if (!c)
    {
        throw std::runtime_error("Unable to create PF_chart graphic.");
    }

    c->addTitle(chart_title.c_str());

    c->addLegend(k50, 90, false, "Times New Roman Bold Italic", k12)->setBackground(Chart::Transparent);

    c->xAxis()->setLabels(StringArray(x_axis_label_data.data(), x_axis_label_data.size()))->setFontAngle(45.);

    c->xAxis()->setLabelStep((columns_in_PF_Chart - skipped_columns) / 40, 0);

    c->yAxis()->setLabelStyle("Arial Bold");
    if (the_chart.IsPercent())
    {
        c->yAxis()->setLogScale(std::max(0.0, dec2dbl(the_chart.GetYLimits().first - k10)),
                                std::min(dec2dbl(the_chart.GetYLimits().second + k10), 10000.0));
    }
    else
    {
        // c->yAxis()->setLinearScale(
        //     std::max(0.0, dec2dbl(the_chart.GetYLimits().first - k10)),
        //     std::min(dec2dbl(the_chart.GetYLimits().second + the_chart.GetChartBoxSize()) + k10, 10000.0));
    }

    c->yAxis2()->copyAxis(c->yAxis());

    // now we can add our data for the columns.  Each column type in its own layer.

    c->addBoxLayer(DoubleArray(up_data_top.data(), up_data_top.size()),
                   DoubleArray(up_data_bot.data(), up_data_bot.size()), GREEN, "Up");
    c->addBoxLayer(DoubleArray(down_data_top.data(), down_data_top.size()),
                   DoubleArray(down_data_bot.data(), down_data_bot.size()), RED, "Down");

    // reversal layes do not always occur

    if (!reversed_to_up_data_top.empty())
    {
        c->addBoxLayer(DoubleArray(reversed_to_up_data_top.data(), reversed_to_up_data_top.size()),
                       DoubleArray(reversed_to_up_data_bot.data(), reversed_to_up_data_bot.size()), BLUE, "Revse2Up");
    }
    if (!reversed_to_down_data_top.empty())
    {
        c->addBoxLayer(DoubleArray(reversed_to_down_data_top.data(), reversed_to_down_data_top.size()),
                       DoubleArray(reversed_to_down_data_bot.data(), reversed_to_down_data_bot.size()), ORANGE,
                       "Revse2Down");
    }

    Signals_1 data_for_PF;

    ConstructCDPFChartGraphicAddPFSignals(the_chart, data_for_PF, skipped_columns, c);

    // let's show where we started from

    const auto dash_color = c->dashLineColor(RED, Chart::DashLine);
    c->yAxis()->addMark(dec2dbl(first_value), dash_color)->setLineWidth(3);

    // add a point to keep our mark line in view

    double mark_marker = dec2dbl(first_value);
    double mark_col = 0.0;
    c->addScatterLayer(DoubleArray(&mark_col, 1), DoubleArray(&mark_marker, 1));

    Signals_2 data_for_prices;

    std::unique_ptr<XYChart> p;

    // allocate store for prices chart labels.

    std::vector<std::string> p_x_axis_labels;
    std::vector<const char*> p_x_axis_label_data;

    if (!streamed_prices.price_.empty())
    {
        // We get a LOT of data from Eodhd so we need to limit how much we show on the graphic
        // for transaction data.
        // For now, just limit the max number of points to the most recent 'n' where
        // 'n' = the width of the chart.

        const auto max_price_cols = static_cast<size_t>(kChartWidth * kDpi - k120 - k50) * 2;
        size_t skipped_price_cols = 0;
        if (streamed_prices.timestamp_seconds_.size() > max_price_cols)
        {
            skipped_price_cols = streamed_prices.timestamp_seconds_.size() - max_price_cols;
        }
        // for x-axis label, we use the time in nanoseconds from the
        // streamed data.
        // the chart software wants an array of const char*.
        // this will take several steps.

        p_x_axis_labels.reserve(streamed_prices.timestamp_seconds_.size() - skipped_price_cols);

        rng::for_each(
            streamed_prices.timestamp_seconds_ | vws::drop(skipped_price_cols),
            [&p_x_axis_labels, &date_or_time](const auto& secs)
            {
                if (date_or_time == X_AxisFormat::e_show_date)
                {
                    p_x_axis_labels.emplace_back(std::format("{:%F}", PF_Column::TmPt{std::chrono::seconds{secs}}));
                }
                else
                {
                    p_x_axis_labels.emplace_back(
                        UTCTimePointToLocalTZHMSString(PF_Column::TmPt{std::chrono::seconds{secs}}));
                }
            });

        p_x_axis_label_data.reserve(p_x_axis_labels.size());
        rng::for_each(p_x_axis_labels,
                      [&p_x_axis_label_data](const auto& label) { p_x_axis_label_data.push_back(label.c_str()); });

        p = std::make_unique<XYChart>((kChartWidth * kDpi), (kChartHeight3 * kDpi));
        if (!p)
        {
            throw std::runtime_error("Unable to create streamed prices graphic.");
        }

        p->setPlotArea(k50, k50, (kChartWidth * kDpi - k120), (kChartHeight3 * kDpi - k200))
            ->setGridColor(LITEGRAY, LITEGRAY);

        p->addTitle(std::format("Price data for {} {}", the_chart.GetSymbol(),
                                (skipped_price_cols == 0 ? "" : std::format("Showing last {} cols", max_price_cols)))
                        .c_str());

        p->addLegend(k50, 40, false, "Times New Roman Bold Italic", 12)->setBackground(Chart::Transparent);

        // trim our data by bumping pointer and adjusting counter

        p->addLineLayer(DoubleArray(streamed_prices.price_.data() + skipped_price_cols,
                                    streamed_prices.price_.size() - skipped_price_cols),
                        RED);

        p->xAxis()->setLabels(StringArray(p_x_axis_label_data.data(), p_x_axis_label_data.size()))->setFontAngle(45.);

        p->xAxis()->setLabelStep(k40, 0);
        p->yAxis()->setLabelStyle("Arial Bold");

        p->yAxis2()->copyAxis(p->yAxis());

        // let's show where we started from

        const auto dash_color = p->dashLineColor(RED, Chart::DashLine);
        p->yAxis()->addMark(dec2dbl(first_value), dash_color)->setLineWidth(2);
        // add a point to keep our mark line in view

        double mark_marker = dec2dbl(first_value);
        double mark_col = 0.0;
        p->addScatterLayer(DoubleArray(&mark_col, 1), DoubleArray(&mark_marker, 1));

        // next, add our signals to this graphic.
        // Each signal type will be a separate layer so it can have its own glyph and color.

        ConstructCDPricesGraphicAddSignals(the_chart, data_for_prices, skipped_price_cols, streamed_prices, p);
    }

    if (!p)
    {
        c->makeChart(output_filename.c_str());
        return;
    }

    // we have a dual chart setup
    // Output the chart

    std::unique_ptr<MultiChart> m = std::make_unique<MultiChart>((kChartWidth * kDpi), (kChartHeight4 * kDpi));
    m->addChart(0, 0, c.get());
    m->addChart(0, (kChartHeight2 * kDpi), p.get());

    m->makeChart(output_filename.c_str());
}

void ConstructCDPFChartGraphicAddPFSignals(const PF_Chart& the_chart, Signals_1& data_arrays, size_t skipped_columns,
                                           std::unique_ptr<XYChart>& the_graphic)
{
    // we may need to drop some signals.

    auto filter_skipped_cols = the_chart.GetSignals() | vws::filter([skipped_columns](const auto& sig)
                                                                    { return sig.column_number_ >= skipped_columns; });

    for (const auto& sig : filter_skipped_cols)
    {
        switch (sig.signal_type_)
        {
            using enum PF_SignalType;
            case e_double_top_buy:
                data_arrays.dt_buys_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.dt_buys_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_double_bottom_sell:
                data_arrays.db_sells_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.db_sells_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_triple_top_buy:
                data_arrays.tt_buys_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.tt_buys_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_triple_bottom_sell:
                data_arrays.tb_sells_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.tb_sells_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_bullish_tt_buy:
                data_arrays.bullish_tt_buys_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.bullish_tt_buys_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_bearish_tb_sell:
                data_arrays.bearish_tb_sells_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.bearish_tb_sells_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_catapult_buy:
                data_arrays.cat_buys_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.cat_buys_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_catapult_sell:
                data_arrays.cat_sells_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.cat_sells_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_ttop_catapult_buy:
                data_arrays.tt_cat_buys_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.tt_cat_buys_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_tbottom_catapult_sell:
                data_arrays.tb_cat_sells_price_.emplace_back(dec2dbl(sig.signal_price_));
                data_arrays.tb_cat_sells_x_.emplace_back(sig.column_number_ - skipped_columns);
                break;
            case e_unknown:
            default:
                break;
        }
    }

    // now we can add layers (if any) with signals

    if (!data_arrays.dt_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.dt_buys_x_.data(), data_arrays.dt_buys_x_.size()),
            DoubleArray(data_arrays.dt_buys_price_.data(), data_arrays.dt_buys_price_.size()), "dt buy", dt_buy_sym,
            k10, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.tt_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tt_buys_x_.data(), data_arrays.tt_buys_x_.size()),
            DoubleArray(data_arrays.tt_buys_price_.data(), data_arrays.tt_buys_price_.size()), "tt buy", tt_buy_sym,
            k10, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.db_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.db_sells_x_.data(), data_arrays.db_sells_x_.size()),
            DoubleArray(data_arrays.db_sells_price_.data(), data_arrays.db_sells_price_.size()), "db sell", db_sell_sym,
            k10, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.tb_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tb_sells_x_.data(), data_arrays.tb_sells_x_.size()),
            DoubleArray(data_arrays.tb_sells_price_.data(), data_arrays.tb_sells_price_.size()), "tb sell", tb_sell_sym,
            k10, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.bullish_tt_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.bullish_tt_buys_x_.data(), data_arrays.bullish_tt_buys_x_.size()),
            DoubleArray(data_arrays.bullish_tt_buys_price_.data(), data_arrays.bullish_tt_buys_price_.size()),
            "bullish tt buy", bullish_tt_buy_sym, k10, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.bearish_tb_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.bearish_tb_sells_x_.data(), data_arrays.bearish_tb_sells_x_.size()),
            DoubleArray(data_arrays.bearish_tb_sells_price_.data(), data_arrays.bearish_tb_sells_price_.size()),
            "bearish tb sell", bearish_tb_sell_sym, k10, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.cat_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.cat_buys_x_.data(), data_arrays.cat_buys_x_.size()),
            DoubleArray(data_arrays.cat_buys_price_.data(), data_arrays.cat_buys_price_.size()), "cat buy", cat_buy_sym,
            k10, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.cat_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.cat_sells_x_.data(), data_arrays.cat_sells_x_.size()),
            DoubleArray(data_arrays.cat_sells_price_.data(), data_arrays.cat_sells_price_.size()), "cat sell",
            cat_sell_sym, k10, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.tt_cat_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tt_cat_buys_x_.data(), data_arrays.tt_cat_buys_x_.size()),
            DoubleArray(data_arrays.tt_cat_buys_price_.data(), data_arrays.tt_cat_buys_price_.size()), "tt cat buy",
            tt_cat_buy_sym, k10, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.tb_cat_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tb_cat_sells_x_.data(), data_arrays.tb_cat_sells_x_.size()),
            DoubleArray(data_arrays.tb_cat_sells_price_.data(), data_arrays.tb_cat_sells_price_.size()), "tb cat sell",
            tb_cat_sell_sym, k10, BLACK);
        the_layer->moveFront();
    }
}

void ConstructCDPricesGraphicAddSignals(const PF_Chart& the_chart, Signals_2& data_arrays, size_t skipped_price_cols,
                                        const StreamedPrices& streamed_prices, std::unique_ptr<XYChart>& the_graphic)
{
    for (int32_t ndx = 0 + skipped_price_cols; ndx < streamed_prices.signal_type_.size(); ++ndx)
    {
        switch (streamed_prices.signal_type_[ndx])
        {
            using enum PF_SignalType;
            case std::to_underlying(e_double_top_buy):
                data_arrays.dt_buys_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.dt_buys_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_double_bottom_sell):
                data_arrays.db_sells_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.db_sells_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_triple_top_buy):
                data_arrays.tt_buys_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.tt_buys_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_triple_bottom_sell):
                data_arrays.tb_sells_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.tb_sells_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_bullish_tt_buy):
                data_arrays.bullish_tt_buys_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.bullish_tt_buys_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_bearish_tb_sell):
                data_arrays.bearish_tb_sells_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.bearish_tb_sells_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_catapult_buy):
                data_arrays.cat_buys_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.cat_buys_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_catapult_sell):
                data_arrays.cat_sells_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.cat_sells_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_ttop_catapult_buy):
                data_arrays.tt_cat_buys_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.tt_cat_buys_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_tbottom_catapult_sell):
                data_arrays.tb_cat_sells_price_.emplace_back(streamed_prices.price_[ndx]);
                data_arrays.tb_cat_sells_x_.emplace_back(ndx - skipped_price_cols);
                break;
            case std::to_underlying(e_unknown):
            default:
                break;
        }
    }

    // now we can add layers (if any) with signals

    if (!data_arrays.dt_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.dt_buys_x_.data(), data_arrays.dt_buys_x_.size()),
            DoubleArray(data_arrays.dt_buys_price_.data(), data_arrays.dt_buys_price_.size()), "dt buy", dt_buy_sym,
            k13, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.tt_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tt_buys_x_.data(), data_arrays.tt_buys_x_.size()),
            DoubleArray(data_arrays.tt_buys_price_.data(), data_arrays.tt_buys_price_.size()), "tt buy", tt_buy_sym,
            k13, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.db_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.db_sells_x_.data(), data_arrays.db_sells_x_.size()),
            DoubleArray(data_arrays.db_sells_price_.data(), data_arrays.db_sells_price_.size()), "db sell", db_sell_sym,
            k13, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.tb_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tb_sells_x_.data(), data_arrays.tb_sells_x_.size()),
            DoubleArray(data_arrays.tb_sells_price_.data(), data_arrays.tb_sells_price_.size()), "tb sell", tb_sell_sym,
            k13, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.bullish_tt_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.bullish_tt_buys_x_.data(), data_arrays.bullish_tt_buys_x_.size()),
            DoubleArray(data_arrays.bullish_tt_buys_price_.data(), data_arrays.bullish_tt_buys_price_.size()),
            "bullish tt buy", bullish_tt_buy_sym, k13, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.bearish_tb_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.bearish_tb_sells_x_.data(), data_arrays.bearish_tb_sells_x_.size()),
            DoubleArray(data_arrays.bearish_tb_sells_price_.data(), data_arrays.bearish_tb_sells_price_.size()),
            "bearish tb sell", bearish_tb_sell_sym, k13, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.cat_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.cat_buys_x_.data(), data_arrays.cat_buys_x_.size()),
            DoubleArray(data_arrays.cat_buys_price_.data(), data_arrays.cat_buys_price_.size()), "cat buy", cat_buy_sym,
            k13, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.cat_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.cat_sells_x_.data(), data_arrays.cat_sells_x_.size()),
            DoubleArray(data_arrays.cat_sells_price_.data(), data_arrays.cat_sells_price_.size()), "cat sell",
            cat_sell_sym, k13, BLACK);
        the_layer->moveFront();
    }

    if (!data_arrays.tt_cat_buys_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tt_cat_buys_x_.data(), data_arrays.tt_cat_buys_x_.size()),
            DoubleArray(data_arrays.tt_cat_buys_price_.data(), data_arrays.tt_cat_buys_price_.size()), "tt cat buy",
            tt_cat_buy_sym, k13, YELLOW);
        the_layer->moveFront();
    }

    if (!data_arrays.tb_cat_sells_price_.empty())
    {
        auto* the_layer = the_graphic->addScatterLayer(
            DoubleArray(data_arrays.tb_cat_sells_x_.data(), data_arrays.tb_cat_sells_x_.size()),
            DoubleArray(data_arrays.tb_cat_sells_price_.data(), data_arrays.tb_cat_sells_price_.size()), "tb cat sell",
            tb_cat_sell_sym, k13, BLACK);
        the_layer->moveFront();
    }
}

void ConstructCDSummaryGraphic(const PF_StreamedSummary& streamed_summary, const fs::path& output_filename)
{
    // simple floating bar graphic which shows overall price movement for each symbol

    const auto columns_in_PF_SummaryChart = streamed_summary.size();

    // this chart will be a simple bar chart showing percent change for each ticker.
    // base value is prior day's close.

    std::vector<double> deltas{};
    deltas.reserve(streamed_summary.size());

    std::vector<std::string> x_axis_labels;
    x_axis_labels.reserve(streamed_summary.size());

    for (const auto& [symbol, data] : streamed_summary)
    {
        auto delta = (data.latest_price_ - data.opening_price_) / data.opening_price_ * 100.;
        deltas.push_back(delta);

        x_axis_labels.push_back(symbol);
    }

    std::vector<const char*> x_axis_label_data;
    x_axis_label_data.reserve(streamed_summary.size());

    rng::for_each(x_axis_labels,
                  [&x_axis_label_data](const auto& label) { x_axis_label_data.push_back(label.c_str()); });

    std::unique_ptr<XYChart> c = std::make_unique<XYChart>((kChartWidth * kDpi), (kChartHeight1 * kDpi));

    c->setPlotArea(k50, k100, (kChartWidth * kDpi - k120), (kChartHeight1 * kDpi - k200))
        ->setGridColor(LITEGRAY, LITEGRAY);

    c->addTitle("\nShowing percent change for streamed tickers.\n(relative to previous day's close)");

    BarLayer* layer = c->addBarLayer(Chart::Overlay);

    // Select positive data and add it as data set with green color
    layer->addDataSet(ArrayMath(DoubleArray(deltas.data(), deltas.size())).selectGEZ(DoubleArray(), Chart::NoValue),
                      GREEN);

    // Select negative data and add it as data set with red color
    layer->addDataSet(ArrayMath(DoubleArray(deltas.data(), deltas.size())).selectLTZ(DoubleArray(), Chart::NoValue),
                      RED);

    c->xAxis()->setLabels(StringArray(x_axis_label_data.data(), x_axis_label_data.size()))->setFontAngle(45.);

    // c->xAxis()->setLabelStep((columns_in_PF_Chart - skipped_columns) / 40, 0);

    c->yAxis()->setLabelStyle("Arial Bold");
    c->yAxis()->setTickWidth(3, 1);
    c->yAxis2()->copyAxis(c->yAxis());
    c->yAxis2()->setTickWidth(3, 1);

    c->makeChart(output_filename.c_str());
}
