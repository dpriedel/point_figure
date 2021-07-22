// =====================================================================================
// 
//       Filename:  aLine.h
// 
//    Description:  class to be used to read file line by line.
// 
//        Version:  1.0
//        Created:  04/26/2013 04:21:38 PM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================

	/* This file is part of CollectEDGARData. */

	/* CollectEDGARData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* CollectEDGARData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with CollectEDGARData.  If not, see <http://www.gnu.org/licenses/>. */


//#pragma once

#ifndef	_A_LINE_
#define _A_LINE_

#include <iostream>
#include <string>

//	a simple class to be used with stream iterators to read/write a 'line' of text

struct aLine
{
	std::string lineData;

	operator std::string(void) const { return lineData; }
};


inline std::istream&	operator>>(std::istream& input, aLine& data)
{
	std::getline(input, data.lineData, '\n');
	return input;
}

inline std::ostream&	operator<<(std::ostream& output, const aLine& data)
{
	output.write(data.lineData.c_str(), data.lineData.size());
	return output;
}

#endif

