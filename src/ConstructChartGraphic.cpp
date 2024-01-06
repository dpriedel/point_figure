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
#include "PF_Column.h"
#include "PF_Signals.h"

// NOLINTBEGIN
constexpr auto RED = 0xFF0000;     // for down columns
constexpr auto GREEN = 0x00FF00;   // for up columns
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

constexpr auto k10 = 10;
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
                                             const std::string& /*show_trend_lines*/,
                                             PF_Chart::X_AxisFormat date_or_time)
{
    BOOST_ASSERT_MSG(
        !the_chart.empty(),
        std::format("Chart for symbol: {} contains no data. Unable to draw graphic.", the_chart.GetSymbol()).c_str());

    const auto columns_in_PF_Chart = the_chart.size();

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

    rng::for_each(up_cols, [&up_layer](const auto& col) { up_layer[col.col_nbr_] = col; });
    rng::for_each(down_cols, [&down_layer](const auto& col) { down_layer[col.col_nbr_] = col; });
    if (!rev_to_up_cols.empty())
    {
        rng::for_each(rev_to_up_cols,
                      [&reversed_to_up_layer](const auto& col) { reversed_to_up_layer[col.col_nbr_] = col; });
    }
    if (!rev_to_down_cols.empty())
    {
        rng::for_each(rev_to_down_cols,
                      [&reversed_to_down_layer](const auto& col) { reversed_to_down_layer[col.col_nbr_] = col; });
    }

    // now, make our data arrays for the graphic software
    // but first, limit the number of columns shown on graphic if requested

    const auto max_columns_for_graph = the_chart.GetMaxGraphicColumns();
    size_t skipped_columns = max_columns_for_graph < 1 || the_chart.size() <= max_columns_for_graph
                                 ? 0
                                 : the_chart.size() - max_columns_for_graph;

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
    if (!rev_to_up_cols.empty())
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
    if (!rev_to_down_cols.empty())
    {
        reversed_to_down_data_top.reserve(columns_in_PF_Chart - skipped_columns);
        reversed_to_down_data_bot.reserve(columns_in_PF_Chart - skipped_columns);
        for (const auto& col : reversed_to_down_layer | vws::drop(skipped_columns))
        {
            reversed_to_down_data_top.push_back(col.col_top_);
            reversed_to_down_data_bot.push_back(col.col_bot_);
        }
    }

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
    last_value = the_chart.back().GetDirection() == PF_Column::Direction::e_Up ? the_chart.back().GetTop()
                                                                               : the_chart.back().GetBottom();

    decimal::Decimal overall_pct_chg = ((last_value - first_value) / first_value * k100).rescale(-2);

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
    auto chart_title = std::format(
        "\n{}{} X {} for {} {}. Overall % change: {}{}\nLast change: {:%a, %b %d, %Y at %I:%M:%S %p %Z}\n{}",
        the_chart.GetChartBoxSize().format("f"), (the_chart.IsPercent() ? "%" : ""), the_chart.GetReversalboxes(),
        the_chart.GetSymbol(), (the_chart.IsPercent() ? "percent" : ""), overall_pct_chg.format("f"),
        skipped_columns_text,
        std::chrono::zoned_time(std::chrono::current_zone(),
                                std::chrono::clock_cast<std::chrono::system_clock>(the_chart.GetLastChangeTime())),
        explanation_text);

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

    std::vector<const char*> x_axis_label_data;
    x_axis_label_data.reserve(columns_in_PF_Chart - skipped_columns);
    rng::for_each(x_axis_labels | vws::drop(skipped_columns),
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

    c->addTitle(chart_title.c_str());

    c->addLegend(k50, 90, false, "Times New Roman Bold Italic", 12)->setBackground(Chart::Transparent);

    c->xAxis()->setLabels(StringArray(x_axis_label_data.data(), x_axis_label_data.size()))->setFontAngle(45.);
    c->xAxis()->setLabelStep(static_cast<int32_t>((columns_in_PF_Chart - skipped_columns) / k40), 0);
    c->yAxis()->setLabelStyle("Arial Bold");
    if (the_chart.IsPercent())
    {
        c->yAxis()->setLogScale(std::min(0.0, dec2dbl(the_chart.GetYLimits().first - 10)),
                                std::min(dec2dbl(the_chart.GetYLimits().second + 10), 1000.0));
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

    c = ConstructCDPFChartGraphicAddPFSignals(the_chart, skipped_columns, std::move(c));
    // let's show where we started from

    const auto dash_color = c->dashLineColor(RED, Chart::DashLine);
    c->yAxis()->addMark(dec2dbl(first_value), dash_color)->setLineWidth(3);

    const auto prices_graphic = ConstructCDPFChartPricesGraphic(the_chart, streamed_prices, date_or_time);

    if (!prices_graphic)
    {
        c->makeChart(output_filename.c_str());
        return;
    }

    // we have a dual chart setup
    // Output the chart

    std::unique_ptr<MultiChart> m = std::make_unique<MultiChart>((kChartWidth * kDpi), (kChartHeight4 * kDpi));
    m->addChart(0, 0, c.get());
    m->addChart(0, (kChartHeight2 * kDpi), prices_graphic.get());

    m->makeChart(output_filename.c_str());
}

std::unique_ptr<XYChart> ConstructCDPFChartGraphicAddPFSignals(const PF_Chart& the_chart, size_t skipped_columns,
                                                               std::unique_ptr<XYChart> the_graphic)
{
    std::vector<double> dt_buys_price;
    std::vector<double> tt_buys_price;
    std::vector<double> db_sells_price;
    std::vector<double> tb_sells_price;
    std::vector<double> bullish_tt_buys_price;
    std::vector<double> bearish_tb_sells_price;
    std::vector<double> cat_buys_price;
    std::vector<double> cat_sells_price;
    std::vector<double> tt_cat_buys_price;
    std::vector<double> tb_cat_sells_price;

    std::vector<double> dt_buys_x;
    std::vector<double> tt_buys_x;
    std::vector<double> db_sells_x;
    std::vector<double> tb_sells_x;
    std::vector<double> bullish_tt_buys_x;
    std::vector<double> bearish_tb_sells_x;
    std::vector<double> cat_buys_x;
    std::vector<double> cat_sells_x;
    std::vector<double> tt_cat_buys_x;
    std::vector<double> tb_cat_sells_x;

    for (const auto& sig : the_chart.GetSignals())
    {
        switch (sig.signal_type_)
        {
            using enum PF_SignalType;
            case e_double_top_buy:
                dt_buys_price.push_back(dec2dbl(sig.signal_price_));
                dt_buys_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_double_bottom_sell:
                db_sells_price.push_back(dec2dbl(sig.signal_price_));
                db_sells_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_triple_top_buy:
                tt_buys_price.push_back(dec2dbl(sig.signal_price_));
                tt_buys_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_triple_bottom_sell:
                tb_sells_price.push_back(dec2dbl(sig.signal_price_));
                tb_sells_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_bullish_tt_buy:
                bullish_tt_buys_price.push_back(dec2dbl(sig.signal_price_));
                bullish_tt_buys_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_bearish_tb_sell:
                bearish_tb_sells_price.push_back(dec2dbl(sig.signal_price_));
                bearish_tb_sells_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_catapult_buy:
                cat_buys_price.push_back(dec2dbl(sig.signal_price_));
                cat_buys_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_catapult_sell:
                cat_sells_price.push_back(dec2dbl(sig.signal_price_));
                cat_sells_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_ttop_catapult_buy:
                tt_cat_buys_price.push_back(dec2dbl(sig.signal_price_));
                tt_cat_buys_x.push_back(dec2dbl(sig.column_number_ - skipped_columns));
                break;
            case e_tbottom_catapult_sell:
                tb_cat_sells_price.push_back(dec2dbl(sig.signal_price_));
                tb_cat_sells_x.push_back(sig.column_number_ - skipped_columns);
                break;
            case e_unknown:
            default:
                break;
        }
    }

    // now we can add layers (if any) with signals

    if (!dt_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(dt_buys_x.data(), dt_buys_x.size()),
                                     DoubleArray(dt_buys_price.data(), dt_buys_price.size()), "dt buy", dt_buy_sym, k10,
                                     GREEN);
    }

    if (!tt_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tt_buys_x.data(), tt_buys_x.size()),
                                     DoubleArray(tt_buys_price.data(), tt_buys_price.size()), "tt buy", tt_buy_sym, k10,
                                     GREEN);
    }

    if (!db_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(db_sells_x.data(), db_sells_x.size()),
                                     DoubleArray(db_sells_price.data(), db_sells_price.size()), "db sell", db_sell_sym,
                                     k10, RED);
    }

    if (!tb_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tb_sells_x.data(), tb_sells_x.size()),
                                     DoubleArray(tb_sells_price.data(), dt_buys_price.size()), "tb sell", tb_sell_sym,
                                     k10, RED);
    }

    if (!bullish_tt_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(bullish_tt_buys_x.data(), bullish_tt_buys_x.size()),
                                     DoubleArray(bullish_tt_buys_price.data(), bullish_tt_buys_price.size()),
                                     "bullish tt buy", bullish_tt_buy_sym, k10, GREEN);
    }

    if (!bearish_tb_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(bearish_tb_sells_x.data(), bearish_tb_sells_x.size()),
                                     DoubleArray(bearish_tb_sells_price.data(), bearish_tb_sells_price.size()),
                                     "bearish tb sell", bearish_tb_sell_sym, k10, RED);
    }

    if (!cat_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(cat_buys_x.data(), cat_buys_x.size()),
                                     DoubleArray(cat_buys_price.data(), cat_buys_price.size()), "cat buy", cat_buy_sym,
                                     k10, GREEN);
    }

    if (!cat_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(cat_sells_x.data(), cat_sells_x.size()),
                                     DoubleArray(cat_sells_price.data(), cat_sells_price.size()), "cat sell",
                                     cat_sell_sym, k10, RED);
    }

    if (!tt_cat_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tt_cat_buys_x.data(), tt_cat_buys_x.size()),
                                     DoubleArray(tt_cat_buys_price.data(), tt_cat_buys_price.size()), "tt cat buy",
                                     tt_cat_buy_sym, k10, GREEN);
    }

    if (!tb_cat_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tb_cat_sells_x.data(), tb_cat_sells_x.size()),
                                     DoubleArray(tb_cat_sells_price.data(), tb_cat_sells_price.size()), "tb cat sell",
                                     tb_cat_sell_sym, k10, RED);
    }

    return the_graphic;
}

