//
// Created by tom on 1/4/23.
//

#ifndef FLEXFRINGE_CSVPARSER_H
#define FLEXFRINGE_CSVPARSER_H

#include "i_parser.h"
#include "csv.hpp"
#include "input/trace.h"
#include "symbol_info.h"
#include <coroutine>
#include <optional>

class csv_header_parser {
    using TypeName = std::string;
private:
    static const std::set<std::string> default_col_type_names;

    std::unordered_map<TypeName, std::set<int>> col_types;
    std::unordered_map<TypeName, std::vector<std::string>> col_names;

    std::set<std::string> col_type_names;

    void setup_col_maps();

    void parse(const std::vector<std::string> &headers);

public:
    explicit csv_header_parser(const std::vector<std::string> &headers);

    explicit csv_header_parser(const std::vector<std::string> &headers,
                               const std::set<std::string> &col_type_names);

    const std::set<int> &get(const std::string &type) const;

    const std::vector<std::string> &get_names(const std::string &type) const;

    const std::set<std::string> &get_column_type_names() const;
};

class csv_parser : parser {
    using ID = std::string;

private:
    std::unique_ptr<csv::CSVReader> reader;
    std::unique_ptr<csv_header_parser> header_parser;

public:
    explicit csv_parser(std::unique_ptr<csv::CSVReader> reader)
            : reader(std::move(reader)) {

        std::vector<std::string> col_names = this->reader->get_col_names();

        // TODO: maybe DI this?
        header_parser = std::make_unique<csv_header_parser>(col_names);
    }

    // This should actually be a std::generator but there is no compiler support for that yet :(
    std::optional<symbol_info> next();

    [[maybe_unused]]
    std::string get_str_from_row(const std::string &label, const csv::CSVRow &row);

    std::vector<std::string> get_vec_from_row(const std::string &label, const csv::CSVRow &row);
};

#endif //FLEXFRINGE_CSVPARSER_H
