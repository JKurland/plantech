#include "error.h"

#include <string>
#include <cassert>


std::string formatError(const Error& error) {
    std::string rtn = error.location.path.string();
    rtn.push_back(':');
    rtn.append(std::to_string(error.location.line));
    rtn.append(": ");
    rtn.append(error.message);
    rtn.append("\n    ");
    rtn.append(error.location.snippet);
    rtn.push_back('\n');
    for (size_t i = 0; i < error.location.character + 4; i++) {
        rtn.push_back(' ');
    }
    rtn.push_back('^');
    return rtn;
}

SourceLocation getLocation(const SourceFile& sourceFile, size_t pos) {
    size_t lineStart = 0;
    size_t lineNumber = 0;
    size_t lineEnd = sourceFile.content.find('\n');

    assert(pos < sourceFile.content.size());

    while (lineEnd != std::string::npos && lineEnd < pos) {
        lineStart = lineEnd + 1;
        lineNumber++;
        lineEnd = sourceFile.content.find('\n', lineStart);
    }

    if (lineEnd == std::string::npos) {
        lineEnd = sourceFile.content.size() - 1;
    }

    std::string_view snippet = std::string_view(&sourceFile.content[lineStart], lineEnd - lineStart);
    while (snippet.ends_with("\n")) {
        snippet.remove_suffix(1);
    }

    return SourceLocation {
        .path = sourceFile.path,
        .snippet = snippet,
        .line = lineNumber,
        .character = pos - lineStart,
    };
}
