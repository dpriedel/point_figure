// =====================================================================================
// 
//       Filename:  PF_Column.h
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

#ifndef  PF_COLUMN_INC_
#define  PF_COLUMN_INC_

#include <cstdint>
#include <utility>
#include <memory>
#include <optional>

#include "DDecDouble.h"

// =====================================================================================
//        Class:  PF_Column
//  Description:  
// =====================================================================================
class PF_Column
{
public:

    enum class Direction {e_unknown, e_up, e_down};
    enum class Status { e_accepted, e_ignored, e_reversal };
    enum class FractionalBoxes { e_integral, e_fractional };

    using AddResult = std::pair<Status, std::optional<std::unique_ptr<PF_Column>>>;

    // ====================  LIFECYCLE     =======================================
    PF_Column () = default;                             // constructor
    PF_Column(DprDecimal::DDecDouble box_size, int reversal_boxes,
            FractionalBoxes fractional_boxes = FractionalBoxes::e_integral,
            Direction direction = Direction::e_unknown,
            DprDecimal::DDecDouble top =-1, DprDecimal::DDecDouble bottom =-1);

    // ====================  ACCESSORS     =======================================

    [[nodiscard]] DprDecimal::DDecDouble GetTop() const { return top_; }
    [[nodiscard]] DprDecimal::DDecDouble GetBottom() const { return bottom_; }
    [[nodiscard]] Direction GetDirection() const { return direction_; }
    [[nodiscard]] DprDecimal::DDecDouble GetBoxsize() const { return box_size_; }
    [[nodiscard]] int GetReversalboxes() const { return reversal_boxes_; }
    [[nodiscard]] bool GetHadReversal() const { return had_reversal_; }

    // ====================  MUTATORS      =======================================

    [[nodiscard]] AddResult AddValue(const DprDecimal::DDecDouble& new_value);
	friend std::ostream& operator<<(std::ostream& os, const PF_Column& column);

    // ====================  OPERATORS     =======================================

protected:
    // make reversed column here because we know everything needed to do so.

    std::unique_ptr<PF_Column> MakeReversalColumn(Direction direction, DprDecimal::DDecDouble value);

    // ====================  DATA MEMBERS  =======================================

private:

    [[nodiscard]] DprDecimal::DDecDouble RoundDownToNearestBox(const DprDecimal::DDecDouble& a_value) const;

    // ====================  DATA MEMBERS  =======================================

    DprDecimal::DDecDouble box_size_ = -1;
    int32_t reversal_boxes_ = -1;

    DprDecimal::DDecDouble bottom_;
    DprDecimal::DDecDouble top_;
    Direction direction_;
    FractionalBoxes fractional_boxes_;      // whether to drop fractional part of new values.

    // for 1-box, can have both up and down in same column
    bool had_reversal_ = false;

}; // -----  end of class PF_Column  -----

//
//	stream inserter
//

inline std::ostream& operator<<(std::ostream& os, const PF_Column::Status status)
{
    switch(status)
    {
        case PF_Column::Status::e_accepted:
            os << "accepted";
            break;

        case PF_Column::Status::e_ignored:
            os << "ignored";
            break;

        case PF_Column::Status::e_reversal:
            os << "reversed";
            break;
    };

	return os;
}

inline std::ostream& operator<<(std::ostream& os, const PF_Column::Direction direction)
{
    switch(direction)
    {
        case PF_Column::Direction::e_unknown:
            os << "unknown";
            break;

        case PF_Column::Direction::e_down:
            os << "down";
            break;

        case PF_Column::Direction::e_up:
            os << "up";
            break;
    };

	return os;
}

inline std::ostream& operator<<(std::ostream& os, const PF_Column& column)
{
    os << "bottom: " << column.bottom_ << " top: " << column.top_ << " direction: " << column.direction_
        << (column.had_reversal_ ? " one step back reversal" : "");
    return os;
}

#endif   // ----- #ifndef P_F_COLUMN_INC_  ----- 
