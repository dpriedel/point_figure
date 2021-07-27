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
#include <cstdint>

// =====================================================================================
//        Class:  P_F_Column
//  Description:  
// =====================================================================================
class P_F_Column
{
public:

    enum class Direction {e_unknown, e_up, e_down};
    enum class Status { e_accepted, e_ignored, e_reversal };

    // ====================  LIFECYCLE     =======================================
    P_F_Column () = default;                             // constructor
    P_F_Column(int box_size, int reversal_boxes, Direction=Direction::e_unknown);

    // ====================  ACCESSORS     =======================================

    [[nodiscard]] int GetTop() const { return top_; }
    [[nodiscard]] int GetBottom() const { return bottom_; }
    [[nodiscard]] Direction GetDirection() const { return direction_; }
    [[nodiscard]] int GetBoxsize() const { return box_size_; }
    [[nodiscard]] int GetReversalboxes() const { return reversal_boxes_; }
    [[nodiscard]] bool GetHadReversal() const { return had_reversal_; }

    // ====================  MUTATORS      =======================================

    [[nodiscard]] Status AddValue(const DprDecimal::DDecDouble& new_value);

    // ====================  OPERATORS     =======================================

protected:
    // ====================  DATA MEMBERS  =======================================

private:

    [[nodiscard]] int32_t RoundDownToNearestBox(const DprDecimal::DDecDouble& a_value) const;

    // ====================  DATA MEMBERS  =======================================

    int32_t box_size_ = -1;
    int32_t reversal_boxes_ = -1;

    int32_t bottom_ = -1;
    int32_t top_ = -1;
    Direction direction_ = Direction::e_unknown;

    // for 1-box, can have both up and down in same column
    bool had_reversal_ = false;

}; // -----  end of class P_F_Column  -----

#endif   // ----- #ifndef P_F_COLUMN_INC  ----- 
