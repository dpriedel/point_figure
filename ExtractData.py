#!/usr/bin/python

"""
	this program will extract data for the specified symbol and date range from
	the current_data stocks DB.

"""


import os, sys, traceback, datetime, time
from operator import attrgetter

import logging
THE_LOGGER = logging.getLogger()
THE_LOGGER.setLevel(logging.WARN)
THE_LOGGER.addHandler(logging.StreamHandler(sys.stderr))

import argparse
import pyodbc

BIN_DIR =   ""			  #	in case we need other stuff

class ConvertDate(argparse.Action):
	
	def __call__(self, parser, namespace, values, option_string=None):
		#print '%r %r %r' % (namespace, values, option_string)
		the_date = datetime.datetime.strptime(values, "%Y-%m-%d")
		setattr(namespace, self.dest, the_date.date())

def Main():
	
	result  =   0
	
	try:

		#	we set up some import paths

		runtime_dir = os.path.dirname(os.path.abspath(sys.argv[0]))
		
		#	we may have some other location from which to load modules

		if len(BIN_DIR) > 0:
			if BIN_DIR not in sys.path:
				sys.path.insert(0, BIN_DIR)
			
		#	Make sure we check the local directory first
	
		if runtime_dir not in sys.path:
			sys.path.insert(0, runtime_dir)
		
		parser = argparse.ArgumentParser(description='Extract stock data from DB.')
		parser.add_argument("-b", "--begin",  action=ConvertDate, dest="start_date", default=datetime.date.today(),
			help="starting with this date or %s if not specified." % datetime.date.today())
		parser.add_argument("-e", "--end",  action=ConvertDate, dest="end_date", default=datetime.date.today(),
			help="going through this date or %s if not specified." % datetime.date.today())
		parser.add_argument("-s", "--symbol",  action="store", type=str, dest="symbol", default="",
			help="Specify symbol for which data is to be extracted.")
		parser.add_argument("-o", "--output",  action="store", type=str, dest="output", default="",
			help="Specify output path. Omit for stdout.")
		parser.add_argument("-l", "--logging",  action="store", dest="log_level", default="warning",
			help="logging level: info, debug, warning, error, critical. Default is 'warning'.")

		args = parser.parse_args()

		LEVELS = {"debug": logging.DEBUG,
				"info": logging.INFO,
				"warning": logging.WARNING,
				"error": logging.ERROR,
				"critical": logging.CRITICAL}
		
		log_level = LEVELS.get(args.log_level, logging.WARNING)
		THE_LOGGER.setLevel(level=log_level)

		if makes_sense_to_run(args):
			ExtractSymbolData(args.start_date, args.end_date, args.symbol, args.output)

	#	parse_args will throw a SystemExit after displaying help info.
	
	except SystemExit:
		pass
	
	except:

		traceback.print_exc()
		
		sys.stdout.flush()
		result = 6

	return result

def makes_sense_to_run(args):
	
	if args.symbol == "":
		raise RuntimeError("Must specify a symbol." )
	return True
	
	
if __name__ == '__main__':
	sys.exit(Main())
	
