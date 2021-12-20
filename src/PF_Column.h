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

#include <chrono>
#include <cstdint>
#include <utility>
//#include <memory>
#include <optional>

#include <json/json.h>

#include "DDecQuad.h"
#include "utilities.h"

// =====================================================================================
//        Class:  PF_Column
//  Description:  
// =====================================================================================
class PF_Column
{
public:

    enum class Direction {e_unknown, e_up, e_down};
    enum class Status { e_accepted, e_ignored, e_reversal };
    enum class BoxType { e_integral, e_fractional };
    enum class ColumnScale { e_linear, e_percent };

    using tpt = std::chrono::time_point<std::chrono::system_clock>;
    using TimeSpan = std::pair<tpt, tpt>;

    using AddResult = std::pair<Status, std::optional<PF_Column>>;

    // ====================  LIFECYCLE     =======================================

    PF_Column() = default;
    PF_Column(const PF_Column& rhs) = default;
    PF_Column(PF_Column&& rhs) = default;

    PF_Column(const DprDecimal::DDecQuad& box_size, int reversal_boxes,
            BoxType fractional_boxes = BoxType::e_integral,
            ColumnScale column_scale = ColumnScale::e_linear,
            Direction direction = Direction::e_unknown,
            DprDecimal::DDecQuad top =-1, DprDecimal::DDecQuad bottom =-1);

    PF_Column(const Json::Value& new_data);

    // ====================  ACCESSORS     =======================================

    [[nodiscard]] DprDecimal::DDecQuad GetTop() const { return column_scale_ == ColumnScale::e_linear ? top_ : top_.exp_n(); }
    [[nodiscard]] DprDecimal::DDecQuad GetBottom() const { return column_scale_ == ColumnScale::e_linear ? bottom_ : bottom_.exp_n(); }
    [[nodiscard]] DprDecimal::DDecQuad GetTop_Raw() const { return top_; }
    [[nodiscard]] DprDecimal::DDecQuad GetBottom_Raw() const { return  bottom_ ; }
    [[nodiscard]] Direction GetDirection() const { return direction_; }
    [[nodiscard]] DprDecimal::DDecQuad GetBoxsize() const { return box_size_; }
    [[nodiscard]] ColumnScale GetColumnScale() const { return column_scale_; }
    [[nodiscard]] int GetReversalboxes() const { return reversal_boxes_; }
    [[nodiscard]] bool GetHadReversal() const { return had_reversal_; }
    [[nodiscard]] TimeSpan GetTimeSpan() const { return time_span_; }

    [[nodiscard]] Json::Value ToJSON() const;

    // ====================  MUTATORS      =======================================

    [[nodiscard]] AddResult AddValue(const DprDecimal::DDecQuad& new_value, tpt the_time);

    // ====================  OPERATORS     =======================================

    PF_Column& operator= (const PF_Column& rhs) = default;
    PF_Column& operator= (PF_Column&& rhs) = default;

    PF_Column& operator= (const Json::Value& new_data);

    // NOTE: time_span_ is excluded from equality comparison so it can be used
    // when looking for patterns over time.

    bool operator==(const PF_Column& rhs) const;
    bool operator!=(const PF_Column& rhs) const { return !operator==(rhs); };

	friend std::ostream& operator<<(std::ostream& os, const PF_Column& column);

protected:
    // make reversed column here because we know everything needed to do so.

    PF_Column MakeReversalColumn(Direction direction, DprDecimal::DDecQuad value, tpt the_time);
    PF_Column MakeReversalColumnLog(Direction direction, DprDecimal::DDecQuad value, tpt the_time);

    // ====================  DATA MEMBERS  =======================================

private:

    void FromJSON(const Json::Value& new_data);

    [[nodiscard]] AddResult StartColumn(const DprDecimal::DDecQuad& new_value, tpt the_time);
    [[nodiscard]] AddResult TryToFindDirection(const DprDecimal::DDecQuad& possible_value, tpt the_time);
    [[nodiscard]] AddResult TryToExtendUp(const DprDecimal::DDecQuad& possible_value, tpt the_time);
    [[nodiscard]] AddResult TryToExtendDown(const DprDecimal::DDecQuad& possible_value, tpt the_time);

    [[nodiscard]] AddResult AddValueLog(const DprDecimal::DDecQuad& new_value, tpt the_time);

    [[nodiscard]] AddResult StartColumnLog(const DprDecimal::DDecQuad& new_value, tpt the_time);
    [[nodiscard]] AddResult TryToFindDirectionLog(const DprDecimal::DDecQuad& possible_value, tpt the_time);
    [[nodiscard]] AddResult TryToExtendUpLog(const DprDecimal::DDecQuad& possible_value, tpt the_time);
    [[nodiscard]] AddResult TryToExtendDownLog(const DprDecimal::DDecQuad& possible_value, tpt the_time);

    [[nodiscard]] DprDecimal::DDecQuad RoundDownToNearestBox(const DprDecimal::DDecQuad& a_value) const;

    // ====================  DATA MEMBERS  =======================================

    TimeSpan time_span_;
    DprDecimal::DDecQuad box_size_ = -1;
    DprDecimal::DDecQuad log_box_increment_ = -1;
    int32_t reversal_boxes_ = -1;

    DprDecimal::DDecQuad bottom_;
    DprDecimal::DDecQuad top_;
    Direction direction_ = Direction::e_unknown;
    BoxType box_type_ = BoxType::e_integral;      // whether to drop fractional part of new values.
    ColumnScale column_scale_ = ColumnScale::e_linear;

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
    os << "bottom: " << column.GetBottom() << " top: " << column.GetTop() << " direction: " << column.direction_
        << " begin date: " << TimePointToYMDString(column.GetTimeSpan().first) << (column.had_reversal_ ? " one-step-back reversal" : "");
    return os;
}

#endif   // ----- #ifndef P_F_COLUMN_INC_  ----- 
