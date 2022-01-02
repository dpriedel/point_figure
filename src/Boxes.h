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


#ifndef  BOXES_INC
#define  BOXES_INC

#include <cstdint>
#include <deque>
#include <json/json.h>

#include "DDecQuad.h"

// =====================================================================================
//        Class:  Boxes
//  Description:  Manage creation and use of P & F boxes 
// =====================================================================================
class Boxes
{
public:

    enum class BoxType { e_integral, e_fractional };
    enum class BoxScale { e_linear, e_percent };

    using Box = DprDecimal::DDecQuad;
    using BoxList = std::deque<Box>;
//    using BoxList = std::deque<Box>;

    // ====================  LIFECYCLE     ======================================= 
    Boxes () = default;                             // constructor 
    Boxes (const Boxes& rhs) = default;
    Boxes (Boxes&& rhs) = default;

    Boxes (const DprDecimal::DDecQuad& box_size, BoxType box_type=BoxType::e_integral, BoxScale box_scale=BoxScale::e_linear);

    Boxes(const Json::Value& new_data);

    // ====================  ACCESSORS     ======================================= 

    [[nodiscard]] DprDecimal::DDecQuad GetBoxsize() const { return box_size_; }
    [[nodiscard]] BoxType GetBoxType() const { return box_type_; }
    [[nodiscard]] BoxScale GetBoxScale() const { return box_scale_; }

    [[nodiscard]] const BoxList& GetBoxList() const { return boxes_; }

    [[nodiscard]] Json::Value ToJSON() const;

    [[nodiscard]] int32_t Distance(const Box& from, const Box& to) const;

	friend std::ostream& operator<<(std::ostream& os, const Boxes& boxes);

    // ====================  MUTATORS      ======================================= 

    Box FindBox(const DprDecimal::DDecQuad& new_value);
    Box FindNextBox(const DprDecimal::DDecQuad& current_value);
    Box FindPrevBox(const DprDecimal::DDecQuad& current_value);

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
    Box FindPrevBoxPercent(const DprDecimal::DDecQuad& current_value);
    [[nodiscard]] Box RoundDownToNearestBox(const DprDecimal::DDecQuad& a_value) const;

    // ====================  DATA MEMBERS  ======================================= 

    BoxList boxes_;

    DprDecimal::DDecQuad box_size_ = -1;
    DprDecimal::DDecQuad percent_box_factor_up_ = -1;
    DprDecimal::DDecQuad percent_box_factor_down_ = -1;

    int32_t percent_exponent_ = 0;
    BoxType box_type_ = BoxType::e_integral;      // whether to drop fractional part of new values.
    BoxScale box_scale_ = BoxScale::e_linear;

}; // -----  end of class Boxes  ----- 

inline std::ostream& operator<<(std::ostream& os, const Boxes::BoxType box_type)
{
    switch(box_type)
    {
        case Boxes::BoxType::e_integral:
            os << "integral";
            break;

        case Boxes::BoxType::e_fractional:
            os << "fractional";
            break;
    };

	return os;
}

inline std::ostream& operator<<(std::ostream& os, const Boxes::BoxScale box_scale)
{
    switch(box_scale)
    {
        case Boxes::BoxScale::e_linear:
            os << "linear";
            break;

        case Boxes::BoxScale::e_percent:
            os << "percent";
            break;
    };

	return os;
}

inline std::ostream& operator<<(std::ostream& os, const Boxes& boxes)
{
    os << "box size: " << boxes.box_size_ << " factor_up: " << boxes.percent_box_factor_up_ << " factor down: " << boxes.percent_box_factor_down_
        << " exponent: " << boxes.percent_exponent_ << " box type: " << boxes.box_type_ << " box scale: " << boxes.box_scale_ << '\n';
    os << "Box values [ ";
    for (const auto& box : boxes.boxes_)
    {
        os << box << ", ";
    }
    os << " ]\n";

    return os;
}

#endif   // ----- #ifndef BOXES_INC  ----- 
