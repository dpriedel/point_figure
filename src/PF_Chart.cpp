// =====================================================================================
//
//       Filename:  PF_Chart.cpp
//
//    Description:  Implementation of class which contains Point & Figure data
//    for a symbol.
//
//        Version:  2.0
//        Created:  2021-07-29 08:47 AM
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

#include <date/date.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

namespace rng = std::ranges;
namespace vws = std::ranges::views;

#include <date/chrono_io.h>
#include <date/tz.h>
#include <spdlog/spdlog.h>

using namespace std::string_literals;

#include "PF_Chart.h"
#include "PF_Column.h"
#include "PF_Signals.h"
#include "utilities.h"

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart(const PF_Chart &rhs)
    : boxes_{rhs.boxes_},
      signals_{rhs.signals_},
      columns_{rhs.columns_},
      current_column_{rhs.current_column_},
      symbol_{rhs.symbol_},
      chart_base_name_{rhs.chart_base_name_},
      base_box_size_{rhs.base_box_size_},
      fname_box_size_{rhs.fname_box_size_},
      box_size_modifier_{rhs.box_size_modifier_},
      first_date_{rhs.first_date_},
      last_change_date_{rhs.last_change_date_},
      last_checked_date_{rhs.last_checked_date_},
      y_min_{rhs.y_min_},
      y_max_{rhs.y_max_},
      current_direction_{rhs.current_direction_},
      max_columns_for_graph_{rhs.max_columns_for_graph_}

