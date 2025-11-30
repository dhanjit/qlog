#ifndef _STRINGCT_HPP_
#define _STRINGCT_HPP_

#include <algorithm>
#include <type_traits>
#include <utility>

namespace common {
namespace stringct {

// C++20 CNTTP Helper
template <std::size_t N>
struct FixedString {
    char buf[N]{};
    constexpr FixedString(char const (&s)[N]) { std::copy_n(s, N, buf); }
    constexpr char operator[](std::size_t i) const { return buf[i]; }
    constexpr std::size_t size() const { return N - 1; }    // Exclude null terminator
};

template <char... chars>
struct StringCT {
    using type = StringCT<chars...>;
    static constexpr char str[sizeof...(chars) + 1] = {chars..., '\0'};
    // StringCT() = delete; // Allow default construction for usage in other templates if needed
};

template <char... chars>
constexpr char StringCT<chars...>::str[sizeof...(chars) + 1];

template <FixedString S, typename Idx>
struct StringLiteralToCTImpl;

template <FixedString S, std::size_t... I>
struct StringLiteralToCTImpl<S, std::index_sequence<I...>> {
    using type = StringCT<S[I]...>;
};

template <FixedString S>
using StringLiteralToCT = typename StringLiteralToCTImpl<S, std::make_index_sequence<S.size()>>::type;

// Macro replacement
#define SCT(str) ::common::stringct::StringLiteralToCT<str>

// Existing helpers (ConcatStringCT, FormatSpecifierCT, etc.) need to be preserved
// as they operate on StringCT<char...>

template <typename...>
struct ConcatStringCT;

template <char... c1, char... c2>
struct ConcatStringCT<StringCT<c1...>, StringCT<c2...>> : StringCT<c1..., c2...> {};

template <typename S1, typename S2, typename S3, typename... Args>
struct ConcatStringCT<S1, S2, S3, Args...> : ConcatStringCT<typename ConcatStringCT<S1, S2>::type, S3, Args...> {};

template <typename>
struct FormatSpecifierCT;
template <>
struct FormatSpecifierCT<double> : StringCT<'f'> {};
template <>
struct FormatSpecifierCT<float> : StringCT<'f'> {};
template <>
struct FormatSpecifierCT<int> : StringCT<'d'> {};
template <>
struct FormatSpecifierCT<unsigned> : StringCT<'u'> {};
template <>
struct FormatSpecifierCT<long> : StringCT<'l', 'd'> {};
template <>
struct FormatSpecifierCT<unsigned long> : StringCT<'l', 'u'> {};
template <>
struct FormatSpecifierCT<long long> : StringCT<'l', 'l', 'd'> {};
template <>
struct FormatSpecifierCT<unsigned long long> : StringCT<'l', 'l', 'u'> {};
template <>
struct FormatSpecifierCT<char *> : StringCT<'s'> {};
template <>
struct FormatSpecifierCT<char> : StringCT<'c'> {};
template <>
struct FormatSpecifierCT<bool> : StringCT<'d'> {};
template <typename T>
struct FormatSpecifierCT<const T *> : FormatSpecifierCT<T *> {};

template <typename...>
struct FormatStringCT;
template <>
struct FormatStringCT<> : StringCT<> {};
template <typename T>
struct FormatStringCT<T> : ConcatStringCT<StringCT<'%'>, typename FormatSpecifierCT<T>::type> {};

template <char delim, typename T, typename U, typename... Args>
struct FormatStringCT<StringCT<delim>, T, U, Args...> : ConcatStringCT<typename FormatStringCT<T>::type, StringCT<delim>, typename FormatStringCT<StringCT<delim>, U, Args...>::type> {};
template <char delim, typename T>
struct FormatStringCT<StringCT<delim>, T> : FormatStringCT<T> {};

template <char delim, typename... Args>
using DelimitFormatStringCT = FormatStringCT<StringCT<delim>, Args...>;

template <typename...>
struct DelimitConcatStringCT;
template <typename D, typename T>
struct DelimitConcatStringCT<D, T> : T {};
template <typename D, typename T, typename U, typename... Args>
struct DelimitConcatStringCT<D, T, U, Args...> : ConcatStringCT<T, D, typename DelimitConcatStringCT<D, U, Args...>::type> {};

template <typename T, bool...>
struct PrintfConvert {};

template <typename T>
struct PrintfConvert<T, true, false> {
    using value_type = decltype(std::declval<T>().toStringify());
    using format = typename T::printfformat;

    // This return type should always be an rvalue reference
    static value_type convert(T &t) { return t.toStringify(); }
};

template <typename T>
struct PrintfConvert<T, false, true> {
    using value_type = const char *;
    using format = typename FormatSpecifierCT<typename std::decay<T>::type>::type;
    static value_type convert(const T &t) { return t.c_str(); }
};

template <typename T>
struct PrintfConvert<T, false, false> {
    using value_type = T;
    using format = typename FormatSpecifierCT<typename std::decay<T>::type>::type;
    static value_type convert(const T &t) { return t; }
};

}    // namespace stringct
}    // namespace common

#endif
