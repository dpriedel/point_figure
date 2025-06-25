// =====================================================================================
//
//       Filename:  Boxes.h
//
//    Description:  Encapsulate/track box creation and use
//
//        Version:  1.0
//        Created:  12/22/2021 09:42:30 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  David P. Riedel (), driedel@cox.net
//        License:  GNU General Public License -v3
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

#ifndef BOXES_INC
#define BOXES_INC

#include <cstdint>
#include <deque>
#include <format>
#include <iterator>

#include <json/json.h>

#include <decimal.hh>

#include "utilities.h"

enum class BoxType : int32_t
{
    e_Integral,
    e_Fractional
};
enum class BoxScale : int32_t
{
    e_Linear,
    e_Percent
};

// =====================================================================================
//        Class:  Boxes
//  Description:  Manage creation and use of P & F boxes
// =====================================================================================
class Boxes
{
public:
    using Box = decimal::Decimal;
    using BoxList = std::deque<Box>; // use a deque so we can add at either end

    static constexpr std::size_t kMaxBoxes = 1000; // too many boxes and everything becomes too slow
    static constexpr int64_t kMinExponent = -5;

    // ====================  LIFECYCLE     =======================================
    Boxes() = default; // constructor
    Boxes(const Boxes &rhs) = default;
    Boxes(Boxes &&rhs) noexcept = default;

    explicit Boxes(decimal::Decimal base_box_size, decimal::Decimal box_size_modifier = 0,
                   BoxScale box_scale = BoxScale::e_Linear);
    explicit Boxes(double base_box_size, double box_size_modifier = 0.0, BoxScale box_scale = BoxScale::e_Linear)
        : Boxes(dbl2dec(base_box_size), dbl2dec(box_size_modifier), box_scale) {};

    explicit Boxes(const Json::Value &new_data);

    ~Boxes() = default;

    // ====================  ACCESSORS     =======================================

    [[nodiscard]] decimal::Decimal GetBoxSize() const
    {
        return runtime_box_size_;
    }
    [[nodiscard]] BoxType GetBoxType() const
    {
        return box_type_;
    }
    [[nodiscard]] BoxScale GetBoxScale() const
    {
        return box_scale_;
    }
    [[nodiscard]] decimal::Decimal GetScaleUpFactor() const
    {
        return percent_box_factor_up_;
    }
    [[nodiscard]] decimal::Decimal GetScaleDownFactor() const
    {
        return percent_box_factor_down_;
    }
    [[nodiscard]] int64_t GetExponent() const
    {
        return percent_exponent_;
    }
    [[nodiscard]] size_t GetHowMany() const
    {
        return boxes_.size();
    }

    [[nodiscard]] const BoxList &GetBoxList() const
    {
        return boxes_;
    }

    [[nodiscard]] Json::Value ToJSON() const;

    [[nodiscard]] size_t Distance(const Box &from, const Box &to) const;

    // ====================  MUTATORS      =======================================

    Box FindBox(const decimal::Decimal &new_value);
    Box FindNextBox(const decimal::Decimal &current_value);
    Box FindPrevBox(const decimal::Decimal &current_value);

    // we have some lookup-only uses

    [[nodiscard]] Box FindNextBox(const decimal::Decimal &current_value) const;
    [[nodiscard]] Box FindPrevBox(const decimal::Decimal &current_value) const;

    // ====================  OPERATORS     =======================================

    bool operator==(const Boxes &rhs) const;
    bool operator!=(const Boxes &rhs) const
    {
        return !operator==(rhs);
    }

    Boxes &operator=(const Json::Value &new_data);

    Boxes &operator=(const Boxes &rhs) = default;
    Boxes &operator=(Boxes &&rhs) = default;

protected:
    // ====================  METHODS       =======================================

    // ====================  DATA MEMBERS  =======================================

private:
    // ====================  METHODS       =======================================

    void FromJSON(const Json::Value &new_data);

