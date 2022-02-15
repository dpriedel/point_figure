"""
    Make this code into a module so it can be loaded at program
    startup and parsed just once.

    This module is called from a C++ program using an embedded Python
    interpreter.  The C++ program creates a set of local variables which 
    are made available to this code.
"""

import pandas as pd 
import matplotlib
import matplotlib.pyplot as plt
import mplfinance as mpf
from matplotlib.ticker import ScalarFormatter

# load a non-GUI back end since we only produce graphic files as output

matplotlib.use("SVG")

def SetStepbackColor (is_up, stepped_back) : 
    if is_up:
        if stepped_back:
            return "blue"
    if not is_up:
        if stepped_back:
            return "yellow"
    return None

def DrawChart(the_data, IsUp, StepBack, ChartTitle, ChartFileName, DateTimeFormat, UseLogScale, Y_min, Y_max):
    the_data["Date"] = pd.to_datetime(the_data["Date"])
    chart_data = pd.DataFrame(the_data)
    chart_data.set_index("Date", inplace=True)

    #print(chart_data)

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

    plt.savefig(ChartFileName)
    for ax in axlist:
        ax.clear()
        del(ax)
    plt.close(fig)
    del(axlist)
    del(fig)
