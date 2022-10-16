
import json
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import mplfinance as mpf
from matplotlib.ticker import ScalarFormatter
import numpy as np

"""
    Make this code into a module so it can be loaded at program
    startup and parsed just once.

    This module is called from a C++ program using an embedded Python
    interpreter.  The C++ program creates a set of local variables which
    are made available to this code.
"""

"""
    /* This file is part of PointAndFigure. */

    /* PointAndFigure is free software: you can redistribute it and/or modify */
    /* it under the terms of the GNU General Public License as published by */
    /* the Free Software Foundation, either version 3 of the License, or */
    /* (at your option) any later version. */

    /* PointAndFigure is distributed in the hope that it will be useful, */
    /* but WITHOUT ANY WARRANTY; without even the implied warranty of */
    /* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
    /* GNU General Public License for more details. */

    /* You should have received a copy of the GNU General Public License */
    /* along with PointAndFigure.  If not, see <http://www.gnu.org/licenses/>. */


use axline to plot 45 degree trend lines

"""

# load a non-GUI back end since we only produce graphic files as output

matplotlib.use("SVG")

SLOPE = 0.707

# some functions to use for trend lines


def FindMinimum(is_up, chart_data):
    # we expect a data_frame

    minimum = 100000
    minimum_column = 0

    for i in range(len(is_up)):
        if not is_up[i]:
            # a down column
            if chart_data.iloc[i]["Low"] < minimum:
                minimum = chart_data.iloc[i]["Low"]
                minimum_column = i

    return minimum_column


def FindNextMaximum(data, start_at):
    pass


def SetStepbackColor(is_up, stepped_back):
    if is_up:
        if stepped_back:
            return "blue"
    if not is_up:
        if stepped_back:
            return "orange"
    return None