std::unique_ptr<XYChart> ConstructCDPFChartPricesGraphic(const PF_Chart& the_chart,
                                                         const StreamedPrices& streamed_prices,
                                                         PF_Chart::X_AxisFormat date_or_time)
{
    // if we have price data, then make a separate chart for it.

    if (streamed_prices.price_.empty())
    {
        return {};
    }

    // for x-axis label, we use the time in nanoseconds from the
    // streamed data.
    // the chart software wants an array of const char*.
    // this will take several steps.

    std::vector<std::string> x_axis_labels;
    x_axis_labels.reserve(streamed_prices.timestamp_.size());

    for (const auto& nanosecs : streamed_prices.timestamp_)
    {
        if (date_or_time == PF_Chart::X_AxisFormat::e_show_date)
        {
            x_axis_labels.push_back(std::format("{:%F}", PF_Column::TmPt{std::chrono::nanoseconds{nanosecs}}));
        }
        else
        {
            x_axis_labels.push_back(
                UTCTimePointToLocalTZHMSString(PF_Column::TmPt{std::chrono::nanoseconds{nanosecs}}));
        }
    }

    std::vector<const char*> x_axis_label_data;
    x_axis_label_data.reserve(streamed_prices.timestamp_.size());
    rng::for_each(x_axis_labels,
                  [&x_axis_label_data](const auto& label) { x_axis_label_data.push_back(label.c_str()); });

    std::unique_ptr<XYChart> p = std::make_unique<XYChart>((kChartWidth * kDpi), (kChartHeight3 * kDpi));
    p->setPlotArea(k50, k50, (kChartWidth * kDpi - k80), (kChartHeight3 * kDpi - k200))
        ->setGridColor(LITEGRAY, LITEGRAY);

    p->addTitle("Price data for PF_Chart");

    p->addLegend(k50, 40, false, "Times New Roman Bold Italic", 12)->setBackground(Chart::Transparent);

    p->addLineLayer(DoubleArray(streamed_prices.price_.data(), streamed_prices.price_.size()), RED);

    p->xAxis()->setLabels(StringArray(x_axis_label_data.data(), x_axis_label_data.size()))->setFontAngle(45.);
    p->xAxis()->setLabelStep(static_cast<int32_t>((x_axis_label_data.size()) / 40), 0);
    p->yAxis()->setLabelStyle("Arial Bold");

    // next, add our signals to this graphic.
    // Each signal type will be a separate layer so it can have its own glyph and color.

    p = ConstructCDPricesGraphicAddSignals(the_chart, streamed_prices, std::move(p));

    return p;
}

