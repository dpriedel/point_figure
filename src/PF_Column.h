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


	/* This file is part of PF_CollectData. */

	/* PF_CollectData is free software: you can redistribute it and/or modify */
	/* it under the terms of the GNU General Public License as published by */
	/* the Free Software Foundation, either version 3 of the License, or */
	/* (at your option) any later version. */

	/* PF_CollectData is distributed in the hope that it will be useful, */
	/* but WITHOUT ANY WARRANTY; without even the implied warranty of */
	/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the */
	/* GNU General Public License for more details. */

	/* You should have received a copy of the GNU General Public License */
	/* along with PF_CollectData.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef  PF_COLUMN_INC_
#define  PF_COLUMN_INC_

#include <chrono>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <utility>

#include <json/json.h>
#include <decimal.hh>

#include "Boxes.h"
#include "utilities.h"

class PF_Chart;

// =====================================================================================
//        Class:  PF_Column
//  Description:  
// =====================================================================================
class PF_Column
{
    friend class PF_Chart;

public:

    enum class Direction {e_Unknown, e_Up, e_Down};
    enum class Status { e_Accepted, e_Ignored, e_Reversal, e_AcceptedWithSignal };

    using TmPt = std::chrono::utc_time<std::chrono::utc_clock::duration>;
    using TimeSpan = std::pair<TmPt, TmPt>;

    using AddResult = std::pair<Status, std::optional<PF_Column>>;

    using ColumnBoxes = std::vector<Boxes::Box>;

    // ====================  LIFECYCLE     =======================================

    PF_Column() = default;
    PF_Column(const PF_Column& rhs) = default;
    PF_Column(PF_Column&& rhs) = default;

    PF_Column(Boxes* boxes, int32_t column_number, int32_t reversal_boxes,
            Direction direction = Direction::e_Unknown,
            decimal::Decimal top =-1, decimal::Decimal bottom =-1);

    PF_Column(Boxes* boxes, const Json::Value& new_data);

    ~PF_Column() = default;

    // ====================  ACCESSORS     =======================================

	[[nodiscard]] bool IsEmpty() const { return top_ == -1 && bottom_ == -1; }
    [[nodiscard]] decimal::Decimal GetTop() const { return top_; }
    [[nodiscard]] decimal::Decimal GetBottom() const { return  bottom_ ; }
    [[nodiscard]] double GetTopAsDbl() const { return dec2dbl(top_); };
    [[nodiscard]] double GetBottomAsDbl() const { return dec2dbl(bottom_); };
    [[nodiscard]] Direction GetDirection() const { return direction_; }
    [[nodiscard]] int32_t GetColumnNumber() const { return column_number_; }
    [[nodiscard]] int GetReversalboxes() const { return reversal_boxes_; }
    [[nodiscard]] bool GetHadReversal() const { return had_reversal_; }
    [[nodiscard]] TimeSpan GetTimeSpan() const { return time_span_; }

    // we return an actual list of the boxes rather than
    // a range in case the underlying Boxes list is modified
    // after we got our result.

    [[nodiscard]] ColumnBoxes GetColumnBoxes() const;

    [[nodiscard]] Json::Value ToJSON() const;

    // ====================  MUTATORS      =======================================

    [[nodiscard]] AddResult AddValue(const decimal::Decimal& new_value, TmPt the_time);
    [[nodiscard]] AddResult AddValue(std::string_view new_value, std::string_view the_time);

    // ====================  OPERATORS     =======================================

    PF_Column& operator= (const PF_Column& rhs) = default;
    PF_Column& operator= (PF_Column&& rhs) = default;

    // NOTE: time_span_ is excluded from equality comparison so it can be used
    // when looking for patterns over time.

    bool operator==(const PF_Column& rhs) const;
    bool operator!=(const PF_Column& rhs) const { return !operator==(rhs); };

protected:
    // make reversed column here because we know everything needed to do so.

    PF_Column MakeReversalColumn(Direction direction, const decimal::Decimal& value, TmPt the_time);

    // ====================  DATA MEMBERS  =======================================

private:

    void FromJSON(const Json::Value& new_data);

    [[nodiscard]] AddResult StartColumn(const decimal::Decimal& new_value, TmPt the_time);
    [[nodiscard]] AddResult TryToFindDirection(const decimal::Decimal& new_value, TmPt the_time);
    [[nodiscard]] AddResult TryToExtendUp(const decimal::Decimal& new_value, TmPt the_time);
    [[nodiscard]] AddResult TryToExtendDown(const decimal::Decimal& new_value, TmPt the_time);

    // ====================  DATA MEMBERS  =======================================

    TimeSpan time_span_;

    Boxes* boxes_ = nullptr;

    int32_t column_number_ = -1;
    int32_t reversal_boxes_ = -1;
    decimal::Decimal top_ = -1;
    decimal::Decimal bottom_ = -1;
    Direction direction_ = Direction::e_Unknown;

    // for 1-box, can have both up and down in same column
    bool had_reversal_ = false;

}; // -----  end of class PF_Column  -----

//
//	stream inserter
//

// custom fmtlib formatter for Direction

template <> struct std::formatter<PF_Column::Direction>: std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Column::Direction& direction, std::format_context& ctx) const
    {
        std::string s;
        switch(direction)
        {
            using enum PF_Column::Direction;
            case e_Unknown:
				std::format_to(std::back_inserter(s), "{}", "unknown");
                break;

            case e_Down:
				std::format_to(std::back_inserter(s), "{}", "down");
                break;

            case e_Up:
				std::format_to(std::back_inserter(s), "{}", "up");
                break;
        };
        return formatter<std::string>::format(s, ctx);
    }
};

// custom fmtlib formatter for Status

template <> struct std::formatter<PF_Column::Status>: std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Column::Status& status, std::format_context& ctx) const
    {
        std::string s;
        switch(status)
        {
            using enum PF_Column::Status;
            case e_Accepted:
				std::format_to(std::back_inserter(s), "{}", "accepted");
                break;

            case e_AcceptedWithSignal:
				std::format_to(std::back_inserter(s), "{}", "accepted w/signal");
                break;

            case e_Ignored:
				std::format_to(std::back_inserter(s), "{}", "ignored");
                break;

            case e_Reversal:
				std::format_to(std::back_inserter(s), "{}", "reversed");
                break;
        };
        return formatter<std::string>::format(s, ctx);
    }
};

// inline std::ostream& operator<<(std::ostream& os, const PF_Column::Status status)
// {
//     std::format_to(std::ostream_iterator<char>{os}, "{}", status);
//
// 	return os;
// }
//
// inline std::ostream& operator<<(std::ostream& os, const PF_Column::Direction direction)
// {
//     std::format_to(std::ostream_iterator<char>{os}, "{}", direction);
//
// 	return os;
// }
//
template <> struct std::formatter<PF_Column>: std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const PF_Column& column, std::format_context& ctx) const
    {
        std::string s;
        std::format_to(std::back_inserter(s), "col nbr: {}. bottom: {}. top: {}. direction: {}. begin date: {:%F}. {}",
            column.GetColumnNumber(), column.GetBottom().format("f"), column.GetTop().format("f"), column.GetDirection(), column.GetTimeSpan().first,
            (column.GetHadReversal() ? " one-step-back reversal." : ""));

        return formatter<std::string>::format(s, ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, const PF_Column& column)
{
    std::format_to(std::ostream_iterator<char>{os}, "{}", column);

    return os;
}

#endif   // ----- #ifndef P_F_COLUMN_INC_  ----- 
