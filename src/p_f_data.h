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
#include <string>
#include <vector>

#include <date/date.h>

//using namespace std::chrono_literals;


class P_F_Column;

class P_F_Data
{
    using tpt = std::chrono::time_point<std::chrono::steady_clock>;

public:
    enum class Direction { e_unknown, e_moving_up, e_moving_down };

    // ====================  LIFECYCLE     =======================================
    P_F_Data () = default;                             // constructor
    P_F_Data(std::string symbol, std::int32_t boxsize, int32_t reversal_boxes);

    // ====================  ACCESSORS     =======================================

    void ExportData(std::ostream* output_data);
    [[nodiscard]] Direction GetCurrentDirection() const { return currentdirection_; }

    // ====================  MUTATORS      =======================================
    
    void LoadData(std::istream* input_data);

    // ====================  OPERATORS     =======================================

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================

    std::vector<P_F_Column> columns_;

    std::string symbol_;

    tpt mFirstDate;			//	earliest entry for symbol
    tpt mLastChangeDate;		//	date of last change to data
    tpt mLastCheckedDate;	//	last time checked to see if update needed

    int32_t boxsize_;
    int32_t reversal_boxes_;

    Direction currentdirection_;

}; // -----  end of class P_F_Data  -----
#endif   // ----- #ifndef P_F_DATA_INC  ----- 

