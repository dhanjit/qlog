#ifndef _STRINGCT_HPP_
#define _STRINGCT_HPP_

// The only macro used due to language limitation
// CT("ABCD") will be equivalent to common::stringct::StringCT<'A','B','C','D'>
// supports upto 17 characters

namespace common {
namespace stringct {

template <char... chars>
struct StringCT {
    using type = StringCT<chars...>;
    static constexpr char str[sizeof...(chars) + 1] = {chars..., '\0'};
    StringCT() = delete;
};

template <char... chars>
constexpr char StringCT<chars...>::str[sizeof...(chars) + 1];

template <typename, char...>
struct TrimmedStringCT;
template <char... validchars, char c, char... chars>
struct TrimmedStringCT<StringCT<validchars...>, c, chars...> : TrimmedStringCT<StringCT<validchars..., c>, chars...> {};
template <char... validchars, char... chars>
struct TrimmedStringCT<StringCT<validchars...>, '\0', chars...> : StringCT<validchars...> {};

#define SCT_GET_1(str, i) (i < sizeof(str) ? str[i] : '\0')
#define SCT_GET_4(str, i) SCT_GET_1(str, i + 0), SCT_GET_1(str, i + 1), SCT_GET_1(str, i + 2), SCT_GET_1(str, i + 3)
#define SCT_GET_16(str, i) SCT_GET_4(str, i + 0), SCT_GET_4(str, i + 4), SCT_GET_4(str, i + 8), SCT_GET_4(str, i + 12)
#define SCT_GET_64(str, i) SCT_GET_16(str, i + 0), SCT_GET_16(str, i + 16), SCT_GET_16(str, i + 32), SCT_GET_16(str, i + 48)
#define SCT_GET(str) SCT_GET_64(str, 0)
#define SCT(str) common::stringct::TrimmedStringCT<common::stringct::StringCT<>, SCT_GET(str)>::type

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
struct FormatStringCT<StringCT<delim>, T, U, Args...>
    : ConcatStringCT<typename FormatStringCT<T>::type, StringCT<delim>, typename FormatStringCT<StringCT<delim>, U, Args...>::type> {};
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
    __attribute__((always_inline)) static value_type &&get(T &&t) { return std::forward<value_type>(t.toStringify()); }
};

template <typename T>
struct PrintfConvert<T, true, true> {
    using value_type = decltype(std::declval<typename std::remove_pointer<T>::type>().toStringify());
    using format = typename std::remove_pointer<T>::type::printfformat;
    __attribute__((always_inline)) static const value_type get(const T &t) { return t->toStringify(); }
};

template <typename T, bool x>
struct PrintfConvert<T, false, x> {
    using value_type = T;
    using format = typename FormatStringCT<value_type>::type;
    // static format getT();
    __attribute__((always_inline)) static const value_type &get(const T &t) { return t; }
};

template <typename T>
struct PrintfConvert<T> : PrintfConvert<T, std::is_class<typename std::remove_pointer<T>::type>::value, std::is_pointer<T>::value> {};
}
}
#endif
