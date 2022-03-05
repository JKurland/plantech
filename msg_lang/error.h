#pragma once

#include <filesystem>
#include <string_view>
#include <cstdint>
#include <string>

struct SourceLocation {
    std::filesystem::path path;
    std::string_view snippet;
    size_t line;
    size_t character;
};

struct Error {
    std::string message;
    SourceLocation location;
};

struct SourceFile {
    std::filesystem::path path;
    std::string content;
};

std::string formatError(const Error& error);

SourceLocation getLocation(const SourceFile& sourceFile, size_t pos);