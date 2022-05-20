"""
    Make this code into a module so it can be loaded at program
    startup and parsed just once.

    This module is called from a C++ program using an embedded Python
    interpreter.  The C++ program creates a set of local variables which 
    are made available to this code.
"""

"""
	/* This file is part of PF_CollectData. */

	/* PF_CollectData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* PF_CollectData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with PF_CollectData.  If not, see <http://www.gnu.org/licenses/>. */
"""

import pandas as pd 
import matplotlib
import matplotlib.pyplot as plt
import mplfinance as mpf
from matplotlib.ticker import ScalarFormatter
import numpy as np

# load a non-GUI back end since we only produce graphic files as output

matplotlib.use("SVG")

def SetStepbackColor (is_up, stepped_back) : 
    if is_up:
        if stepped_back:
            return "blue"
    if not is_up:
        if stepped_back:
            return "orange"
    return None

def DrawChart(the_data, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat, UseLogScale, Y_min, Y_max):
    chart_data = pd.DataFrame(the_data)
    chart_data["Date"] = pd.to_datetime(chart_data["Date"])
    chart_data.set_index("Date", drop=False, inplace=True)

    chart_data["row_number"] = np.arange(chart_data.shape[0])

    # print(chart_data.dtypes)
    # print(chart_data)

    def date2index(a_date):
        print("date2index: ", a_date)
        return chart_data.loc[a_date[1]]["row_number"]

    def index2date(row_number):
        print("index2date: ", row_number)
        return chart_data.at[chart_data.index[row_number], "date"]

    mco = []
    for i in range(len(IsUp)):
        mco.append(SetStepbackColor(IsUp[i], StepBack[i]))

    mc = mpf.make_marketcolors(up='g',down='r')
    s  = mpf.make_mpf_style(marketcolors=mc, gridstyle="dashed")

    fig, axlist = mpf.plot(chart_data,
        type="candle",
        style=s,
        marketcolor_overrides=mco,
        title=ChartTitle,
        figsize=(12, 10),
        datetime_format=DateTimeFormat,
        returnfig=True)

    axlist[0].tick_params(which='both', left=True, right=True, labelright=True)
    if UseLogScale:
        plt.ylim(Y_min, Y_max)
        axlist[0].set_yscale("log")
        axlist[0].grid(which='both', axis='both', ls='-')
        axlist[0].yaxis.set_major_formatter(ScalarFormatter())
        axlist[0].yaxis.set_minor_formatter(ScalarFormatter()) 

    # secax = axlist[0].secondary_xaxis('top', functions=(date2index, index2date))
    secax = axlist[0].secondary_xaxis('top')

    plt.savefig(ChartFileName)
    for ax in axlist:
        ax.clear()
        del(ax)
    plt.close(fig)
    del(axlist)
    del(fig)
