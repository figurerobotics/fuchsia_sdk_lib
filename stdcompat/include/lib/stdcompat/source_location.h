// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be found in the LICENSE file.

#ifndef LIB_STDCOMPAT_SOURCE_LOCATION_H_
#define LIB_STDCOMPAT_SOURCE_LOCATION_H_

#include "version.h"

#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L && \
    !defined(LIB_STDCOMPAT_USE_POLYFILLS)

#include <source_location>

namespace cpp20 {

using std::source_location;

}  // namespace cpp20

#else

#include <cstdint>

namespace cpp20 {

class source_location {
  struct values {
    constexpr explicit values(std::uint_least32_t line, std::uint_least32_t column,
                              const char* file_name, const char* function_name)
        : line{line}, column{column}, file_name{file_name}, function_name{function_name} {}

    std::uint_least32_t line;
    std::uint_least32_t column;
    const char* file_name;
    const char* function_name;
  };

 public:
  constexpr source_location() noexcept {}  // LLVM bug 36684
  source_location(const source_location& other) = default;
  source_location(source_location&& other) noexcept = default;
  ~source_location() = default;

#define LIB_STDCOMPAT_SOURCE_LOCATION_LINE (0)
#define LIB_STDCOMPAT_SOURCE_LOCATION_COLUMN (0)
#define LIB_STDCOMPAT_SOURCE_LOCATION_FILE ("")
#define LIB_STDCOMPAT_SOURCE_LOCATION_FUNCTION ("")

#if defined(__has_builtin)
#if __has_builtin(__builtin_LINE)
#undef LIB_STDCOMPAT_SOURCE_LOCATION_LINE
#define LIB_STDCOMPAT_SOURCE_LOCATION_LINE (__builtin_LINE())
#endif
#if __has_builtin(__builtin_COLUMN)
#undef LIB_STDCOMPAT_SOURCE_LOCATION_COLUMN
#define LIB_STDCOMPAT_SOURCE_LOCATION_COLUMN (__builtin_COLUMN())
#endif
#if __has_builtin(__builtin_FILE)
#undef LIB_STDCOMPAT_SOURCE_LOCATION_FILE
#define LIB_STDCOMPAT_SOURCE_LOCATION_FILE (__builtin_FILE())
#endif
#if __has_builtin(__builtin_FUNCTION)
#undef LIB_STDCOMPAT_SOURCE_LOCATION_FUNCTION
#define LIB_STDCOMPAT_SOURCE_LOCATION_FUNCTION (__builtin_FUNCTION())
#endif
#endif

  static constexpr source_location current(values value = values{
                                               LIB_STDCOMPAT_SOURCE_LOCATION_LINE,
                                               LIB_STDCOMPAT_SOURCE_LOCATION_COLUMN,
                                               LIB_STDCOMPAT_SOURCE_LOCATION_FILE,
                                               LIB_STDCOMPAT_SOURCE_LOCATION_FUNCTION}) {
    return source_location{value};
  }

#undef LIB_STDCOMPAT_SOURCE_LOCATION_LINE
#undef LIB_STDCOMPAT_SOURCE_LOCATION_COLUMN
#undef LIB_STDCOMPAT_SOURCE_LOCATION_FILE
#undef LIB_STDCOMPAT_SOURCE_LOCATION_FUNCTION

  constexpr std::uint_least32_t line() const noexcept { return values_.line; }

  constexpr std::uint_least32_t column() const noexcept { return values_.column; }

  constexpr const char* file_name() const noexcept { return values_.file_name; }

  constexpr const char* function_name() const noexcept { return values_.function_name; }

 private:
  constexpr explicit source_location(values values) : values_{values} {}

  values values_{0, 0, "", ""};
};

}  // namespace cpp20

#endif  // __cpp_lib_source_location >= 201907L && !defined(LIB_STDCOMPAT_USE_POLYFILLS)

#endif  // LIB_STDCOMPAT_SOURCE_LOCATION_H_
