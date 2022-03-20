#pragma once

#include <filesystem>
#include <string_view>
#include <cstdint>
#include <string>
#include <compare>

struct SourceLocation {
    std::filesystem::path path;
    std::string_view snippet;
    size_t line;
    size_t character;

    auto operator<=>(const SourceLocation&) const = default;
};

struct Error {
    std::string message;
    SourceLocation location;

    auto operator<=>(const Error&) const = default;
};

struct SourceFile {
    std::filesystem::path path;
    std::string content;
};

std::string formatError(const Error& error);

SourceLocation getLocation(const SourceFile& sourceFile, size_t pos);