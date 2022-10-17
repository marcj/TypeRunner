#pragma once

#include "path.h"

namespace tr {
    using std::string;
    using std::replace;
    using std::regex;
    using tr::utf::charCodeAt;
    using tr::utf::CharacterCodes;

    bool fileExtensionIs(const string &path, const string &extension) {
        return path.size() > extension.size() && endsWith(path, extension);
    }

    bool fileExtensionIsOneOf(const string &path, const vector<string> &extensions) {
        for (auto &extension: extensions) {
            if (fileExtensionIs(path, extension)) {
                return true;
            }
        }

        return false;
    }

    bool fileExtensionIsOneOf(const string &path, const vector<const char *> &extensions) {
        return fileExtensionIsOneOf(path, charToStringVector(extensions));
    }

    /**
     * Normalize path separators, converting `\` into `/`.
     */
    string normalizeSlashes(const string &path) {
        return replaceAll(path, altDirectorySeparator, directorySeparator);
    }

    //// Path Parsing
    bool isVolumeCharacter(const CharCode &charCode) {
        return (charCode.code >= CharacterCodes::a && charCode.code <= CharacterCodes::z) ||
               (charCode.code >= CharacterCodes::A && charCode.code <= CharacterCodes::Z);
    }

    int getFileUrlVolumeSeparatorEnd(const string &url, int start) {
        auto ch0 = charCodeAt(url, start);
        if (ch0.code == CharacterCodes::colon) return start + 1;
        if (ch0.code == CharacterCodes::percent && charCodeAt(url, start + 1).code == CharacterCodes::_3) {
            auto ch2 = charCodeAt(url, start + 2);
            if (ch2.code == CharacterCodes::a || ch2.code == CharacterCodes::A) return start + 3;
        }
        return - 1;
    }

    /**
     * Returns length of the root part of a path or URL (i.e. length of "/", "x:/", "//server/share/, file:///user/files").
     * If the root is part of a URL, the twos-complement of the root length is returned.
     */
    int getEncodedRootLength(const string &path) {
        if (path.empty()) return 0;
        auto ch0 = charCodeAt(path, 0);

        // POSIX or UNC
        if (ch0.code == CharacterCodes::slash || ch0.code == CharacterCodes::backslash) {
            if (charCodeAt(path, 1).code != ch0.code) return 1; // POSIX: "/" (or non-normalized "\")

            auto p1 = path.find(ch0.code == CharacterCodes::slash ? directorySeparator : altDirectorySeparator, 2);
            if (p1 < 0) return path.size(); // UNC: "//server" or "\\server"

            return p1 + 1; // UNC: "//server/" or "\\server\"
        }

        // DOS
        if (isVolumeCharacter(ch0) && charCodeAt(path, 1).code == CharacterCodes::colon) {
            auto ch2 = charCodeAt(path, 2);
            if (ch2.code == CharacterCodes::slash || ch2.code == CharacterCodes::backslash) return 3; // DOS: "c:/" or "c:\"
            if (path.size() == 2) return 2; // DOS: "c:" (but not "c:d")
        }

        // URL
        auto schemeEnd = path.find(urlSchemeSeparator);
        if (schemeEnd != string::npos) {
            auto authorityStart = schemeEnd + urlSchemeSeparatorSize;
            auto authorityEnd = path.find(directorySeparator, authorityStart);
            if (authorityEnd != string::npos) { // URL: "file:///", "file://server/", "file://server/path"
                // For local "file" URLs, include the leading DOS volume (if present).
                // Per https://www.ietf.org/rfc/rfc1738.txt, a host of "" or "localhost" is a
                // special case interpreted as "the machine from which the URL is being interpreted".
                auto scheme = path.substr(0, schemeEnd);
                auto authority = path.substr(authorityStart, authorityEnd);
                if (scheme == "file" && (authority == "" || authority == "localhost") &&
                    isVolumeCharacter(charCodeAt(path, authorityEnd + 1))) {
                    auto volumeSeparatorEnd = getFileUrlVolumeSeparatorEnd(path, authorityEnd + 2);
                    if (volumeSeparatorEnd != - 1) {
                        if (charCodeAt(path, volumeSeparatorEnd).code == CharacterCodes::slash) {
                            // URL: "file:///c:/", "file://localhost/c:/", "file:///c%3a/", "file://localhost/c%3a/"
                            return ~ (volumeSeparatorEnd + 1);
                        }
                        if (volumeSeparatorEnd == path.size()) {
                            // URL: "file:///c:", "file://localhost/c:", "file:///c$3a", "file://localhost/c%3a"
                            // but not "file:///c:d" or "file:///c%3ad"
                            return ~ volumeSeparatorEnd;
                        }
                    }
                }
                return ~ (authorityEnd + 1); // URL: "file://server/", "http://server/"
            }
            return ~ path.size(); // URL: "file://server", "http://server"
        }

        // relative
        return 0;
    }

