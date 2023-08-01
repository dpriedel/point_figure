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

#include <chrono>
#include <ranges>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <pybind11/embed.h> // everything needed for embedding
#include <pybind11/gil.h>
#include <pybind11/stl.h>

#include <spdlog/spdlog.h>

namespace py = pybind11;
using namespace py::literals;

#include "ConstructChartGraphic.h"

void ConstructChartGraphAndWriteToFile (const PF_Chart& the_chart, const fs::path& output_filename, const streamed_prices& streamed_prices, const std::string& show_trend_lines, PF_Chart::X_AxisFormat date_or_time)
{
	BOOST_ASSERT_MSG(! the_chart.empty(), std::format("Chart for symbol: {} contains no data. Unable to draw graphic.", the_chart.GetSymbol()).c_str());

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
	
	const auto max_columns_for_graph = the_chart.GetMaxGraphicColumns();
	size_t skipped_columns = max_columns_for_graph < 1 || the_chart.size() <= max_columns_for_graph ? 0 : the_chart.size() - max_columns_for_graph; 

    // we want to mark the openning value on the chart so change can be seen when drawing only most recent columns. 
    
    const auto& first_col = the_chart[0];
    double openning_price = first_col.GetDirection() == PF_Column::Direction::e_Up ? dec2dbl(first_col.GetBottom()) : dec2dbl(first_col.GetTop());

    for (const auto& col : the_chart.GetColumns() | vws::drop(skipped_columns))
    {
        lowData.push_back(dec2dbl(col.GetBottom()));
        highData.push_back(dec2dbl(col.GetTop()));

        if (col.GetDirection() == PF_Column::Direction::e_Up)
        {
            openData.push_back(dec2dbl(col.GetBottom()));
            closeData.push_back(dec2dbl(col.GetTop()));
            direction_is_up.push_back(true);
        }
        else
        {
            openData.push_back(dec2dbl(col.GetTop()));
            closeData.push_back(dec2dbl(col.GetBottom()));
            direction_is_up.push_back(false);
        }
		if (date_or_time == PF_Chart::X_AxisFormat::e_show_date)
        {
	        x_axis_labels.push_back(std::format("{:%F}", col.GetTimeSpan().first));
        }
		else
        {
            x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first));
        }
        had_step_back.push_back(col.GetHadReversal());
    }

    lowData.push_back(dec2dbl(the_chart.back().GetBottom()));
    highData.push_back(dec2dbl(the_chart.back().GetTop()));

    if (the_chart.back().GetDirection() == PF_Column::Direction::e_Up)
    {
        openData.push_back(dec2dbl(the_chart.back().GetBottom()));
        closeData.push_back(dec2dbl(the_chart.back().GetTop()));
        direction_is_up.push_back(true);
    }
    else
    {
        openData.push_back(dec2dbl(the_chart.back().GetTop()));
        closeData.push_back(dec2dbl(the_chart.back().GetBottom()));
        direction_is_up.push_back(false);
    }
    if (date_or_time == PF_Chart::X_AxisFormat::e_show_date)
    {
        x_axis_labels.push_back(std::format("{:%F}", the_chart.back().GetTimeSpan().first));
    }
    else
    {
        x_axis_labels.push_back(UTCTimePointToLocalTZHMSString(the_chart.back().GetTimeSpan().first));
    }

    had_step_back.push_back(the_chart.back().GetHadReversal());

	// extract and format the Signals, if any, from the chart. The drawing code can plot
	// them or not.
	// NOTE: we need to zap the column number with the skipped columns offset 

	// let's try to do lower level setup for signals drawing here.
    // we want 1 signal per column and that should be the signal with highest priority
    // data is formated as needed by mplfinance library to build lists of marks to overlay
    // on graphic. this saves a bunch of python code

	const auto& sngls = the_chart.GetSignals();
    std::vector<double> dt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> db_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> btt_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> btb_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tt_cat_buys(openData.size(), std::numeric_limits<double>::quiet_NaN());
    std::vector<double> tb_cat_sells(openData.size(), std::numeric_limits<double>::quiet_NaN());

    int had_dt_buy = 0;
    int had_tt_buy = 0;
    int had_db_sell = 0;
    int had_tb_sell = 0;
    int had_bullish_tt_buy = 0;
    int had_bearish_tb_sell = 0;
    int had_catapult_buy = 0;
    int had_catapult_sell = 0;
    int had_tt_catapult_buy = 0;
    int had_tb_catapult_sell = 0;

    for (const auto& sigs : sngls
            | vws::filter([skipped_columns] (const auto& s) { return s.column_number_ >= skipped_columns; })
            | vws::chunk_by([](const auto& a, const auto& b) { return a.column_number_ == b.column_number_; }))
    {
        const auto most_important = rng::max_element(sigs, {}, [](const auto& s) { return std::to_underlying(s.priority_); }) ;
        switch (most_important->signal_type_)
        {
            using enum PF_SignalType;
            case e_Unknown:
                break;
            case e_DoubleTop_Buy:
                dt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_dt_buy += 1;
                break;
            case e_DoubleBottom_Sell:
                db_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_db_sell += 1;
                break;
            case e_TripleTop_Buy:
                tt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_tt_buy += 1;
                break;
            case e_TripleBottom_Sell:
                tb_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_tb_sell += 1;
                break;
            case e_Bullish_TT_Buy:
                btt_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_bullish_tt_buy += 1;
                break;
            case e_Bearish_TB_Sell:
                btb_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_bearish_tb_sell += 1;
                break;
            case e_Catapult_Buy:
                cat_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_catapult_buy += 1;
                break;
            case e_Catapult_Sell:
                cat_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_catapult_sell += 1;
                break;
            case e_TTop_Catapult_Buy:
                tt_cat_buys[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_tt_catapult_buy += 1;
                break;
            case e_TBottom_Catapult_Sell:
                tb_cat_sells[most_important->column_number_ - skipped_columns] = dec2dbl(most_important->box_);
                had_tb_catapult_sell += 1;
                break;
        }
    }

	// want to show approximate overall change in value (computed from boxes, not actual prices)
	
	decimal::Decimal first_value = 0;
	decimal::Decimal last_value = 0;

	if (the_chart.size() > 1)
	{
		const auto& first_col = the_chart[0];
		first_value = first_col.GetDirection() == PF_Column::Direction::e_Up ? first_col.GetBottom() : first_col.GetTop();
		// apparently, this can happen 

		if (first_value == sv2dec("0.0"))
		{
			first_value = sv2dec("0.01");
		}
	}
	else
	{
		first_value = the_chart.back().GetDirection() == PF_Column::Direction::e_Up ? the_chart.back().GetBottom() : the_chart.back().GetTop();
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
    if (the_chart.GetReversalboxes() == 1)
    {
        explanation_text = "Orange: 1-step Up then reversal Down. Green: 1-step Down then reversal Up.";
    }
    auto chart_title = std::format("\n{}{} X {} for {} {}. Overall % change: {}{}\nLast change: {:%a, %b %d, %Y at %I:%M:%S %p %Z}\n{}",
                                   the_chart.GetChartBoxSize().format("f"), (the_chart.IsPercent() ? "%" : ""),
                                   the_chart.GetReversalboxes(), the_chart.GetSymbol(),
                                   (the_chart.IsPercent() ? "percent" : ""), overall_pct_chg.format("f"), skipped_columns_text,
                                   std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::clock_cast<std::chrono::system_clock>(the_chart.GetLastChangeTime())), explanation_text);

    py::dict locals = py::dict{
        "the_data"_a = py::dict{
            "Date"_a = x_axis_labels,
            "Open"_a = openData,
            "High"_a = highData,
            "Low"_a = lowData,
            "Close"_a = closeData
        },
        "ReversalBoxes"_a = the_chart.GetReversalboxes(),
        "IsUp"_a = direction_is_up,
        "StepBack"_a = had_step_back,
        "ChartTitle"_a = chart_title,
        "ChartFileName"_a = output_filename.string(),
        "DateTimeFormat"_a = date_or_time == PF_Chart::X_AxisFormat::e_show_date ? "%Y-%m-%d" : "%H:%M:%S",
        "Y_min"_a = dec2dbl(the_chart.GetYLimits().first),
        "Y_max"_a = dec2dbl(the_chart.GetYLimits().second),
        "openning_price"_a = openning_price,
        "UseLogScale"_a = the_chart.IsPercent(),
        "ShowTrendLines"_a = show_trend_lines,
        "the_signals"_a = py::dict{
            "dt_buys"_a = had_dt_buy != 0 ? dt_buys : std::vector<double>{},
            "db_sells"_a = had_db_sell != 0 ? db_sells : std::vector<double>{},
            "tt_buys"_a = had_tt_buy != 0 ? tt_buys : std::vector<double>{},
            "tb_sells"_a = had_tb_sell != 0 ? tb_sells : std::vector<double>{},
            "bullish_tt_buys"_a = had_bullish_tt_buy != 0 ? btt_buys : std::vector<double>{},
            "bearish_tb_sells"_a = had_bearish_tb_sell != 0 ? btb_sells : std::vector<double>{},
            "catapult_buys"_a = had_catapult_buy != 0 ? cat_buys : std::vector<double>{},
            "catapult_sells"_a = had_catapult_sell != 0 ? cat_sells : std::vector<double>{},
            "tt_catapult_buys"_a = had_tt_catapult_buy != 0 ? tt_cat_buys : std::vector<double>{},
            "tb_catapult_sells"_a = had_tb_catapult_sell != 0 ? tb_cat_sells : std::vector<double>{}
        },
        "streamed_prices"_a = py::dict{
            "the_time"_a = streamed_prices.timestamp_,
            "price"_a = streamed_prices.price_,
            "signal_type"_a = streamed_prices.signal_type_
        }
    };

        // Execute Python code, using the variables saved in `locals`

//        py::gil_scoped_acquire gil{};
        py::exec(R"(
        PF_DrawChart.DrawChart(the_data, ReversalBoxes, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat,
        ShowTrendLines, UseLogScale, Y_min, Y_max, openning_price, the_signals, streamed_prices)
        )", py::globals(), locals);
}		

