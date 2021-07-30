// =====================================================================================
// 
//       Filename:  p_f_data.h
// 
//    Description:  class to manage Point & Figure data for a symbol.
// 
//        Version:  2.0
//        Created:  2021-07-29 08:29 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================

// =====================================================================================
//        Class:  P_F_Data
//  Description:  class to manage Point & Figure data for a symbol.
// =====================================================================================

#ifndef  P_F_DATA_INC
#define  P_F_DATA_INC


#include <chrono>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <date/date.h>

//using namespace std::chrono_literals;

#include "p_f_column.h"


class P_F_Data
{
    using tpt = std::chrono::time_point<std::chrono::steady_clock>;

public:

    using Y_Limits = std::pair<int32_t, int32_t>;

    // ====================  LIFECYCLE     =======================================
    P_F_Data () = default;                             // constructor
    P_F_Data(const std::string& symbol, std::int32_t boxsize, int32_t reversal_boxes);

    // ====================  ACCESSORS     =======================================

    void ExportData(std::ostream* output_data);
    [[nodiscard]] P_F_Column::Direction GetCurrentDirection() const { return current_direction_; }

    [[nodiscard]] std::size_t GetNumberOfColumns() const { return columns_.size(); }

    [[nodiscard]] Y_Limits GetYLimits() const { return {y_min_, y_max_}; }

    // ====================  MUTATORS      =======================================
    
    void LoadData(std::istream* input_data);

    // ====================  OPERATORS     =======================================

    const P_F_Column& operator[](size_t which) const { return columns_[which]; }

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================

    std::vector<P_F_Column> columns_;
    std::unique_ptr<P_F_Column> current_column_;

    std::string symbol_;

    tpt mFirstDate;			//	earliest entry for symbol
    tpt mLastChangeDate;		//	date of last change to data
    tpt mLastCheckedDate;	//	last time checked to see if update needed

    int32_t boxsize_;
    int32_t reversal_boxes_;
    int32_t y_min_ = std::numeric_limits<int32_t>::max();
    int32_t y_max_ = std::numeric_limits<int32_t>::min();

    P_F_Column::Direction current_direction_;

}; // -----  end of class P_F_Data  -----
#endif   // ----- #ifndef P_F_DATA_INC  ----- 