std::unique_ptr<XYChart> ConstructCDPricesGraphicAddSignals(const PF_Chart& the_chart,
                                                            const StreamedPrices& streamed_prices,
                                                            std::unique_ptr<XYChart> the_graphic)
{
    // signal types currently implemented
    // for each signal we want to show, we need x,y coordinates

    std::vector<double> dt_buys_price;
    std::vector<double> tt_buys_price;
    std::vector<double> db_sells_price;
    std::vector<double> tb_sells_price;
    std::vector<double> bullish_tt_buys_price;
    std::vector<double> bearish_tb_sells_price;
    std::vector<double> cat_buys_price;
    std::vector<double> cat_sells_price;
    std::vector<double> tt_cat_buys_price;
    std::vector<double> tb_cat_sells_price;

    std::vector<double> dt_buys_x;
    std::vector<double> tt_buys_x;
    std::vector<double> db_sells_x;
    std::vector<double> tb_sells_x;
    std::vector<double> bullish_tt_buys_x;
    std::vector<double> bearish_tb_sells_x;
    std::vector<double> cat_buys_x;
    std::vector<double> cat_sells_x;
    std::vector<double> tt_cat_buys_x;
    std::vector<double> tb_cat_sells_x;

    for (int32_t ndx = 0; ndx < streamed_prices.signal_type_.size(); ++ndx)
    {
        switch (streamed_prices.signal_type_[ndx])
        {
            using enum PF_SignalType;
            case std::to_underlying(e_double_top_buy):
                dt_buys_price.push_back(streamed_prices.price_[ndx]);
                dt_buys_x.push_back(ndx);
                break;
            case std::to_underlying(e_double_bottom_sell):
                db_sells_price.push_back(streamed_prices.price_[ndx]);
                db_sells_x.push_back(ndx);
                break;
            case std::to_underlying(e_triple_top_buy):
                tt_buys_price.push_back(streamed_prices.price_[ndx]);
                tt_buys_x.push_back(ndx);
                break;
            case std::to_underlying(e_triple_bottom_sell):
                tb_sells_price.push_back(streamed_prices.price_[ndx]);
                tb_sells_x.push_back(ndx);
                break;
            case std::to_underlying(e_bullish_tt_buy):
                bullish_tt_buys_price.push_back(streamed_prices.price_[ndx]);
                bullish_tt_buys_x.push_back(ndx);
                break;
            case std::to_underlying(e_bearish_tb_sell):
                bearish_tb_sells_price.push_back(streamed_prices.price_[ndx]);
                bearish_tb_sells_x.push_back(ndx);
                break;
            case std::to_underlying(e_catapult_buy):
                cat_buys_price.push_back(streamed_prices.price_[ndx]);
                cat_buys_x.push_back(ndx);
                break;
            case std::to_underlying(e_catapult_sell):
                cat_sells_price.push_back(streamed_prices.price_[ndx]);
                cat_sells_x.push_back(ndx);
                break;
            case std::to_underlying(e_ttop_catapult_buy):
                tt_cat_buys_price.push_back(streamed_prices.price_[ndx]);
                tt_cat_buys_x.push_back(ndx);
                break;
            case std::to_underlying(e_tbottom_catapult_sell):
                tb_cat_sells_price.push_back(streamed_prices.price_[ndx]);
                tb_cat_sells_x.push_back(ndx);
                break;
            case std::to_underlying(e_unknown):
            default:
                break;
        }
    }

    // now we can add layers (if any) with signals

    if (!dt_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(dt_buys_x.data(), dt_buys_x.size()),
                                     DoubleArray(dt_buys_price.data(), dt_buys_price.size()), "dt buy", dt_buy_sym, k13,
                                     GREEN);
    }

    if (!tt_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tt_buys_x.data(), tt_buys_x.size()),
                                     DoubleArray(tt_buys_price.data(), tt_buys_price.size()), "tt buy", tt_buy_sym, k13,
                                     GREEN);
    }

    if (!db_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(db_sells_x.data(), db_sells_x.size()),
                                     DoubleArray(db_sells_price.data(), db_sells_price.size()), "db sell", db_sell_sym,
                                     k13, RED);
    }

    if (!tb_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tb_sells_x.data(), tb_sells_x.size()),
                                     DoubleArray(tb_sells_price.data(), dt_buys_price.size()), "tb sell", tb_sell_sym,
                                     k13, RED);
    }

    if (!bullish_tt_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(bullish_tt_buys_x.data(), bullish_tt_buys_x.size()),
                                     DoubleArray(bullish_tt_buys_price.data(), bullish_tt_buys_price.size()),
                                     "bullish tt buy", bullish_tt_buy_sym, k13, GREEN);
    }

    if (!bearish_tb_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(bearish_tb_sells_x.data(), bearish_tb_sells_x.size()),
                                     DoubleArray(bearish_tb_sells_price.data(), bearish_tb_sells_price.size()),
                                     "bearish tb sell", bearish_tb_sell_sym, k13, RED);
    }

    if (!cat_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(cat_buys_x.data(), cat_buys_x.size()),
                                     DoubleArray(cat_buys_price.data(), cat_buys_price.size()), "cat buy", cat_buy_sym,
                                     k13, GREEN);
    }

    if (!cat_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(cat_sells_x.data(), cat_sells_x.size()),
                                     DoubleArray(cat_sells_price.data(), cat_sells_price.size()), "cat sell",
                                     cat_sell_sym, k13, RED);
    }

    if (!tt_cat_buys_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tt_cat_buys_x.data(), tt_cat_buys_x.size()),
                                     DoubleArray(tt_cat_buys_price.data(), tt_cat_buys_price.size()), "tt cat buy",
                                     tt_cat_buy_sym, k13, GREEN);
    }

    if (!tb_cat_sells_price.empty())
    {
        the_graphic->addScatterLayer(DoubleArray(tb_cat_sells_x.data(), tb_cat_sells_x.size()),
                                     DoubleArray(tb_cat_sells_price.data(), tb_cat_sells_price.size()), "tb cat sell",
                                     tb_cat_sell_sym, k13, RED);
    }

    return the_graphic;
}
/*
        d = XYChart((14 * 72), (8 * 72))
        d.setPlotArea(
            50, 50, (14 * 72 - 50), (8 * 72 - 200), -1, -1, LITEGRAY, LITEGRAY, -1
        )
        d.addTitle("Closing prices", "Times New Roman Bold Italic", 18)
        p_layer1 = d.addLineLayer(prices.close.to_list())
        d.addLegend(50, 30, 0, "Times New Roman Bold Italic", 12).setBackground(Transparent)

    */
