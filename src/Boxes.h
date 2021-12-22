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

#include <deque>

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

    // ====================  LIFECYCLE     ======================================= 
    Boxes () = default;                             // constructor 

    Boxes (DprDecimal::DDecQuad box_size, BoxType box_type=BoxType::e_integral, BoxScale box_scale=BoxScale::e_linear);

    // ====================  ACCESSORS     ======================================= 

    [[nodiscard]] DprDecimal::DDecQuad GetBoxsize() const { return box_size_; }
    [[nodiscard]] BoxType GetBoxType() const { return box_type_; }
    [[nodiscard]] BoxScale GetBoxScale() const { return box_scale_; }

    [[nodiscard]] const BoxList& GetBoxList() const { return boxes; }

    // ====================  MUTATORS      ======================================= 

    Box FindBox(const DprDecimal::DDecQuad& new_value);

    // ====================  OPERATORS     ======================================= 

protected:
    // ====================  METHODS       ======================================= 

    // ====================  DATA MEMBERS  ======================================= 

private:
    // ====================  METHODS       ======================================= 

    Box FirstBox(const DprDecimal::DDecQuad& start_at);
    [[nodiscard]] Box RoundDownToNearestBox(const DprDecimal::DDecQuad& a_value) const;

    // ====================  DATA MEMBERS  ======================================= 

    BoxList boxes;

    DprDecimal::DDecQuad box_size_ = -1;
    BoxType box_type_ = BoxType::e_integral;      // whether to drop fractional part of new values.
    BoxScale box_scale_ = BoxScale::e_linear;

}; // -----  end of class Boxes  ----- 

#endif   // ----- #ifndef BOXES_INC  ----- 