    /**
     * Returns length of the root part of a path or URL (i.e. length of "/", "x:/", "//server/share/, file:///user/files").
     *
     * For example:
     * ```ts
     * getRootLength("a") === 0                   // ""
     * getRootLength("/") === 1                   // "/"
     * getRootLength("c:") === 2                  // "c:"
     * getRootLength("c:d") === 0                 // ""
     * getRootLength("c:/") === 3                 // "c:/"
     * getRootLength("c:\\") === 3                // "c:\\"
     * getRootLength("//server") === 7            // "//server"
     * getRootLength("//server/share") === 8      // "//server/"
     * getRootLength("\\\\server") === 7          // "\\\\server"
     * getRootLength("\\\\server\\share") === 8   // "\\\\server\\"
     * getRootLength("file:///path") === 8        // "file:///"
     * getRootLength("file:///c:") === 10         // "file:///c:"
     * getRootLength("file:///c:d") === 8         // "file:///"
     * getRootLength("file:///c:/path") === 11    // "file:///c:/"
     * getRootLength("file://server") === 13      // "file://server"
     * getRootLength("file://server/path") === 14 // "file://server/"
     * getRootLength("http://server") === 13      // "http://server"
     * getRootLength("http://server/path") === 14 // "http://server/"
     * ```
     */
    int getRootLength(const string &path) {
        auto rootLength = getEncodedRootLength(path);
        return rootLength < 0 ? ~ rootLength : rootLength;
    }

    /**
     * Determines whether a charCode corresponds to `/` or `\`.
     */
    bool isAnyDirectorySeparator(const CharCode &charCode) {
        return charCode.code == CharacterCodes::slash || charCode.code == CharacterCodes::backslash;
    }

    bool hasTrailingDirectorySeparator(const string &path) {
        return path.size() > 0 && isAnyDirectorySeparator(charCodeAt(path, path.size() - 1));
    }

    string ensureTrailingDirectorySeparator(const string &path) {
        if (! hasTrailingDirectorySeparator(path)) {
            return path + directorySeparator;
        }

        return path;
    }

    /**
     * Combines paths. If a path is absolute, it replaces any previous path. Relative paths are not simplified.
     *
     * ```ts
     * // Non-rooted
     * combinePaths("path", "to", "file.ext") === "path/to/file.ext"
     * combinePaths("path", "dir", "..", "to", "file.ext") === "path/dir/../to/file.ext"
     * // POSIX
     * combinePaths("/path", "to", "file.ext") === "/path/to/file.ext"
     * combinePaths("/path", "/to", "file.ext") === "/to/file.ext"
     * // DOS
     * combinePaths("c:/path", "to", "file.ext") === "c:/path/to/file.ext"
     * combinePaths("c:/path", "c:/to", "file.ext") === "c:/to/file.ext"
     * // URL
     * combinePaths("file:///path", "to", "file.ext") === "file:///path/to/file.ext"
     * combinePaths("file:///path", "file:///to", "file.ext") === "file:///to/file.ext"
     * ```
     */
    string combinePaths(string path, const vector<string> &paths) {
        if (! path.empty()) path = normalizeSlashes(path);
        for (auto &relativePath: paths) {
            if (relativePath.empty()) continue;
            auto p = normalizeSlashes(relativePath);
            if (path.empty() || getRootLength(p) != 0) {
                path = p;
            } else {
                path = ensureTrailingDirectorySeparator(path) + p;
            }
        }
        return path;
    }

    vector<string> pathComponents(const string &path, int rootLength) {
        auto root = path.substr(0, rootLength);
        auto rest = split(path.substr(rootLength), directorySeparator);
        if (rest.size() && ! lastOrUndefined(rest)) rest.pop_back();
        rest.insert(rest.begin(), root);
        return rest;
    }

