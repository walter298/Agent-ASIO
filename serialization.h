#pragma once

#include <concepts>
#include <ranges>
#include <string>
#include <string_view>

#include <boost/lexical_cast.hpp>

#include "detail/namespace_util.h"

namespace netlib {
    constexpr inline std::string_view MSG_END_DELIM{ "###" };
    constexpr inline std::string_view MSG_ERROR{ "ERROR" };
    constexpr inline char DATUM_END_CHR = '!';
    constexpr inline char DATUM_BREAK_CHR = '$';
    constexpr inline char DATUM_ELEM_BREAK_CHR = '&';

    template<typename T>
    concept Aggregate = std::is_aggregate_v<std::remove_cvref_t<T>>;

    namespace detail {
        using string_it = std::string::const_iterator;
        std::string next_datum(string_it& begin, string_it end);

        template<Aggregate T, size_t... MemberIdxs>
        void parse_each_member(std::string& data, T& aggr, std::index_sequence<MemberIdxs...> seq) {
            auto begin = data.begin();
            ((parseValueFromString(next_datum(begin, data.end()), pfr::get<MemberIdxs>(aggr))), ...);
        }
    }

    template<Aggregate T> 
    void parse_value_from_string(std::string& str, T& aggr) {
        detail::parse_each_member(str, aggr, std::make_index_sequence<pfr::tuple_size_v<T>>());
    }

    namespace detail {
        template<typename Callable, Aggregate T, size_t... MemberIdxs>
        decltype(auto) aggregate_apply_impl(Callable&& callable, T&& aggr, std::index_sequence<MemberIdxs...> seq) {
            return std::invoke(std::forward<Callable>(callable), pfr::get<MemberIdxs>(std::forward<T>(aggr))...);
        }
    }

    template<typename Callable, Aggregate T>
    decltype(auto) aggregate_apply(Callable&& callable, T&& aggr) {
        return detail::aggregate_apply_impl(std::forward<Callable>(callable), std::forward<T>(aggr),
            std::make_index_sequence<pfr::tuple_size_v<std::remove_cvref_t<T>>>());
    }

    template<typename t>
    concept Numeric = std::integral<t> || std::floating_point<t>;

    template<Numeric Num>
    void parse_value_from_string(const std::string& str, Num& integral) {
        integral = boost::lexical_cast<Num>(str);
    }

    template<std::ranges::viewable_range Range>
    void parse_value_from_string(const std::string& str, Range& range) 
        requires(!std::convertible_to<Range, std::string>) 
    {
        auto begin = str.begin();
        while (true) {
            auto nextElemBreak = std::ranges::find(begin, str.end(), DATUM_ELEM_BREAK_CHR);
            if (nextElemBreak == str.end()) {
                break;
            }
            typename Range::element_type elem{};
            range.insert(range.end(), parse_value_from_string(str, elem));
        }
    }
    template<std::convertible_to<std::string> String>
    void parse_value_from_string(const std::string& str, String& other) {
        other = str;
    }

    template<typename T>
    concept Enum = std::is_enum_v<std::remove_cvref_t<T>>;

    template<Enum Enm>
    void parse_value_from_string(const std::string& str, Enm& enm) {
        using underlying_type = std::underlying_type_t<enm>;
        underlying_type num{};
        parse_value_from_string(str, num);
        enm = static_cast<Enm>(num);
    }

    template<std::ranges::viewable_range Buffer, typename... Args>
    bool read_msg(const Buffer& buffer, Args&... args) requires(sizeof...(args) > 0) {
        auto buff_begin = std::begin(buffer);

        namespace rngs = std::ranges;
        auto parse_str = [&](auto& arg) {
            if (buff_begin == std::end(buffer)) {
                return false;
            }
            auto msg_end = rngs::find(buff_begin, std::end(buffer), DATUM_END_CHR);
            if (msg_end == std::end(buffer)) {
                return false;
            }
            
            auto next_buff_begin = std::next(msg_end);
            std::string str{ std::make_move_iterator(buff_begin), std::make_move_iterator(msg_end) };
            parse_value_from_string(str, arg);
            buff_begin = next_buff_begin;

            return true;
        };

        return (parse_str(args) && ...);
    }

    template<std::integral Num>
    std::string to_string(Num num) {
        return std::to_string(num);
    }
    template<std::convertible_to<std::string> String>
    decltype(auto) to_string(String&& str) { //no need to parse string
        return std::forward<String>(str);
    }
    template<std::ranges::viewable_range Range>
    auto to_string(const Range& range) {
        std::string ret;
        for (const auto& elem : range) {
            ret.append(to_string(elem) + '!');
        }
        return ret;
    }

    template<Enum Enm>
    auto to_string(Enm enm) {
        using UnderlyingType = std::underlying_type_t<Enm>;
        return to_string(static_cast<UnderlyingType>(enm));
    }

    template<typename... Args>
    std::string write_msg(Args&&... args) {
        std::string buffer;
        ((buffer.append(to_string(std::forward<Args>(args)) + '!')), ...);
        buffer.append(MSG_END_DELIM);
        return buffer;
    }
}