{
    // now, the reason for doing this explicitly is to fix the column box
    // pointers.

    rng::for_each(columns_, [this](auto &col) { col.boxes_ = &this->boxes_; });
    current_column_.boxes_ = &boxes_;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart(PF_Chart &&rhs) noexcept
    : boxes_{std::move(rhs.boxes_)},
      signals_{std::move(rhs.signals_)},
      columns_{std::move(rhs.columns_)},
      current_column_{std::move(rhs.current_column_)},
      symbol_{std::move(rhs.symbol_)},
      chart_base_name_{std::move(rhs.chart_base_name_)},
      base_box_size_{std::move(rhs.base_box_size_)},
      fname_box_size_{std::move(rhs.fname_box_size_)},
      box_size_modifier_{std::move(rhs.box_size_modifier_)},
      first_date_{rhs.first_date_},
      last_change_date_{rhs.last_change_date_},
      last_checked_date_{rhs.last_checked_date_},
      y_min_{std::move(rhs.y_min_)},
      y_max_{std::move(rhs.y_max_)},
      current_direction_{rhs.current_direction_},
      max_columns_for_graph_{rhs.max_columns_for_graph_}

{
    // now, the reason for doing this explicitly is to fix the column box
    // pointers.

    rng::for_each(columns_, [this](auto &col) { col.boxes_ = &this->boxes_; });
    current_column_.boxes_ = &boxes_;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------

PF_Chart::PF_Chart(std::string symbol, decimal::Decimal base_box_size, int32_t reversal_boxes,
                   decimal::Decimal box_size_modifier, BoxScale box_scale, int64_t max_columns_for_graph)
    : symbol_{std::move(symbol)},
      base_box_size_{std::move(base_box_size)},
      fname_box_size_{box_size_modifier == decimal::Decimal{0} ? base_box_size_ : box_size_modifier},
      box_size_modifier_{std::move(box_size_modifier)},
      max_columns_for_graph_{max_columns_for_graph}

{
    // std::print("params at chart start: {}, {}\n", base_box_size_,
    // box_size_modifier_);

    // std::print("params at chart start2: {}, {}, {}\n", base_box_size_,
    // box_size_modifier_, fname_box_size_);

    // stock prices are listed to 2 decimals.  If we are doing integral scale,
    // then we limit box size to that.

    boxes_ = Boxes{base_box_size_, box_size_modifier_, box_scale};
    current_column_ = PF_Column(&boxes_, columns_.size(), reversal_boxes);

    // std::print("Boxes: {}\n", boxes_);
    chart_base_name_ = MakeChartBaseName();

}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Chart::PF_Chart(const Json::Value &new_data)
{
    if (!new_data.empty())
    {
        this->FromJSON(new_data);
    }
    else
    {
        spdlog::debug("Trying to construct PF_Chart from empty JSON value.");
    }
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Chart PF_Chart::LoadChartFromChartsDB(const PF_DB &chart_db, PF_ChartParams vals, std::string_view interval)
{
    Json::Value chart_data = chart_db.GetPFChartData(MakeChartNameFromParams(vals, interval, "json"));
    PF_Chart chart_from_db{chart_data};
    return chart_from_db;
}  // -----  end of method PF_Chart::PF_Chart  (constructor)  -----

//--------------------------------------------------------------------------------------
//       Class:  PF_Chart
//      Method:  PF_Chart
// Description:  constructor
//--------------------------------------------------------------------------------------
PF_Chart PF_Chart::LoadChartFromJSONPF_ChartFile(const fs::path &file_name)
{
    Json::Value chart_data = ReadAndParsePF_ChartJSONFile(file_name);
    PF_Chart chart_from_file{chart_data};
    return chart_from_file;
}  // -----  end of method PF_Chart::MakeChartFromJSONFile  (constructor)  -----

PF_Chart &PF_Chart::operator=(const PF_Chart &rhs)
{
    if (this != &rhs)
    {
        boxes_ = rhs.boxes_;
        signals_ = rhs.signals_;
        columns_ = rhs.columns_;
        current_column_ = rhs.current_column_;
        symbol_ = rhs.symbol_;
        chart_base_name_ = rhs.chart_base_name_;
        base_box_size_ = rhs.base_box_size_;
        fname_box_size_ = rhs.fname_box_size_;
        box_size_modifier_ = rhs.box_size_modifier_;
        first_date_ = rhs.first_date_;
        last_change_date_ = rhs.last_change_date_;
        last_checked_date_ = rhs.last_checked_date_;
        y_min_ = rhs.y_min_;
        y_max_ = rhs.y_max_;
        current_direction_ = rhs.current_direction_;
        max_columns_for_graph_ = rhs.max_columns_for_graph_;

        // now, the reason for doing this explicitly is to fix the column box
        // pointers.

        rng::for_each(columns_, [this](auto &col) { col.boxes_ = &this->boxes_; });
        current_column_.boxes_ = &boxes_;
    }
    return *this;
}  // -----  end of method PF_Chart::operator=  -----

PF_Chart &PF_Chart::operator=(PF_Chart &&rhs) noexcept
{
    if (this != &rhs)
    {
        boxes_ = std::move(rhs.boxes_);
        signals_ = std::move(rhs.signals_);
        columns_ = std::move(rhs.columns_);
        current_column_ = std::move(rhs.current_column_);
        symbol_ = rhs.symbol_;
        chart_base_name_ = rhs.chart_base_name_;
        base_box_size_ = rhs.base_box_size_;
        fname_box_size_ = rhs.fname_box_size_;
        box_size_modifier_ = rhs.box_size_modifier_;
        first_date_ = rhs.first_date_;
        last_change_date_ = rhs.last_change_date_;
        last_checked_date_ = rhs.last_checked_date_;
        y_min_ = std::move(rhs.y_min_);
        y_max_ = std::move(rhs.y_max_);
        current_direction_ = rhs.current_direction_;
        max_columns_for_graph_ = rhs.max_columns_for_graph_;

        // now, the reason for doing this explicitly is to fix the column box
        // pointers.

        rng::for_each(columns_, [this](auto &col) { col.boxes_ = &this->boxes_; });
        current_column_.boxes_ = &boxes_;
    }
    return *this;
}  // -----  end of method PF_Chart::operator=  -----

PF_Chart &PF_Chart::operator=(const Json::Value &new_data)
{
    this->FromJSON(new_data);
    return *this;
}  // -----  end of method PF_Chart::operator=  -----

bool PF_Chart::operator==(const PF_Chart &rhs) const
{
    if (symbol_ != rhs.symbol_)
    {
        return false;
    }
    if (GetChartBoxSize() != rhs.GetChartBoxSize())
    {
        return false;
    }
    if (GetReversalboxes() != rhs.GetReversalboxes())
    {
        return false;
    }
    if (y_min_ != rhs.y_min_)
    {
        return false;
    }
    if (y_max_ != rhs.y_max_)
    {
        return false;
    }
    if (current_direction_ != rhs.current_direction_)
    {
        return false;
    }
    if (GetBoxType() != rhs.GetBoxType())
    {
        return false;
    }

    if (GetBoxScale() != rhs.GetBoxScale())
    {
        return false;
    }

    // if we got here, then we can look at our data

    if (columns_ != rhs.columns_)
    {
        return false;
    }
    if (current_column_ != rhs.current_column_)
    {
        return false;
    }

    return true;
}  // -----  end of method PF_Chart::operator==  -----

bool PF_Chart::HasReversedColumns() const
{
    return rng::find_if(*this, [](const auto &col) { return col.GetHadReversal(); }) != this->end();
}  // -----  end of method PF_Chart::HasReversedColumns  -----

PF_Column::Status PF_Chart::AddValue(const decimal::Decimal &new_value, PF_Column::TmPt the_time)
{
    // when extending the chart, don't add 'old' data.

    if (!empty())
    {
        if (the_time <= last_checked_date_)
        {
            return PF_Column::Status::e_Ignored;
        }
    }
    else
    {
        first_date_ = the_time;
    }

    auto [status, new_col] = current_column_.AddValue(new_value, the_time);

    if (status == PF_Column::Status::e_Accepted)
    {
        if (current_column_.GetTop() > y_max_)
        {
            y_max_ = current_column_.GetTop();
        }
        if (current_column_.GetBottom() < y_min_)
        {
            y_min_ = current_column_.GetBottom();
        }
        last_change_date_ = the_time;

        // if (auto had_signal = PF_Chart::LookForSignals(*this, new_value,
        // the_time); had_signal)
        if (auto found_signal = LookForNewSignal(*this, new_value, the_time); found_signal)
        {
            AddSignal(found_signal.value());
            status = PF_Column::Status::e_AcceptedWithSignal;
        }
    }
    else if (status == PF_Column::Status::e_Reversal)
    {
        columns_.push_back(current_column_);
        current_column_ = std::move(new_col.value());

        // now continue on processing the value.

        status = current_column_.AddValue(new_value, the_time).first;
        last_change_date_ = the_time;

        // if (auto found_signal = PF_Chart::LookForSignals(*this, new_value,
        // the_time); found_signal)
        if (auto found_signal = LookForNewSignal(*this, new_value, the_time); found_signal)
        {
            AddSignal(found_signal.value());
            status = PF_Column::Status::e_AcceptedWithSignal;
        }
    }
    current_direction_ = current_column_.GetDirection();
    last_checked_date_ = the_time;
    return status;
}  // -----  end of method PF_Chart::AddValue  -----

std::optional<StreamedPrices> PF_Chart::BuildChartFromCSVStream(std::istream *input_data, std::string_view date_format,
                                                                std::string_view delim,
                                                                PF_CollectAndReturnStreamedPrices return_streamed_data)
{
    StreamedPrices streamed_prices;

    std::string buffer;
    while (!input_data->eof())
    {
        buffer.clear();
        std::getline(*input_data, buffer);
        if (input_data->fail())
        {
            continue;
        }
        auto fields = split_string<std::string_view>(buffer, delim);
        decimal::Decimal new_value{std::string{fields[1]}};
        auto timept = StringToUTCTimePoint(date_format, fields[0]);

        auto chart_changed = AddValue(new_value, timept);

        if (return_streamed_data == PF_CollectAndReturnStreamedPrices::e_yes)
        {
            streamed_prices.timestamp_seconds_.push_back(
                std::chrono::duration_cast<std::chrono::seconds>(timept.time_since_epoch()).count());
            streamed_prices.price_.push_back(dec2dbl(new_value));
            streamed_prices.signal_type_.push_back(chart_changed == PF_Column::Status::e_AcceptedWithSignal
                                                       ? std::to_underlying(GetSignals().back().signal_type_)
                                                       : 0);
        }
    }

    // ??? redundant ??
    // // make sure we keep the last column we were working on
    //
    // if (current_column_.GetTop() > y_max_)
    // {
    //     y_max_ = current_column_.GetTop();
    // }
    // if (current_column_.GetBottom() < y_min_)
    // {
    //     y_min_ = current_column_.GetBottom();
    // }
    // current_direction_ = current_column_.GetDirection();

    if (return_streamed_data == PF_CollectAndReturnStreamedPrices::e_yes)
    {
        return streamed_prices;
    }
    return {};
}  // -----  end of method PF_Chart::BuildChartFromCSVStream  -----

std::optional<StreamedPrices> PF_Chart::BuildChartFromCSVFile(const std::string &file_name,
                                                              std::string_view date_format, std::string_view delim,
                                                              PF_CollectAndReturnStreamedPrices return_streamed_data)
{
    std::ifstream data_file{fs::path{file_name}, std::ios::in | std::ios::binary};
    BOOST_ASSERT_MSG(data_file.is_open(), std::format("Unable to open data file: {}", file_name).c_str());

    const auto streamed_prices = BuildChartFromCSVStream(&data_file, date_format, delim, return_streamed_data);

    data_file.close();

    return streamed_prices;
}  // -----  end of method PF_Chart::BuildChartFromCSVFile  -----

std::optional<StreamedPrices> PF_Chart::BuildChartFromPricesDB(const PF_DB::DB_Params &db_params,
                                                               std::string_view symbol, std::string_view begin_date,
                                                               std::string_view end_date,
                                                               std::string_view price_fld_name,
                                                               PF_CollectAndReturnStreamedPrices return_streamed_data)
{
    StreamedPrices streamed_prices;

    // first, get ready to retrieve our data from DB.

    PF_DB prices_db{db_params};
    pqxx::connection c{std::format("dbname={} user={}", db_params.db_name_, db_params.user_name_)};

    std::string date_range = end_date.empty()
                                 ? std::format("date >= {}", c.quote(begin_date))
                                 : std::format("date BETWEEN {} and {}", c.quote(begin_date), c.quote(end_date));

    std::string get_symbol_prices_cmd =
        std::format("SELECT date, {} FROM {} WHERE symbol = {} AND {} ORDER BY date ASC", price_fld_name,
                    db_params.stock_db_data_source_, c.quote(symbol), date_range);

    // right now, DB only has eod data.

    const auto *dt_format = "%F";

    std::istringstream time_stream;
    date::utc_time<std::chrono::utc_clock::duration> tp;

    // we know our database contains 'date's, but we need timepoints.
    // we'll handle that in the conversion routine below.

    auto Row2Closing = [dt_format, &time_stream, &tp](const auto &r)
    {
        time_stream.clear();
        time_stream.str(std::string{std::get<0>(r)});
        date::from_stream(time_stream, dt_format, tp);
        std::chrono::utc_time<std::chrono::utc_clock::duration> tp1{tp.time_since_epoch()};
        DateCloseRecord new_data{.date_ = tp1, .close_ = decimal::Decimal{std::get<1>(r)}};
        return new_data;
    };

    try
    {
        const auto closing_prices = prices_db.RunSQLQueryUsingStream<DateCloseRecord, std::string_view, const char *>(
            get_symbol_prices_cmd, Row2Closing);

        for (const auto &[new_date, new_price] : closing_prices)
        {
            // std::cout << "new value: " << new_price << "\t" <<
            // new_date << std::endl;
            auto chart_changed = AddValue(new_price, std::chrono::clock_cast<std::chrono::utc_clock>(new_date));
            if (return_streamed_data == PF_CollectAndReturnStreamedPrices::e_yes)
            {
                streamed_prices.timestamp_seconds_.push_back(
                    std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count());
                streamed_prices.price_.push_back(dec2dbl(new_price));
                streamed_prices.signal_type_.push_back(chart_changed == PF_Column::Status::e_AcceptedWithSignal
                                                           ? std::to_underlying(GetSignals().back().signal_type_)
                                                           : 0);
            }
        }
    }
    catch (const std::exception &e)
    {
        spdlog::error(
            std::format("Unable to load data for symbol chart: {} from DB "
                        "because: {}.",
                        MakeChartFileName("eod", ""), e.what()));
    }
    return {};
}  // -----  end of method PF_Chart::BuildChartFromPricesDB  -----

PF_Chart::ColumnBoxList PF_Chart::GetBoxesForColumns(PF_ColumnFilter which_columns) const
{
    ColumnBoxList result;

    auto column_filter = rng::views::filter(
        [&which_columns, this](const auto &col)
        {
            using enum PF_ColumnFilter;
            if (which_columns == e_up_column && col.GetDirection() == PF_Column::Direction::e_Up &&
                !col.GetHadReversal())
            {
                return true;
            }
            if (which_columns == e_down_column && col.GetDirection() == PF_Column::Direction::e_Down &&
                !col.GetHadReversal())
            {
                return true;
            }
            if (which_columns == e_reversed_to_up && col.GetReversalboxes() == 1 &&
                col.GetDirection() == PF_Column::Direction::e_Up && col.GetHadReversal())
            {
                return true;
            }
            if (which_columns == e_reversed_to_down && col.GetReversalboxes() == 1 &&
                col.GetDirection() == PF_Column::Direction::e_Down && col.GetHadReversal())
            {
                return true;
            }
            return false;
        });

    rng::for_each(*this | column_filter,
                  [&result](const auto &col)
                  {
                      auto col_nbr = col.GetColumnNumber();
                      rng::for_each(col.GetColumnBoxes(),
                                    [&result, &col, col_nbr](const auto &box) {
                                        result.push_back(std::pair{col_nbr, dec2dbl(box)});
                                    });
                  });

    return result;
}  // -----  end of method PF_Chart::GetBoxesForColumns  -----
//

PF_Chart::ColumnTopBottomList PF_Chart::GetTopBottomForColumns(PF_ColumnFilter which_columns) const
{
    ColumnTopBottomList result;

    auto column_filter = rng::views::filter(
        [&which_columns, this](const auto &col)
        {
            using enum PF_ColumnFilter;
            if (which_columns == e_up_column && col.GetDirection() == PF_Column::Direction::e_Up &&
                !col.GetHadReversal())
            {
                return true;
            }
            if (which_columns == e_down_column && col.GetDirection() == PF_Column::Direction::e_Down &&
                !col.GetHadReversal())
            {
                return true;
            }
            if (which_columns == e_reversed_to_up && col.GetReversalboxes() == 1 &&
                col.GetDirection() == PF_Column::Direction::e_Up && col.GetHadReversal())
            {
                return true;
            }
            if (which_columns == e_reversed_to_down && col.GetReversalboxes() == 1 &&
                col.GetDirection() == PF_Column::Direction::e_Down && col.GetHadReversal())
            {
                return true;
            }
            return false;
        });

    rng::for_each(*this | column_filter,
                  [&result, this](const auto &col)
                  {
                      auto col_nbr = col.GetColumnNumber();
                      auto bottom = dec2dbl(col.GetBottom());
                      auto top = col.GetTop();
                      auto top_for_chart = dec2dbl(boxes_.FindNextBox(top));
                      result.emplace_back(
                          ColumnTopBottomInfo{.col_nbr_ = col_nbr, .col_top_ = top_for_chart, .col_bot_ = bottom});
                  });

    return result;
}  // -----  end of method PF_Chart::GetTopBottomForColumns  -----

std::string PF_Chart::MakeChartBaseName() const
{
    std::string chart_name =
        std::format("{}_{}{}X{}_{}", symbol_, fname_box_size_.format("f"), (IsPercent() ? "%" : ""), GetReversalboxes(),
                    (IsPercent() ? "percent" : "linear"));
    return chart_name;
}  // -----  end of method PF_Chart::ChartName  -----

std::string PF_Chart::MakeChartFileName(std::string_view interval, std::string_view suffix) const
{
    std::string chart_name =
        std::format("{}{}.{}", chart_base_name_, (!interval.empty() ? "_"s += interval : ""), suffix);
    return chart_name;
}  // -----  end of method PF_Chart::ChartName  -----

void PF_Chart::ConvertChartToJsonAndWriteToFile(const fs::path &output_filename) const
{
    std::ofstream out{output_filename, std::ios::out | std::ios::binary};
    BOOST_ASSERT_MSG(out.is_open(), std::format("Unable to open file: {} for chart output.", output_filename).c_str());
    ConvertChartToJsonAndWriteToStream(out);
    out.close();
}  // -----  end of method PF_Chart::ConvertChartToJsonAndWriteToFile  -----

void PF_Chart::ConvertChartToJsonAndWriteToStream(std::ostream &stream) const
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";  // compact printing and string formatting
    std::unique_ptr<Json::StreamWriter> const writer(builder.newStreamWriter());
    writer->write(this->ToJSON(), &stream);
    stream << std::endl;  // add lf and flush
}  // -----  end of method PF_Chart::ConvertChartToJsonAndWriteToStream  -----

void PF_Chart::ConvertChartToTableAndWriteToFile(const fs::path &output_filename, X_AxisFormat date_or_time) const
{
    std::ofstream out{output_filename, std::ios::out | std::ios::binary};
    BOOST_ASSERT_MSG(out.is_open(),
                     std::format("Unable to open file: {} for graphics data output.", output_filename).c_str());
    ConvertChartToTableAndWriteToStream(out);
    out.close();
}  // -----  end of method PF_Chart::ConvertChartToTableAndWriteToFile  -----

void PF_Chart::ConvertChartToTableAndWriteToStream(std::ostream &stream, X_AxisFormat date_or_time) const
{
    // generate a delimited 'csv' file for use by external programs
    // format is: date, open, low, high, close, color, color index
    // where 'color' means column direction:
    //  up ==> green
    //  down ==> red
    //  reverse up ==> blue
    //  reverse down ==> orange
    //  color index can be used with custom palettes such as in gnuplot
    //

    size_t skipped_columns =
        max_columns_for_graph_ < 1 || size() <= max_columns_for_graph_ ? 0 : size() - max_columns_for_graph_;

    constexpr auto row_template = "{}\t{}\t{}\t{}\t{}\t{}\n";

    auto compute_color = [](const PF_Column &c)
    {
        if (c.GetDirection() == PF_Column::Direction::e_Up)
        {
            return (c.GetHadReversal() ? "blue\t3" : "green\t1");
        }
        if (c.GetDirection() == PF_Column::Direction::e_Down)
        {
            return (c.GetHadReversal() ? "orange\t2" : "red\t0");
        }
        return "black\t4";
    };

    // we'll provide a header record

    std::string header_record{"date\topen\tlow\thigh\tclose\tcolor\tindex\n"};
    stream.write(header_record.data(), header_record.size());

    auto filter_skipped_cols = *this | vws::drop(skipped_columns);
    for (const auto &col : filter_skipped_cols)
    {
        auto next_row = std::format(
            row_template,
            date_or_time == X_AxisFormat::e_show_date ? std::format("{:%F}", col.GetTimeSpan().first)
                                                      : UTCTimePointToLocalTZHMSString(col.GetTimeSpan().first),
            col.GetDirection() == PF_Column::Direction::e_Up ? col.GetBottom().format("f") : col.GetTop().format("f"),
            col.GetBottom().format("f"), col.GetTop().format("f"),
            col.GetDirection() == PF_Column::Direction::e_Up ? col.GetTop().format("f") : col.GetBottom().format("f"),
            compute_color(col));
        stream.write(next_row.data(), next_row.size());
    }

}  // -----  end of method PF_Chart::ConvertChartToTableAndWriteToStream -----

void PF_Chart::StoreChartInChartsDB(const PF_DB &chart_db, std::string_view interval, X_AxisFormat date_or_time,
                                    bool store_cvs_graphics) const
{
    std::string cvs_graphics;
    if (store_cvs_graphics)
    {
        std::ostringstream oss{};
        ConvertChartToTableAndWriteToStream(oss, date_or_time);
        cvs_graphics = oss.str();
    }
    chart_db.StorePFChartDataIntoDB(*this, interval, cvs_graphics);
}  // -----  end of method PF_Chart::StoreChartInChartsDB  -----

void PF_Chart::UpdateChartInChartsDB(const PF_DB &chart_db, std::string_view interval, X_AxisFormat date_or_time,
                                     bool store_cvs_graphics) const
{
    std::string cvs_graphics;
    if (store_cvs_graphics)
    {
        std::ostringstream oss{};
        ConvertChartToTableAndWriteToStream(oss, date_or_time);
        cvs_graphics = oss.str();
    }
    chart_db.UpdatePFChartDataInDB(*this, interval, cvs_graphics);
}  // -----  end of method PF_Chart::StoreChartInChartsDB  -----

Json::Value PF_Chart::ToJSON() const
{
    Json::Value result;
    result["symbol"] = symbol_;
    result["base_name"] = chart_base_name_;
    result["boxes"] = boxes_.ToJSON();

    Json::Value signals{Json::arrayValue};
    for (const auto &sig : signals_)
    {
        signals.append(PF_SignalToJSON(sig));
    }
    result["signals"] = signals;

    result["first_date"] = first_date_.time_since_epoch().count();
    result["last_change_date"] = last_change_date_.time_since_epoch().count();
    result["last_check_date"] = last_checked_date_.time_since_epoch().count();

    result["base_box_size"] = base_box_size_.format("f");
    result["fname_box_size"] = fname_box_size_.format("f");
    result["box_size_modifier"] = box_size_modifier_.format("f");
    result["y_min"] = y_min_.format("f");
    result["y_max"] = y_max_.format("f");

    switch (current_direction_)
    {
        using enum PF_Column::Direction;
        case e_Unknown:
            result["current_direction"] = "unknown";
            break;

        case e_Down:
            result["current_direction"] = "down";
            break;

        case e_Up:
            result["current_direction"] = "up";
            break;
    };
    result["max_columns"] = max_columns_for_graph_;

    Json::Value cols{Json::arrayValue};
    for (const auto &col : columns_)
    {
        cols.append(col.ToJSON());
    }
    result["columns"] = cols;
    result["current_column"] = current_column_.ToJSON();

    return result;

}  // -----  end of method PF_Chart::ToJSON  -----

void PF_Chart::FromJSON(const Json::Value &new_data)
{
    symbol_ = new_data["symbol"].asString();
    chart_base_name_ = new_data["base_name"].asString();
    boxes_ = new_data["boxes"];

    const auto &signals = new_data["signals"];
    signals_.clear();
    rng::for_each(signals, [this](const auto &next_val) { this->signals_.push_back(PF_SignalFromJSON(next_val)); });

    first_date_ = PF_Column::TmPt{std::chrono::nanoseconds{new_data["first_date"].asInt64()}};
    last_change_date_ = PF_Column::TmPt{std::chrono::nanoseconds{new_data["last_change_date"].asInt64()}};
    last_checked_date_ = PF_Column::TmPt{std::chrono::nanoseconds{new_data["last_check_date"].asInt64()}};

    base_box_size_ = decimal::Decimal{new_data["base_box_size"].asCString()};
    fname_box_size_ = decimal::Decimal{new_data["fname_box_size"].asCString()};
    box_size_modifier_ = decimal::Decimal{new_data["box_size_modifier"].asCString()};

    y_min_ = decimal::Decimal{new_data["y_min"].asCString()};
    y_max_ = decimal::Decimal{new_data["y_max"].asCString()};

    const auto direction = new_data["current_direction"].asString();
    if (direction == "up")
    {
        current_direction_ = PF_Column::Direction::e_Up;
    }
    else if (direction == "down")
    {
        current_direction_ = PF_Column::Direction::e_Down;
    }
    else if (direction == "unknown")
    {
        current_direction_ = PF_Column::Direction::e_Unknown;
    }
    else
    {
        throw std::invalid_argument{
            std::format("Invalid direction provided: {}. Must be 'up', 'down', 'unknown'.", direction)};
    }

    max_columns_for_graph_ = new_data["max_columns"].asInt64();

    // lastly, we can do our columns
    // need to hook them up with current boxes_ data

    const auto &cols = new_data["columns"];
    columns_.clear();
    rng::for_each(cols, [this](const auto &next_val) { this->columns_.emplace_back(&boxes_, next_val); });

    current_column_ = PF_Column{&boxes_, new_data["current_column"]};
}  // -----  end of method PF_Chart::FromJSON  -----

// ===  FUNCTION
// ======================================================================
//         Name:  ComputeATR
//  Description:  Expects the input data is in descending order by date
// =====================================================================================

decimal::Decimal ComputeATR(std::string_view symbol, const std::vector<StockDataRecord> &the_data,
                            int32_t how_many_days, int32_t scale)
{
    BOOST_ASSERT_MSG(the_data.size() > how_many_days, std::format("Not enough data provided for: {}. Need at "
                                                                  "least: {} values. Got {}.",
                                                                  symbol, how_many_days, the_data.size())
                                                          .c_str());

    decimal::Decimal total{0};

    decimal::Decimal high_minus_low;
    decimal::Decimal high_minus_prev_close;
    decimal::Decimal low_minus_prev_close;

    for (int32_t i = 0; i < how_many_days; ++i)
    {
        high_minus_low = the_data[i].high_ - the_data[i].low_;
        high_minus_prev_close = (the_data[i].high_ - the_data[i + 1].close_).abs();
        low_minus_prev_close = (the_data[i].low_ - the_data[i + 1].close_).abs();

        decimal::Decimal max = high_minus_low.max(high_minus_prev_close.max(low_minus_prev_close));

        // std::cout << std::format("i: {} hml: {} hmpc: {} lmpc: {} max: {}\n", i, high_minus_low,
        // high_minus_prev_close,
        //                          low_minus_prev_close, max);
        total += max;
    }

    //    std::cout << "total: " << total << '\n';

    total /= how_many_days;
    if (scale > -99)
    {
        return total.rescale(scale);
    }
    return total.rescale(-3);
}  // -----  end of function ComputeATRUsingJSON  -----

std::string MakeChartNameFromParams(const PF_Chart::PF_ChartParams &vals, std::string_view interval,
                                    std::string_view suffix)
{
    std::string chart_name = std::format(
        "{}_{}{}X{}_{}{}.{}", std::get<PF_Chart::e_symbol>(vals), std::get<PF_Chart::e_box_size>(vals).format("f"),
        (std::get<PF_Chart::e_box_scale>(vals) == BoxScale::e_Percent ? "%" : ""), std::get<PF_Chart::e_reversal>(vals),
        (std::get<PF_Chart::e_box_scale>(vals) == BoxScale::e_Linear ? "linear" : "percent"),
        (!interval.empty() ? "_"s += interval : ""), suffix);
    return chart_name;
}  // -----  end of method MakeChartNameFromParams  -----

PF_Chart::iterator PF_Chart::begin() { return iterator{this}; }  // -----  end of method PF_Chart::begin  -----

PF_Chart::const_iterator PF_Chart::begin() const
{
    return const_iterator{this};
}  // -----  end of method PF_Chart::begin  -----

PF_Chart::iterator PF_Chart::end()
{
    return iterator(this, this->size());
}  // -----  end of method PF_Chart::end  -----

PF_Chart::const_iterator PF_Chart::end() const
{
    return const_iterator(this, this->size());
}  // -----  end of method PF_Chart::end  -----

bool PF_Chart::PF_Chart_Iterator::operator==(const PF_Chart_Iterator &rhs) const
{
    return (chart_ == rhs.chart_ && index_ == rhs.index_);
}  // -----  end of method PF_Chart::PF_Chart_Iterator::operator==  -----

PF_Chart::PF_Chart_Iterator &PF_Chart::PF_Chart_Iterator::operator++()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(++index_, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_Iterator::operator++  -----

PF_Chart::PF_Chart_Iterator &PF_Chart::PF_Chart_Iterator::operator+=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(index_ += n, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_Iterator::operator+=  -----

PF_Chart::PF_Chart_Iterator &PF_Chart::PF_Chart_Iterator::operator--()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_less(--index_, 0))
    {
        index_ = 0;
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_Iterator::operator--  -----

PF_Chart::PF_Chart_Iterator &PF_Chart::PF_Chart_Iterator::operator-=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_less(index_ -= n, 0))
    {
        index_ = 0;
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_Iterator::operator-+  -----

PF_Chart::reverse_iterator PF_Chart::rbegin()
{
    return reverse_iterator{this, static_cast<int32_t>(this->size() - 1)};
}  // -----  end of method PF_Chart::rbegin  -----

PF_Chart::const_reverse_iterator PF_Chart::rbegin() const
{
    return const_reverse_iterator{this, static_cast<int32_t>(this->size() - 1)};
}  // -----  end of method PF_Chart::rbegin  -----

PF_Chart::reverse_iterator PF_Chart::rend()
{
    return reverse_iterator(this, -1);
}  // -----  end of method PF_Chart::rend  -----

PF_Chart::const_reverse_iterator PF_Chart::rend() const
{
    return const_reverse_iterator(this, -1);
}  // -----  end of method PF_Chart::rend  -----

bool PF_Chart::PF_Chart_ReverseIterator::operator==(const PF_Chart_ReverseIterator &rhs) const
{
    return (chart_ == rhs.chart_ && index_ == rhs.index_);
}  // -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator==
   // -----

PF_Chart::PF_Chart_ReverseIterator &PF_Chart::PF_Chart_ReverseIterator::operator++()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (--index_ < 0)
    {
        index_ = -1;
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator++
   // -----

PF_Chart::PF_Chart_ReverseIterator &PF_Chart::PF_Chart_ReverseIterator::operator+=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_less(index_ -= n, 0))
    {
        // std::print("index in between: {}\n", index_);
        std::cout << std::format("index in between: {}\n", index_);
        index_ = -1;
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator+=
   // -----

PF_Chart::PF_Chart_ReverseIterator &PF_Chart::PF_Chart_ReverseIterator::operator--()
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(++index_, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator--
   // -----

PF_Chart::PF_Chart_ReverseIterator &PF_Chart::PF_Chart_ReverseIterator::operator-=(difference_type n)
{
    if (chart_ == nullptr)
    {
        return *this;
    }
    if (std::cmp_greater(index_ += n, chart_->size()))
    {
        index_ = chart_->size();
    }
    return *this;
}  // -----  end of method PF_Chart::PF_Chart_ReverseIterator::operator-+
   // -----
