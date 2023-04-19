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


#ifndef  BOXES_INC
#define  BOXES_INC

#include <cstdint>
#include <deque>
#include <json/json.h>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include "DDecQuad.h"

// =====================================================================================
//        Class:  Boxes
//  Description:  Manage creation and use of P & F boxes 
// =====================================================================================
class Boxes
{
public:

    static constexpr std::size_t MAX_BOXES = 1000;			// too many boxes and everything becomes too slow
    static constexpr int32_t MIN_EXPONENT = -5;
    										//
    enum class BoxType { e_integral, e_fractional };
    enum class BoxScale { e_linear, e_percent };

    using Box = DprDecimal::DDecQuad;
    using BoxList = std::deque<Box>;

    // ====================  LIFECYCLE     ======================================= 
    Boxes () = default;                             // constructor 
    Boxes (const Boxes& rhs) = default;
    Boxes (Boxes&& rhs) = default;

    explicit Boxes (DprDecimal::DDecQuad base_box_size, DprDecimal::DDecQuad box_size_modifier=0, BoxScale box_scale=BoxScale::e_linear);

    explicit Boxes(const Json::Value& new_data);

	~Boxes() = default;

    // ====================  ACCESSORS     ======================================= 

    [[nodiscard]] DprDecimal::DDecQuad GetBoxSize() const { return runtime_box_size_; }
    [[nodiscard]] BoxType GetBoxType() const { return box_type_; }
    [[nodiscard]] BoxScale GetBoxScale() const { return box_scale_; }
    [[nodiscard]] DprDecimal::DDecQuad GetScaleUpFactor() const { return percent_box_factor_up_; }
    [[nodiscard]] DprDecimal::DDecQuad GetScaleDownFactor() const { return percent_box_factor_down_; }
    [[nodiscard]] int32_t GetExponent() const { return percent_exponent_; }
    [[nodiscard]] size_t GetHowMany() const { return boxes_.size(); }

    [[nodiscard]] const BoxList& GetBoxList() const { return boxes_; }

    [[nodiscard]] Json::Value ToJSON() const;

    [[nodiscard]] size_t Distance(const Box& from, const Box& to) const;

    // ====================  MUTATORS      ======================================= 

    Box FindBox(const DprDecimal::DDecQuad& new_value);
    Box FindNextBox(const DprDecimal::DDecQuad& current_value);
    Box FindPrevBox(const DprDecimal::DDecQuad& current_value);

    // we have some lookup-only uses

    Box FindNextBox(const DprDecimal::DDecQuad& current_value) const;
    Box FindPrevBox(const DprDecimal::DDecQuad& current_value) const;

    // ====================  OPERATORS     ======================================= 

    bool operator == (const Boxes& rhs) const;
    bool operator != (const Boxes& rhs) const { return ! operator==(rhs); }

    Boxes& operator= (const Json::Value& new_data);

    Boxes& operator= (const Boxes& rhs) = default;
    Boxes& operator= (Boxes&& rhs) = default; 

protected:
    // ====================  METHODS       ======================================= 

    // ====================  DATA MEMBERS  ======================================= 

private:
    // ====================  METHODS       ======================================= 

    void FromJSON(const Json::Value& new_data);

    Box FirstBox(const DprDecimal::DDecQuad& start_at);
    Box FirstBoxPerCent(const DprDecimal::DDecQuad& start_at);
    Box FindBoxPercent(const DprDecimal::DDecQuad& new_value);
    Box FindNextBoxPercent(const DprDecimal::DDecQuad& current_value);
    Box FindNextBoxPercent(const DprDecimal::DDecQuad& current_value) const;
    Box FindPrevBoxPercent(const DprDecimal::DDecQuad& current_value);
    Box FindPrevBoxPercent(const DprDecimal::DDecQuad& current_value) const;
    [[nodiscard]] Box RoundDownToNearestBox(const DprDecimal::DDecQuad& a_value) const;

	// these functions implement our max number of boxes limit 
	
	void PushFront(Box new_box);
	void PushBack(Box new_box);

    // ====================  DATA MEMBERS  ======================================= 

    BoxList boxes_;

    DprDecimal::DDecQuad base_box_size_ = -1;
    DprDecimal::DDecQuad box_size_modifier_ = 0;
    DprDecimal::DDecQuad runtime_box_size_ = -1;
    DprDecimal::DDecQuad percent_box_factor_up_ = -1;
    DprDecimal::DDecQuad percent_box_factor_down_ = -1;

    int32_t percent_exponent_ = 0;
    BoxType box_type_ = BoxType::e_integral;      // whether to drop fractional part of new values.
    BoxScale box_scale_ = BoxScale::e_linear;

}; // -----  end of class Boxes  ----- 

// custom fmtlib formatter for BoxType

template <> struct fmt::formatter<Boxes::BoxType>: formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Boxes::BoxType& box_type, fmt::format_context& ctx) const
    {
        std::string s;
		fmt::format_to(std::back_inserter(s), "{}", (box_type == Boxes::BoxType::e_integral ? "integral" : "fractional"));
        return formatter<std::string>::format(s, ctx);
    }
};

// custom fmtlib formatter for BoxScale

template <> struct fmt::formatter<Boxes::BoxScale>: formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Boxes::BoxScale& box_scale, fmt::format_context& ctx) const
    {
        std::string s;
		fmt::format_to(std::back_inserter(s), "{}", (box_scale == Boxes::BoxScale::e_linear ? "linear" : "percent"));
        return formatter<std::string>::format(s, ctx);
    }
};

// inline std::ostream& operator<<(std::ostream& os, const Boxes::BoxType box_type)
// {
//     fmt::format_to(std::ostream_iterator<char>{os}, "{}", box_type);
//
// 	return os;
// }
//
// inline std::ostream& operator<<(std::ostream& os, const Boxes::BoxScale box_scale)
// {
//     fmt::format_to(std::ostream_iterator<char>{os}, "{}", box_scale);
//
// 	return os;
// }
//
template <> struct fmt::formatter<Boxes>: formatter<std::string>
{
    // parse is inherited from formatter<string>.
    auto format(const Boxes& boxes, fmt::format_context& ctx) const
    {
        std::string s;
    	fmt::format_to(std::back_inserter(s),
        	"Boxes: how many: {}. box size: {}. factor up: {}. factor down: {}. exponent: {}. box type: {}. box scale: {}.\n",
        	boxes.GetHowMany(), boxes.GetBoxSize(), boxes.GetScaleUpFactor(), boxes.GetScaleDownFactor(), boxes.GetExponent(), boxes.GetBoxType(), boxes.GetBoxScale());
    	fmt::format_to(std::back_inserter(s), "{}", "[");
    	for(auto i = boxes.GetBoxList().size(); const auto& box : boxes.GetBoxList())
    	{
			fmt::format_to(std::back_inserter(s), "{}{}", box, (--i > 0 ? ", " : ""));
    	}
    	fmt::format_to(std::back_inserter(s), "{}", "]");
        return formatter<std::string>::format(s, ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, const Boxes& boxes)
{
    fmt::format_to(std::ostream_iterator<char>{os}, "{}", boxes);

    return os;
}

#endif   // ----- #ifndef BOXES_INC  ----- 
         //
