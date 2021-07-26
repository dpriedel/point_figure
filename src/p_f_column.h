// =====================================================================================
// 
//       Filename:  p_f_column.h
// 
//    Description:  header for class which implements Point & Figure column data
// 
//        Version:  1.0
//        Created:  2021-07-26 09:36 AM 
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  David P. Riedel (dpr), driedel@cox.net
//        License:  GNU General Public License v3
//        Company:  
// 
// =====================================================================================

#ifndef  P_F_COLUMN_INC_
#define  P_F_COLUMN_INC_

#include "DDecDouble.h"

// =====================================================================================
//        Class:  P_F_Column
//  Description:  
// =====================================================================================
class P_F_Column
{
public:

    enum Direction {e_unknown, e_up, e_down};

    // ====================  LIFECYCLE     =======================================
    P_F_Column () = default;                             // constructor
    P_F_Column(int box_size, int reversal_size, Direction=e_unknown);

    // ====================  ACCESSORS     =======================================

    int GetTop() { return top_; }
    int GetBottom() { return top_; }
    Direction GetDirection() { return direction_; }
    int GetBoxsize() { return box_size_; }
    int GetReversalsize() { return reversal_size_; }

    // ====================  MUTATORS      =======================================

    bool AddValue(DprDecimal::DDecDouble& new_value);

    // ====================  OPERATORS     =======================================

protected:
    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  DATA MEMBERS  =======================================

    int box_size_ = 0;
    int reversal_size_ = 0;

    int bottom_ = 0;
    int top_ = 0;
    Direction direction_ = e_unknown;

}; // -----  end of class P_F_Column  -----

#endif   // ----- #ifndef P_F_COLUMN_INC  ----- 