def DrawChart(the_data, ReversalBoxes, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat, ShowTrendLines,
              UseLogScale, Y_min, Y_max, openning_price, signals):

    apds = []
    if ReversalBoxes > 1:
        # print(signals)
        signal_data = json.loads(signals)

        # extract triple top buys
        # need to have correct number of values in list
        # must be same as number of rows columns in data
        dt_buys = [np.nan] * len(the_data["Open"])
        tt_buys = [np.nan] * len(the_data["Open"])
        db_sells = [np.nan] * len(the_data["Open"])
        tb_sells = [np.nan] * len(the_data["Open"])
        bullish_tt_buys = [np.nan] * len(the_data["Open"])
        bearish_tb_sells = [np.nan] * len(the_data["Open"])

        had_dt_buy = 0
        had_tt_buy = 0
        had_db_sell = 0
        had_tb_sell = 0
        had_bullish_tt_buy = 0
        had_bearish_tb_sell = 0

        for sig in signal_data:
            if sig["type"] == "dt_buy":
                dt_buys[int(sig["column"])] = float(sig["box"])
                had_dt_buy += 1
            elif sig["type"] == "tt_buy":
                tt_buys[int(sig["column"])] = float(sig["box"])
                had_tt_buy += 1
            elif sig["type"] == "db_sell":
                db_sells[int(sig["column"])] = float(sig["box"])
                had_db_sell += 1
            elif sig["type"] == "tb_sell":
                tb_sells[int(sig["column"])] = float(sig["box"])
                had_tb_sell += 1
            elif sig["type"] == "bullish_tt_buy":
                bullish_tt_buys[int(sig["column"])] = float(sig["box"])
                had_bullish_tt_buy += 1
            elif sig["type"] == "bearish_tb_sell":
                bearish_tb_sells[int(sig["column"])] = float(sig["box"])
                had_bearish_tb_sell += 1

        # use the higher priority signal when more than 1 at a location
        # hard code for now 

        for indx in range(len(dt_buys)):
            if not np.isnan(dt_buys[indx]) and not np.isnan(tt_buys[indx]):
                dt_buys[indx] = np.nan
                had_dt_buy -= 1
        for indx in range(len(tt_buys)):
            if not np.isnan(tt_buys[indx]) and not np.isnan(bullish_tt_buys[indx]):
                tt_buys[indx] = np.nan
                had_tt_buy -= 1
        for indx in range(len(db_sells)):
            if not np.isnan(db_sells[indx]) and not np.isnan(tb_sells[indx]):
                db_sells[indx] = np.nan
                had_db_sell -= 1
        for indx in range(len(tb_sells)):
            if not np.isnan(tb_sells[indx]) and not np.isnan(bearish_tb_sells[indx]):
                tb_sells[indx] = np.nan
                had_tb_sell -= 1

        if had_dt_buy:
            sig_buys0 = mpf.make_addplot(dt_buys, type="scatter", marker="^", color="black")
            apds.append(sig_buys0)
        if had_tt_buy:
            sig_buys = mpf.make_addplot(tt_buys, type="scatter", marker="^", color="yellow")
            apds.append(sig_buys)
        if had_db_sell:
            sig_sells = mpf.make_addplot(db_sells, type="scatter", marker="v", color="black")
            apds.append(sig_sells)
        if had_tb_sell:
            sig_sells3 = mpf.make_addplot(tb_sells, type="scatter", marker="v", color="yellow")
            apds.append(sig_sells3)
        if had_bullish_tt_buy:
            sig_buys2 = mpf.make_addplot(bullish_tt_buys, type="scatter", marker="P", color="blue")
            apds.append(sig_buys2)
        if had_bearish_tb_sell:
            sig_sells2 = mpf.make_addplot(bearish_tb_sells, type="scatter", marker="X", color="blue")
            apds.append(sig_sells2)

    chart_data = pd.DataFrame(the_data)
    chart_data["Date"] = pd.to_datetime(chart_data["Date"])
    chart_data.set_index("Date", drop=False, inplace=True)

    chart_data["row_number"] = np.arange(chart_data.shape[0])

    mco = []
    for i in range(len(IsUp)):
        mco.append(SetStepbackColor(IsUp[i], StepBack[i]))

    mc = mpf.make_marketcolors(up='g', down='r')
    s = mpf.make_mpf_style(marketcolors=mc, gridstyle="dashed")

    # now generate a sequence of date pairs:
    # dates = chart_data["Date"]
    # datepairs = [(d1,d2) for d1,d2 in zip(dates,dates[1:])]

    if chart_data.shape[0] < 3:
        ShowTrendLines = "no"

    if ShowTrendLines == "no":
        fig, axlist = mpf.plot(chart_data,
                               type="candle",
                               style=s,
                               marketcolor_overrides=mco,
                               title=ChartTitle,
                               figsize=(14, 10),
                               datetime_format=DateTimeFormat,
                               hlines=dict(hlines=[openning_price], colors=['r'], linestyle='dotted', linewidths=(2)),
                               addplot=apds,
                               returnfig=True)

    elif ShowTrendLines == "angle":

        # 45 degree trend line.
        # need 2 points: first down column bottom and computed value for last column

        x1 = FindMinimum(IsUp, chart_data)
        y1 = chart_data.iloc[x1]["Low"]
        x2 = chart_data.shape[0] - 1

        # formula for point slope line equation
        # y = slope(x - x1) + y1

        y2 = SLOPE * (x2 - x1) + y1
        a_line_points = [(chart_data.iloc[x1]["Date"], y1), (chart_data.iloc[x2]["Date"], y2)]
        fig, axlist = mpf.plot(chart_data,
                               type="candle",
                               style=s,
                               marketcolor_overrides=mco,
                               title=ChartTitle,
                               figsize=(14, 10),
                               datetime_format=DateTimeFormat,
                               alines=a_line_points,
                               returnfig=True)

    else:
        d1 = chart_data.index[0]
        d2 = chart_data.index[-1]
        tdates = [(d1, d2)]
    
        fig, axlist = mpf.plot(chart_data,
                               type="candle",
                               style=s,
                               marketcolor_overrides=mco,
                               title=ChartTitle,
                               figsize=(14, 10),
                               datetime_format=DateTimeFormat,
                               tlines=[dict(tlines=tdates, tline_use='High', tline_method="point-to-point", colors='r'),
                                       dict(tlines=tdates, tline_use='Low', tline_method="point-to-point", colors='b')],
                               returnfig=True)

    axlist[0].tick_params(which='both', left=True, right=True, labelright=True)
    axlist[0].secondary_xaxis('top')

    if UseLogScale:
        plt.ylim(Y_min, Y_max)
        axlist[0].set_yscale("log")
        axlist[0].grid(which='both', axis='both', ls='-')
        axlist[0].yaxis.set_major_formatter(ScalarFormatter())
        axlist[0].yaxis.set_minor_formatter(ScalarFormatter())

    plt.savefig(ChartFileName)
    for ax in axlist:
        ax.clear()
        del ax
    plt.close(fig)
    del axlist
    del fig
