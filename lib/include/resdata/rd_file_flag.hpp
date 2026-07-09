#pragma once

#include <type_traits>

enum class FileMode : int {
    DEFAULT = 0,
    CLOSE_STREAM =
        1, /* This flag will close the underlying FILE object between each access;
              this is mainly to save filedescriptors in cases where many rd_file
              instances are open at the same time. */
    WRITABLE =
        2 /* This flag opens the file in a mode where it can be updated and modified,
             but it must still exist and be readable. I.e. this should not compared
             with the normal: fopen(filename , "w") where an existing file is
             truncated to zero upon successfull open. */
};

constexpr FileMode operator|(FileMode lhs, FileMode rhs) {
    using T = std::underlying_type_t<FileMode>;
    return static_cast<FileMode>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

constexpr FileMode operator&(FileMode lhs, FileMode rhs) {
    using T = std::underlying_type_t<FileMode>;
    return static_cast<FileMode>(static_cast<T>(lhs) & static_cast<T>(rhs));
}

constexpr FileMode operator^(FileMode lhs, FileMode rhs) {
    using T = std::underlying_type_t<FileMode>;
    return static_cast<FileMode>(static_cast<T>(lhs) ^ static_cast<T>(rhs));
}

constexpr FileMode operator~(FileMode value) {
    using T = std::underlying_type_t<FileMode>;
    return static_cast<FileMode>(~static_cast<T>(value));
}

constexpr FileMode &operator|=(FileMode &lhs, FileMode rhs) {
    lhs = lhs | rhs;
    return lhs;
}

constexpr FileMode &operator&=(FileMode &lhs, FileMode rhs) {
    lhs = lhs & rhs;
    return lhs;
}

constexpr FileMode &operator^=(FileMode &lhs, FileMode rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}
