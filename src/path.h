#pragma once

#include <string>;

namespace ts {

    using std::string;

    /**
     * Normalize path separators, converting `\` into `/`.
     */
    string normalizeSlashes(const string &path) {
        const index = path.indexOf("\\");
        if (index === -1) {
            return path;
        }
        backslashRegExp.lastIndex = index; // prime regex with known position
        return path.replace(backslashRegExp, directorySeparator);
    }

    string normalizePath(const string &path) {
        path = normalizeSlashes(path);
        // Most paths don't require normalization
        if (!relativePathSegmentRegExp.test(path)) {
            return path;
        }
        // Some paths only require cleanup of `/./` or leading `./`
        const simplified = path.replace(/\/\.\//g, "/").replace(/^\.\//, "");
        if (simplified !== path) {
            path = simplified;
            if (!relativePathSegmentRegExp.test(path)) {
                return path;
            }
        }
        // Other paths require full normalization
        const normalized = getPathFromPathComponents(reducePathComponents(getPathComponents(path)));
        return normalized && hasTrailingDirectorySeparator(path) ? ensureTrailingDirectorySeparator(normalized) : normalized;
    }
}
