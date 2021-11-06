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
//        Class:  PF_Chart
//  Description:  class to manage Point & Figure data for a symbol.
// =====================================================================================

#ifndef  PF_CHART_INC
#define  PF_CHART_INC


#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <date/date.h>

#include "utilities.h"

namespace fs = std::filesystem;

//using namespace std::chrono_literals;

#include "PF_Column.h"


class PF_Chart
{
public:

    using Y_Limits = std::pair<DprDecimal::DDecQuad, DprDecimal::DDecQuad>;

    // make it look like a range
    //TODO: use a custom iterator which will include current_column_ in 
    // the iteration.

//    using const_iterator = std::vector<PF_Column>::const_iterator;

    // ====================  LIFECYCLE     =======================================
    PF_Chart () = default;                             // constructor
    PF_Chart(const std::string& symbol, DprDecimal::DDecQuad box_size, int32_t reversal_boxes,
            PF_Column::FractionalBoxes fractional_boxes=PF_Column::FractionalBoxes::e_integral,
            bool use_logarithms=false);

    explicit PF_Chart(const Json::Value& new_data);

    // ====================  ACCESSORS     =======================================

    [[nodiscard]] PF_Column::Direction GetCurrentDirection() const { return current_direction_; }

    // need to include current_column_ 

    [[nodiscard]] std::size_t GetNumberOfColumns() const { return columns_.size() + 1; }

    [[nodiscard]] Y_Limits GetYLimits() const { return {y_min_, y_max_}; }

//    [[nodiscard]] const_iterator begin() const { return columns_.begin(); }
//    [[nodiscard]] const_iterator end() const { return columns_.end(); }

    void ConstructChartAndWriteToFile(const fs::path& output_filename) const;

    [[nodiscard]] Json::Value ToJSON() const;

    // ====================  MUTATORS      =======================================
    
    PF_Column::Status AddValue(const DprDecimal::DDecQuad& new_value, PF_Column::tpt the_time);
    void LoadData(std::istream* input_data, std::string_view date_format, char delim);

    // ====================  OPERATORS     =======================================

    PF_Chart& operator = (const Json::Value& new_data);

    bool operator== (const PF_Chart& rhs) const;
    bool operator!= (const PF_Chart& rhs) const { return ! operator==(rhs); }

    const PF_Column& operator[](size_t which) const { return (which < columns_.size() ? columns_[which] : current_column_); }
	friend std::ostream& operator<<(std::ostream& os, const PF_Chart& chart);

protected:
    // ====================  DATA MEMBERS  =======================================

private:

    void FromJSON(const Json::Value& new_data);

    // ====================  DATA MEMBERS  =======================================

    std::vector<PF_Column> columns_;
    PF_Column current_column_;

    std::string symbol_;

    PF_Column::tpt first_date_;			    //	earliest entry for symbol
    PF_Column::tpt last_change_date_;		//	date of last change to data
    PF_Column::tpt last_checked_date_;	    //	last time checked to see if update needed

    DprDecimal::DDecQuad box_size_ = -1;
    int32_t reversal_boxes_ = -1;
    DprDecimal::DDecQuad y_min_ = 100000;         // just a number
    DprDecimal::DDecQuad y_max_ = -1;

    PF_Column::Direction current_direction_ = PF_Column::Direction::e_unknown;
    PF_Column::FractionalBoxes fractional_boxes_ = PF_Column::FractionalBoxes::e_integral;

    bool use_logarithms_ = false;

}; // -----  end of class PF_Chart  -----

inline std::ostream& operator<<(std::ostream& os, const PF_Chart& chart)
{
    os << "chart for ticker: " << chart.symbol_ << " box size: " << chart.box_size_ << " reversal boxes: " << chart.reversal_boxes_<< '\n';
    for (const auto& col : chart.columns_)
    {
        os << '\t' << col << '\n';
    }
    os << '\t' << chart.current_column_ << '\n';
    os << "number of columns: " << chart.GetNumberOfColumns() << " min value: " << chart.y_min_ << " max value: " << chart.y_max_ << '\n';
    return os;
}

// In order to set our box size, we use the Average True Range method. For
// each symbol, we will look up the 'n' most recent historical values 
// starting with the given date and moving backward from 
// Tiingo and compute the ATR using the formula from Investopedia.

DprDecimal::DDecQuad ComputeATR(std::string_view symbol, const Json::Value& the_data, int32_t how_many_days, UseAdjusted use_adjusted=UseAdjusted::e_No);

#endif   // ----- #ifndef PF_CHART_INC  ----- 

