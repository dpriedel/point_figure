

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

# some tables for printing signals on price graphs
# for each signal type, define the 'marker' and color we use

SIG_TYPE = {}
SIG_TYPE["dt_buys"] = 1
SIG_TYPE["db_sells"] = 2
SIG_TYPE["tt_buys"] = 3
SIG_TYPE["tb_sells"] = 4
SIG_TYPE["bullish_tt_buys"] = 5
SIG_TYPE["bearish_tb_sells"] = 6
SIG_TYPE["catapult_buys"] = 7
SIG_TYPE["catapult_sells"] = 8

SIG_INFO = {}
SIG_INFO[1] = ("^", "black")
SIG_INFO[2] = ("v", "black")
SIG_INFO[3] = ("^", "orange")
SIG_INFO[4] = ("v", "orange")
SIG_INFO[5] = ("P", "blue")
SIG_INFO[6] = ("X", "blue")
SIG_INFO[7] = ("^", "orange")
SIG_INFO[8] = ("v", "orange")


def SetStepbackColor(is_up, stepped_back):
    if is_up:
        if stepped_back:
            return "green"
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
        prices["DateTime"] = pd.to_datetime(prices["the_time"], utc=True)
        prices.set_index("DateTime", drop=True, inplace=True)
        prices.index = prices.index.tz_convert('America/New_York')
        prices["the_time"] = prices.index
        prices["Time"] = prices["the_time"].dt.time
        prices["signal_type"] = prices["signal_type"].apply(pd.to_numeric)

    mco = []
    for i in range(len(IsUp)):
        mco.append(SetStepbackColor(IsUp[i], StepBack[i]))

    mc = mpf.make_marketcolors(up='b', down='r')
    s = mpf.make_mpf_style(marketcolors=mc, gridstyle="dashed")

    if prices.shape[0] < 1:
        fig = mpf.figure(figsize=(14, 10))
        ax1 = fig.add_subplot(1, 1, 1, style=s, title=ChartTitle)
        ax2 = None
    else:
        fig = mpf.figure(figsize=(14, 14))
        ax1 = fig.add_subplot(2, 1, 1, style=s, title=ChartTitle)
        ax2 = fig.add_subplot(2, 1, 2, style='yahoo')

    apds = []

    # if ReversalBoxes > 1:
    for key in SIG_TYPE.keys():
        if len(the_signals[key]) > 0:
            mark, color = SIG_INFO[SIG_TYPE[key]]
            apds.append(mpf.make_addplot(the_signals[key], ax=ax1, type="scatter", marker=mark, color=color))

    mpf.plot(chart_data,
             ax=ax1,
             type="candle",
             style=s,
             marketcolor_overrides=mco,
             # title=ChartTitle,
             datetime_format=DateTimeFormat,
             hlines=dict(hlines=[openning_price], colors=['r'], linestyle='dotted', linewidths=(2)),
             addplot=apds)

    plt.figure(fig)
    plt.tick_params(which='both', left=True, right=True, labelright=True)

    # fig.suptitle(ChartTitle)

    if prices.shape[0] > 0:
        zzz = prices.plot("Time", "price", ax=ax2, color='gray')
        zzz.grid(which='minor', axis='x', linestyle='dashed')
        # ax2.xaxis.set_major_formatter(mdates.DateFormatter("%I:%M:%S"))
        # ax2.xaxis.set_major_formatter(matplotlib.dates.DateFormatter("%I:%M:%S", tz="America/New_York"))

        # if ReversalBoxes > 1:
        for key in SIG_INFO.keys():
            mark, color = SIG_INFO[key]
            sigs = prices[["Time", "price", "signal_type"]].copy()
            sigs.loc[sigs["signal_type"] != key, "price"] = np.nan
            values_to_show = sigs["price"].dropna()
            # sigs = prices[prices["signal_type"] == key]
            if values_to_show.size > 0:
                sigs.plot("Time", "price", ax=ax2, style=mark, color=color, markersize=8, label=list(SIG_TYPE.keys())[key - 1])

    # plt.figure(fig)
    if prices.shape[0] > 0: # and ReversalBoxes > 1:
        plt.legend(loc=2)

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
