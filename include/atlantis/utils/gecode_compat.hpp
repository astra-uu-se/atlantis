#pragma once

#ifndef _LIBCPP_STD_VER
#if __cplusplus <= 201103L
#define _LIBCPP_STD_VER 11
#elif __cplusplus <= 201402L
#define _LIBCPP_STD_VER 14
#elif __cplusplus <= 201703L
#define _LIBCPP_STD_VER 17
#elif __cplusplus <= 202002L
#define _LIBCPP_STD_VER 20
#elif __cplusplus <= 202302L
#define _LIBCPP_STD_VER 23  // current year, or date of c++2a ratification
#else
#define _LIBCPP_STD_VER 26
#endif
#endif