    /**
     * Parse a path into an array containing a root component (at index 0) and zero or more path
     * components (at indices > 0). The result is not normalized.
     * If the path is relative, the root component is `""`.
     * If the path is absolute, the root component includes the first path separator (`/`).
     *
     * ```ts
     * // POSIX
     * getPathComponents("/path/to/file.ext") === ["/", "path", "to", "file.ext"]
     * getPathComponents("/path/to/") === ["/", "path", "to"]
     * getPathComponents("/") === ["/"]
     * // DOS
     * getPathComponents("c:/path/to/file.ext") === ["c:/", "path", "to", "file.ext"]
     * getPathComponents("c:/path/to/") === ["c:/", "path", "to"]
     * getPathComponents("c:/") === ["c:/"]
     * getPathComponents("c:") === ["c:"]
     * // URL
     * getPathComponents("http://typescriptlang.org/path/to/file.ext") === ["http://typescriptlang.org/", "path", "to", "file.ext"]
     * getPathComponents("http://typescriptlang.org/path/to/") === ["http://typescriptlang.org/", "path", "to"]
     * getPathComponents("http://typescriptlang.org/") === ["http://typescriptlang.org/"]
     * getPathComponents("http://typescriptlang.org") === ["http://typescriptlang.org"]
     * getPathComponents("file://server/path/to/file.ext") === ["file://server/", "path", "to", "file.ext"]
     * getPathComponents("file://server/path/to/") === ["file://server/", "path", "to"]
     * getPathComponents("file://server/") === ["file://server/"]
     * getPathComponents("file://server") === ["file://server"]
     * getPathComponents("file:///path/to/file.ext") === ["file:///", "path", "to", "file.ext"]
     * getPathComponents("file:///path/to/") === ["file:///", "path", "to"]
     * getPathComponents("file:///") === ["file:///"]
     * getPathComponents("file://") === ["file://"]
     */
    vector<string> getPathComponents(const string &path, const string &currentDirectory) {
        auto p = combinePaths(currentDirectory, {path});
        return pathComponents(p, getRootLength(p));
    }

    /**
     * Formats a parsed path consisting of a root component (at index 0) and zero or more path
     * segments (at indices > 0).
     *
     * ```ts
     * getPathFromPathComponents(["/", "path", "to", "file.ext"]) === "/path/to/file.ext"
     * ```
     */
    string getPathFromPathComponents(const vector<string> &pathComponents) {
        if (pathComponents.empty()) return "";

        string root = ensureTrailingDirectorySeparator(pathComponents[0]);
        return root + join(slice(pathComponents, 1), directorySeparator);
    }

    /**
     * Reduce an array of path components to a more simplified path by navigating any
     * `"."` or `".."` entries in the path.
     */
    vector<string> reducePathComponents(const vector<string> &components) {
        if (components.empty()) return {};

        vector<string> reduced{components[0]};

        for (int i = 1; i < components.size(); i ++) {
            auto component = components[i];
            if (component.empty()) continue;
            if (component == ".") continue;
            if (component == "..") {
                if (reduced.size() > 1) {
                    if (reduced[reduced.size() - 1] != "..") {
                        reduced.pop_back();
                        continue;
                    }
                } else if (! reduced.empty() && ! reduced[0].empty()) continue;
            }
            reduced.push_back(component);
        }
        return reduced;
    }

    string normalizePath(string &_path) {
        string path = normalizeSlashes(_path);
        //todo: rework that to make it faster
        return path;
        // Most paths don't require normalization
//        if (! regex_search(path, relativePathSegmentRegExp)) {
//            return path;
//        }
//        // Some paths only require cleanup of `/./` or leading `./`
//        auto simplified = replaceLeading(replaceAll(path, "/./", "/"), "./", "");
//        if (simplified != path) {
//            path = simplified;
//            if (! regex_search(path, relativePathSegmentRegExp)) {
//                return path;
//            }
//        }
//        // Other paths require full normalization
//        auto normalized = getPathFromPathComponents(reducePathComponents(getPathComponents(path)));
//        if (! normalized.empty() && hasTrailingDirectorySeparator(path)) return ensureTrailingDirectorySeparator(normalized);
//        return normalized;
    }
}
