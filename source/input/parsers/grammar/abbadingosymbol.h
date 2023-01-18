//
// Created by tom on 1/13/23.
//

#ifndef FLEXFRINGE_ABBADINGOSYMBOL_H
#define FLEXFRINGE_ABBADINGOSYMBOL_H

#include <lexy/dsl/literal.hpp>
#include <lexy/action/parse.hpp> // lexy::parse
#include <lexy/callback.hpp>     // value callbacks
#include <lexy/dsl.hpp>          // lexy::dsl::*

#include <string>
#include <utility>
#include <vector>
#include <set>
#include <iostream>


struct abbadingo_symbol_info {
    std::string_view name;
    std::optional<std::vector<std::string_view>> attribute_values;
    std::optional<std::string_view> data;
};

struct abbadingo_trace_specifier_info {
    uint64_t number;
    std::optional<std::vector<std::string_view>> attribute_values;
    std::optional<std::string_view> data;
};

struct abbadingo_trace_info {
    std::string_view label;
    // Symbol info functions as holder for trace info since they are parsed the same way
    abbadingo_trace_specifier_info trace_info;
    std::vector<abbadingo_symbol_info> symbols;
};

namespace {
    namespace symbol_grammar {
        namespace dsl = lexy::dsl;

        constexpr auto ws = dsl::whitespace(dsl::ascii::blank);

        struct number {
            static constexpr auto rule = dsl::integer<std::uint64_t>;
            static constexpr auto value = lexy::as_integer<std::uint64_t>;
        };

        struct name {
            static constexpr auto rule = dsl::identifier(dsl::ascii::alpha_digit_underscore);
            static constexpr auto value = lexy::as_string<std::string_view>;
        };

        // Attribute value - convert to double
        struct attr_val_double {
            static constexpr auto rule = dsl::capture(dsl::digits<>) + dsl::period + dsl::capture(dsl::digits<>);
            static constexpr auto value = lexy::callback<double>([](auto integer, auto decimal) {
                std::string tmp(integer.begin(), integer.end());
                tmp.append(".");
                tmp.append(decimal.begin(), decimal.end());
                return std::stod(tmp);
            });
        };

        // Attribute value - keep as string
        struct attr_val_str {
            static constexpr auto rule = dsl::capture(dsl::digits<>) + dsl::period + dsl::capture(dsl::digits<>);
            static constexpr auto value = lexy::callback<std::string_view>([](auto integer, auto decimal) {
                std::string_view tmp(integer.begin(), decimal.end());
                return tmp;
            });
        };

        // Attribute value list
        struct attr_val_list {
            static constexpr auto rule = LEXY_LIT(":") + dsl::list(dsl::p<attr_val_str>, dsl::sep(dsl::comma));
            static constexpr auto value = lexy::as_list<std::vector<std::string_view>>;
        };

        // Data
        struct data {
            static constexpr auto rule = LEXY_LIT("/") + dsl::identifier(dsl::ascii::alpha_digit_underscore);
            static constexpr auto value = lexy::as_string<std::string_view>;
        };

        // The actual symbols in the trace: symbol_name:1.0,2.0/foo
        struct symbol {
            static constexpr auto rule = [] {
                auto lookahead_attr = dsl::lookahead(LEXY_LIT(":"), dsl::literal_set(LEXY_LIT(" "), LEXY_LIT("\n")));
                auto lookahead_data = dsl::lookahead(LEXY_LIT("/"), dsl::literal_set(LEXY_LIT(" "), LEXY_LIT("\n")));
                return  (dsl::p<name>
                        + (lookahead_attr >> dsl::p<attr_val_list> | dsl::else_ >> dsl::nullopt)
                        + (lookahead_data >> dsl::p<data> | dsl::else_ >> dsl::nullopt));
            }();

            static constexpr auto value = lexy::callback<abbadingo_symbol_info>([](auto name, auto attr_val_list, auto data) {
                return abbadingo_symbol_info {
                    .name = name,
                    .attribute_values = attr_val_list,
                    .data = data
                };
            });
        };

        // Like a symbol, but for declaring the trace length and attributes: 10:1.0,2.0/foo
        struct trace_specifier {
            static constexpr auto rule = [] {
                auto lookahead_attr = dsl::lookahead(LEXY_LIT(":"), dsl::literal_set(LEXY_LIT(" "), LEXY_LIT("\n")));
                auto lookahead_data = dsl::lookahead(LEXY_LIT("/"), dsl::literal_set(LEXY_LIT(" "), LEXY_LIT("\n")));
                return  (dsl::p<number>
                         + (lookahead_attr >> dsl::p<attr_val_list> | dsl::else_ >> dsl::nullopt)
                         + (lookahead_data >> dsl::p<data> | dsl::else_ >> dsl::nullopt));
            }();

            static constexpr auto value = lexy::callback<abbadingo_trace_specifier_info>([](auto number, auto attr_val_list, auto data) {
                return abbadingo_trace_specifier_info {
                        .number = number,
                        .attribute_values = attr_val_list,
                        .data = data
                };
            });
        };

        struct symbol_list {
            static constexpr auto rule = dsl::list(dsl::p<symbol>, dsl::sep(dsl::ascii::space));
            static constexpr auto value = lexy::as_list<std::vector<abbadingo_symbol_info>>;
        };

        struct trace_label {
            static constexpr auto rule = dsl::identifier(dsl::ascii::alpha_digit_underscore);
            static constexpr auto value = lexy::as_string<std::string_view>;
        };

        struct abbadingo_trace {
            static constexpr auto rule = [] {
                auto trace_info_part = dsl::p<trace_label> + ws + dsl::p<trace_specifier>;
                auto symbol_list_part = ws + dsl::p<symbol_list>;
                auto condition = dsl::peek(symbol_list_part);
                return trace_info_part + dsl::opt(condition >> symbol_list_part);
            }();

            static constexpr auto value = lexy::callback<abbadingo_trace_info>(
                    [] (auto trace_label, auto trace_info, const std::optional<std::vector<abbadingo_symbol_info>>& symbols_list_maybe) {

                        auto symbols_list = symbols_list_maybe.value_or(std::vector<abbadingo_symbol_info> {});

                        return abbadingo_trace_info{
                                .label = trace_label,
                                .trace_info = trace_info,
                                .symbols = symbols_list
                        };
                    }
            );
        };
    }
}



#endif //FLEXFRINGE_ABBADINGOSYMBOL_H
