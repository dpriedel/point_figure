
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import mplfinance as mpf

import warnings
import matplotlib.cbook
warnings.filterwarnings("ignore", category=matplotlib.cbook.mplDeprecation)

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


def SetStepbackColor(is_up, stepped_back):
    if is_up:
        if stepped_back:
            return "blue"
    if not is_up:
        if stepped_back:
            return "orange"
    return None


def DrawChart(the_data, ReversalBoxes, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat, ShowTrendLines,
              UseLogScale, Y_min, Y_max, openning_price, the_signals, streamed_prices):

    chart_data = pd.DataFrame(the_data)
    chart_data["DateTime"] = pd.to_datetime(chart_data["Date"], format=DateTimeFormat)
    chart_data.set_index("DateTime", drop=True, inplace=True)

    chart_data["row_number"] = np.arange(chart_data.shape[0])

    prices = pd.DataFrame(streamed_prices)
    if prices.shape[0] > 0:
        prices.index = pd.DatetimeIndex(pd.to_datetime(prices["the_time"], utc=True))
        prices.index = prices.index.tz_convert('America/New_York')
        prices["the_time"] = prices.index

    mco = []
    for i in range(len(IsUp)):
        mco.append(SetStepbackColor(IsUp[i], StepBack[i]))

    mc = mpf.make_marketcolors(up='g', down='r')
    s = mpf.make_mpf_style(marketcolors=mc, gridstyle="dashed")

    if prices.shape[0] < 1:
        fig = mpf.figure(figsize=(14, 10))
        ax1 = fig.add_subplot(1, 1, 1, style=s)
        ax2 = None
    else:
        fig = mpf.figure(figsize=(14, 14))
        ax1 = fig.add_subplot(2, 1, 1, style=s)
        ax2 = fig.add_subplot(2, 1, 2, style='yahoo')

    apds = []

    if ReversalBoxes > 1:
        if len(the_signals["dt_buys"]) > 0:
            sig_buys0 = mpf.make_addplot(the_signals["dt_buys"], ax=ax1, type="scatter", marker="^", color="black")
            apds.append(sig_buys0)
        if len(the_signals["tt_buys"]) > 0:
            sig_buys = mpf.make_addplot(the_signals["tt_buys"], ax=ax1, type="scatter", marker="^", color="yellow")
            apds.append(sig_buys)
        if len(the_signals["db_sells"]) > 0:
            sig_sells = mpf.make_addplot(the_signals["db_sells"], ax=ax1, type="scatter", marker="v", color="black")
            apds.append(sig_sells)
        if len(the_signals["tb_sells"]) > 0:
            sig_sells3 = mpf.make_addplot(the_signals["tb_sells"], ax=ax1, type="scatter", marker="v", color="yellow")
            apds.append(sig_sells3)
        if len(the_signals["bullish_tt_buys"]) > 0:
            sig_buys2 = mpf.make_addplot(the_signals["bullish_tt_buys"], ax=ax1, type="scatter", marker="P", color="blue")
            apds.append(sig_buys2)
        if len(the_signals["bearish_tb_sells"]) > 0:
            sig_sells2 = mpf.make_addplot(the_signals["bearish_tb_sells"], ax=ax1, type="scatter", marker="X", color="blue")
            apds.append(sig_sells2)

    mpf.plot(chart_data,
             ax=ax1,
             type="candle",
             style=s,
             marketcolor_overrides=mco,
             # title=ChartTitle,
             datetime_format=DateTimeFormat,
             hlines=dict(hlines=[openning_price], colors=['r'], linestyle='dotted', linewidths=(2)),
             addplot=apds)

    # plt.tick_params(which='both', left=True, right=True, labelright=True)

    fig.suptitle(ChartTitle)
    if prices.shape[0] > 0:
        zzz = prices.plot("the_time", "price", ax=ax2)
        zzz.grid(which='minor', axis='x', linestyle='dashed')

    plt.tick_params(which='both', left=True, right=True, labelright=True)
    plt.axhline(y=openning_price, color='r', linestyle='dotted')

    plt.savefig(ChartFileName)
    # ax1.clear()
    # ax2.clear()
    del ax1
    if prices.shape[0] > 0:
        del ax2

    # for ax in axlist:
    #     ax.clear()
    #     del ax
    plt.close(fig)
    # del axlist
    del fig
