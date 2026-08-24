#pragma once

#include <type_traits>
#include <cstdint>

/* adapted from krisvers/vkom */
#define KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY(T_, op_) \
inline T_ operator op_(T_ const& a, T_ const& b) { \
    return static_cast<T_>(static_cast<std::underlying_type_t<T_>>(a) op_ static_cast<std::underlying_type_t<T_>>(b)); \
}

#define KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_UNARY(T_, op_) \
inline T_ operator op_(T_ const& a) { \
    return static_cast<T_>(op_ static_cast<std::underlying_type_t<T_>>(a)); \
}

#define KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY_ASSIGNMENT(T_, op_, opeq_) \
inline T_& operator opeq_(T_& a, T_ const& b) { \
    return (a = static_cast<T_>(static_cast<std::underlying_type_t<T_>>(a) op_ static_cast<std::underlying_type_t<T_>>(b))); \
}

#define KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_UNARY_ASSIGNMENT(T_, op_, opeq_) \
inline T_& operator opeq_(T_& a) { \
    return (a = static_cast<T_>(op_ static_cast<std::underlying_type_t<T_>>(a))); \
}

#define KLATR_DEFINE_ENUM_BITFLAGS_OPERATORS(T_) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_UNARY(T_, ~) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY(T_, |) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY_ASSIGNMENT(T_, |, |=) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY(T_, &) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY_ASSIGNMENT(T_, &, &=) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY(T_, ^) \
    KLATR_DEFINE_ENUM_BITFLAGS_OPERATOR_BINARY_ASSIGNMENT(T_, ^, ^=)
