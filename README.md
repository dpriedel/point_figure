PF_CollectData
================

This is a program which either collects data from a live feed (currently only from Tiingo because that is what I have access to) or by reading EOD data from CSV or JSON files.

This data is then processed into a Point and Figure 'charts' using logic described in "The Definitive Guide to Point and Figure" by Jeremy du Plessis, published by Harriman House Ltd.

These 'charts' are actually data files containing data about each column.  These charts can be rendered to SVG files using included code (Python and MPLFinace).

The idea is that the data files (or, coming, database tables) can be scanned in an automated process to find useful information.  The focus of this application is on bulk processing rather than interactive use.
I have this idea that it is possible to build machine learning processes to identify investeding and trading opportunites using this data.

At this point, I believe the code produces usable output so I am going to tag it as a release.