    Box FirstBox(const decimal::Decimal &start_at);
    Box FirstBoxPerCent(const decimal::Decimal &start_at);
    Box FindBoxPercent(const decimal::Decimal &new_value);
    Box FindNextBoxPercent(const decimal::Decimal &current_value);
    [[nodiscard]] Box FindNextBoxPercent(const decimal::Decimal &current_value) const;
    Box FindPrevBoxPercent(const decimal::Decimal &current_value);
    [[nodiscard]] Box FindPrevBoxPercent(const decimal::Decimal &current_value) const;
    [[nodiscard]] Box RoundDownToNearestBox(const decimal::Decimal &a_value) const;

    // these functions implement our max number of boxes limit

    void PushFront(Box new_box);
    void PushBack(Box new_box);

    // ====================  DATA MEMBERS  =======================================

    Box k_min_box_size_{".01"}; // This is arbitrary since stocks can trade in fractions of a penny

    BoxList boxes_;

    decimal::Decimal base_box_size_ = -1;
    decimal::Decimal box_size_modifier_ = 0;
    decimal::Decimal runtime_box_size_ = -1;
    decimal::Decimal percent_box_factor_up_ = -1;
    decimal::Decimal percent_box_factor_down_ = -1;

    int64_t percent_exponent_ = 0;
    BoxType box_type_ = BoxType::e_Integral; // whether to drop fractional part of new values.
    BoxScale box_scale_ = BoxScale::e_Linear;

}; // -----  end of class Boxes  -----

// custom fmtlib formatter for BoxType

template <> struct std::formatter<BoxType> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const BoxType &box_type, std::format_context &ctx) const
    {
        std::string s;
        std::format_to(std::back_inserter(s), "{}", (box_type == BoxType::e_Integral ? "integral" : "fractional"));
        return formatter<std::string>::format(s, ctx);
    }
};

// custom fmtlib formatter for BoxScale

template <> struct std::formatter<BoxScale> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const BoxScale &box_scale, std::format_context &ctx) const
    {
        std::string s;
        std::format_to(std::back_inserter(s), "{}", (box_scale == BoxScale::e_Linear ? "linear" : "percent"));
        return formatter<std::string>::format(s, ctx);
    }
};

// inline std::ostream& operator<<(std::ostream& os, const BoxType box_type)
// {
//     std::format_to(std::ostream_iterator<char>{os}, "{}", box_type);
//
// 	return os;
// }
//
// inline std::ostream& operator<<(std::ostream& os, const BoxScale box_scale)
// {
//     std::format_to(std::ostream_iterator<char>{os}, "{}", box_scale);
//
// 	return os;
// }
//
template <> struct std::formatter<Boxes> : std::formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Boxes &boxes, std::format_context &ctx) const
    {
        std::string s;
        std::format_to(std::back_inserter(s),
                       "Boxes: how many: {}. box size: {}. factor up: {}. factor down: {}. exponent: {}. box type: {}. "
                       "box scale: {}.\n",
                       boxes.GetHowMany(), boxes.GetBoxSize().format("f"), boxes.GetScaleUpFactor().format("f"),
                       boxes.GetScaleDownFactor().format("f"), boxes.GetExponent(), boxes.GetBoxType(),
                       boxes.GetBoxScale());
        std::format_to(std::back_inserter(s), "{}", "[");
        for (auto i = boxes.GetBoxList().size(); const auto &box : boxes.GetBoxList())
        {
            std::format_to(std::back_inserter(s), "{}{}", box.format("f"), (--i > 0 ? ", " : ""));
        }
        std::format_to(std::back_inserter(s), "{}", "]");
        return formatter<std::string>::format(s, ctx);
    }
};

inline std::ostream &operator<<(std::ostream &os, const Boxes &boxes)
{
    std::format_to(std::ostream_iterator<char>{os}, "{}", boxes);

    return os;
}

#endif // ----- #ifndef BOXES_INC  -----
       //
