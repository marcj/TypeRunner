/**
 * A lighter string replacement. Since most nodes like Identifier/StringLiteral/NumberLiteral etc
 * do not need std::string with all its features, it's enough if we either have a string_view (reference)
 * to the SourceFile::text, or have a static string wrapper around const char *.
 */
namespace tr {

    struct simple_string {
        const char *text;
        int size;
    };

}