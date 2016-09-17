namespace common {
namespace logger {
template <char... chars> struct StringCT {
    static constexpr char s[sizeof...(chars) + 1] = {chars..., '\0'};
};

template <char... chars> const char StringCT<chars...>::str[sizeof...(chars) + 1];

template <char... chars> struct StringBuilderCT {
    using value = StringCT<chars...>;
};

// template <char... chars, typename T, class... Args>
// auto concatFormatString(StringBuilderCT<chars...>, T x, Args... args) {

// }

template <char... chars, class... Args> StringBuilderCT<chars..., '\n'> concatFormatString(StringBuilderCT<chars...>);
}
}
