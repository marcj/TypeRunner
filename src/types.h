#pragma once

#include <map>
#include <iostream>
#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <variant>
#include <type_traits>
#include <stdexcept>
#include "core.h"

namespace ts {
    struct SourceFile;
}

namespace ts::types {
    using namespace std;
    struct CompilerOptions;

    enum class ScriptTarget {
        ES3 = 0,
        ES5 = 1,
        ES2015 = 2,
        ES2016 = 3,
        ES2017 = 4,
        ES2018 = 5,
        ES2019 = 6,
        ES2020 = 7,
        ES2021 = 8,
        ES2022 = 9,
        ESNext = 99,
        JSON = 100,
        Latest = ESNext,
    };

    enum class ScriptKind {
        Unknown = 0,
        JS = 1,
        JSX = 2,
        TS = 3,
        TSX = 4,
        External = 5,
        JSON = 6,
        /**
         * Used on extensions that doesn't define the ScriptKind but the content defines it.
         * Deferred extensions are going to be included in all project contexts.
         */
        Deferred = 7
    };

    enum CommentDirectiveType {
        ExpectError,
        Ignore,
    };

    struct TextRange {
        int pos;
        int end;
    };

    struct CommentDirective {
        TextRange range;
        CommentDirectiveType type;
    };

    enum LanguageVariant {
        Standard,
        JSX
    };
    enum DiagnosticCategory {
        Warning,
        Error,
        Suggestion,
        Message
    };

    struct DiagnosticMessage {
        int code;
        DiagnosticCategory category;
        string key;
        string message;
        bool reportsUnnecessary;
        bool reportsDeprecated;
        /* @internal */
        bool elidedInCompatabilityPyramid;
    };

    struct DiagnosticMessageChain {
        string messageText;
        DiagnosticCategory category;
        int code;
        vector<DiagnosticMessageChain> next;
    };

    struct DiagnosticRelatedInformation {
        DiagnosticCategory category;
        int code;
        SourceFile *file;
        int start = - 1; //-1 = undefined
        int length = - 1; //-1 = undefined
        string messageText;
        DiagnosticMessageChain *messageChain = nullptr;
    };

    struct Diagnostic: DiagnosticRelatedInformation {
        /** May store more in future. For now, this will simply be `true` to indicate when a diagnostic is an unused-identifier diagnostic. */
        bool reportsUnnecessary = false;

        bool reportsDeprecated = false;
        string source;
        vector<DiagnosticRelatedInformation> relatedInformation;
        /* @internal */ CompilerOptions *skippedOn = nullptr;
    };

    struct DiagnosticWithDetachedLocation: Diagnostic {
        string fileName;
        int start = 0;
        int length = 0;
    };

    enum class ImportsNotUsedAsValues {
        Remove,
        Preserve,
        Error,
    };

    enum class JsxEmit {
        None = 0,
        Preserve = 1,
        React = 2,
        ReactNative = 3,
        ReactJSX = 4,
        ReactJSXDev = 5,
    };

    enum class ModuleKind {
        None = 0,
        CommonJS = 1,
        AMD = 2,
        UMD = 3,
        System = 4,

        // NOTE: ES module kinds should be contiguous to more easily check whether a module kind is *any* ES module kind.
        //       Non-ES module kinds should not come between ES2015 (the earliest ES module kind) and ESNext (the last ES
        //       module kind).
        ES2015 = 5,
        ES2020 = 6,
        ES2022 = 7,
        ESNext = 99,

        // Node16+ is an amalgam of commonjs (albeit updated) and es2022+, and represents a distinct module system from es2020/esnext
        Node16 = 100,
        NodeNext = 199,
    };

    enum class ModuleResolutionKind {
        Classic = 1,
        NodeJs = 2,
        // Starting with node12, node's module resolver has significant departures from traditional cjs resolution
        // to better support ecmascript modules and their use within BaseNode - however more features are still being added.
        // TypeScript's BaseNode ESM support was introduced after BaseNode 12 went end-of-life, and BaseNode 14 is the earliest stable
        // version that supports both pattern trailers - *but*, Node 16 is the first version that also supports ECMASCript 2022.
        // In turn, we offer both a `NodeNext` moving resolution target, and a `Node16` version-anchored resolution target
        Node16 = 3,
        NodeNext = 99, // Not simply `Node16` so that compiled code linked against TS can use the `Next` value reliably (same as with `ModuleKind`)
    };

    enum class ModuleDetectionKind {
        /**
         * Files with imports, exports and/or import.meta are considered modules
         */
        Legacy = 1,
        /**
         * Legacy, but also files with jsx under react-jsx or react-jsxdev and esm mode files under moduleResolution: node16+
         */
        Auto = 2,
        /**
         * Consider all non-declaration files modules, regardless of present syntax
         */
        Force = 3,
    };

    enum class NewLineKind {
        CarriageReturnLineFeed = 0,
        LineFeed = 1
    };

    struct CompilerOptions {
        /*@internal*/bool all = false;
        bool allowJs = false;
        /*@internal*/ bool allowNonTsExtensions = false;
        bool allowSyntheticDefaultImports = false;
        bool allowUmdGlobalAccess = false;
        bool allowUnreachableCode = false;
        bool allowUnusedLabels = false;
        bool alwaysStrict = false;  // Always combine with strict property
        optional<string> baseUrl;
        /** An error if set - this should only go through the -b pipeline and not actually be observed */
        /*@internal*/
        bool build = false;
        optional<string> charset;
        bool checkJs = false;
        /* @internal */ optional<string> configFilePath; //?: string;
        /** configFile is set as non enumerable property so as to avoid checking of json source files */
//        /* @internal */ configFile?: TsConfigSourceFile;

        bool declaration = false;
        bool declarationMap = false;
        bool emitDeclarationOnly = false;
        optional<string> declarationDir;
        /* @internal */ bool diagnostics = false;
        /* @internal */ bool extendedDiagnostics = false;
        bool disableSizeLimit = false;
        bool disableSourceOfProjectReferenceRedirect = false;
        bool disableSolutionSearching = false;
        bool disableReferencedProjectLoad = false;
        bool downlevelIteration = false;
        bool emitBOM = false;
        bool emitDecoratorMetadata = false;
        bool exactOptionalPropertyTypes = false;
        bool experimentalDecorators = false;
        bool forceConsistentCasingInFileNames = false;
        /*@internal*/string generateCpuProfile; //?: string;
        /*@internal*/string generateTrace; //?: string;
        /*@internal*/bool help = false;
        bool importHelpers = false;
        optional<ImportsNotUsedAsValues> importsNotUsedAsValues;
        /*@internal*/bool init = false;
        bool inlineSourceMap = false;
        bool inlineSources = false;
        bool isolatedModules = false;
        optional<JsxEmit> jsx;
        bool keyofStringsOnly = false;
        vector<string> lib; //?: string[]
        /*@internal*/bool listEmittedFiles = false;
        /*@internal*/bool listFiles = false;
        /*@internal*/bool explainFiles = false;
        /*@internal*/bool listFilesOnly = false;
        string locale; //?: string;
        string mapRoot; //?: string;
        int maxNodeModuleJsDepth; //?: number;
        ModuleKind module; //?: ModuleKind;
        ModuleResolutionKind moduleResolution; //?: ModuleResolutionKind;
        optional<vector<string>> moduleSuffixes; //?: string[];
        ModuleDetectionKind moduleDetection; //?: ModuleDetectionKind;
        NewLineKind newLine; //?: NewLineKind;
        bool noEmit = false;
        /*@internal*/bool noEmitForJsFiles = false;
        bool noEmitHelpers = false;
        bool noEmitOnError = false;
        bool noErrorTruncation = false;
        bool noFallthroughCasesInSwitch = false;
        bool noImplicitAny = false;  // Always combine with strict property
        bool noImplicitReturns = false;
        bool noImplicitThis = false;  // Always combine with strict property
        bool noStrictGenericChecks = false;
        bool noUnusedLocals = false;
        bool noUnusedParameters = false;
        bool noImplicitUseStrict = false;
        bool noPropertyAccessFromIndexSignature = false;
        bool assumeChangesOnlyAffectDirectDependencies = false;
        bool noLib = false;
        bool noResolve = false;
        /*@internal*/
        bool noDtsResolution = false;
        bool noUncheckedIndexedAccess = false;
        string out; //?: string;
        string outDir; //?: string;
        string outFile; //?: string;
        map<string, vector<string>> paths; //?: MapLike<string[]>;
        /** The directory of the config file that specified 'paths'. Used to resolve relative paths when 'baseUrl' is absent. */
        /*@internal*/ string pathsBasePath; //?: string;
//        /*@internal*/ plugins?: PluginImport[];
        bool preserveConstEnums = false;
        bool noImplicitOverride = false;
        bool preserveSymlinks = false;
        bool preserveValueImports = false;
        /* @internal */ bool preserveWatchOutput = false;
        string project; //?: string;
        /* @internal */ bool pretty = false;
        string reactNamespace; //?: string;
        string jsxFactory; //?: string;
        string jsxFragmentFactory; //?: string;
        string jsxImportSource; //?: string;
        bool composite = false;
        bool incremental = false;
        string tsBuildInfoFile; //?: string;
        bool removeComments = false;
        string rootDir; //?: string;
        string rootDirs; //?: string[];
        bool skipLibCheck = false;
        bool skipDefaultLibCheck = false;
        bool sourceMap = false;
        string sourceRoot; //?: string;
        bool strict = false;
        bool strictFunctionTypes = false;  // Always combine with strict property
        bool strictBindCallApply = false;  // Always combine with strict property
        bool strictNullChecks = false;  // Always combine with strict property
        bool strictPropertyInitialization = false;  // Always combine with strict property
        bool stripInternal = false;
        bool suppressExcessPropertyErrors = false;
        bool suppressImplicitAnyIndexErrors = false;
        /* @internal */ bool suppressOutputPathCheck = false;
        ScriptTarget target;
        bool traceResolution = false;
        bool useUnknownInCatchVariables = false;
        bool resolveJsonModule = false;
        vector<string> types; //?: string[];
        /** Paths used to compute primary types search locations */
        vector<string> typeRoots; //?: string[];
        /*@internal*/ bool version = false;
        /*@internal*/ bool watch = false;
        bool esModuleInterop = false;
        /* @internal */ bool showConfig = false;
        bool useDefineForClassFields = false;
    };

    enum TokenFlags {
        None = 0,
        /* @internal */
        PrecedingLineBreak = 1 << 0,
        /* @internal */
        PrecedingJSDocComment = 1 << 1,
        /* @internal */
        Unterminated = 1 << 2,
        /* @internal */
        ExtendedUnicodeEscape = 1 << 3,
        Scientific = 1 << 4,        // e.g. `10e2`
        Octal = 1 << 5,             // e.g. `0777`
        HexSpecifier = 1 << 6,      // e.g. `0x00000000`
        BinarySpecifier = 1 << 7,   // e.g. `0b0110010000000000`
        OctalSpecifier = 1 << 8,    // e.g. `0o777`
        /* @internal */
        ContainsSeparator = 1 << 9, // e.g. `0b1100_0101`
        /* @internal */
        UnicodeEscape = 1 << 10,
        /* @internal */
        ContainsInvalidEscape = 1 << 11,    // e.g. `\uhello`
        /* @internal */
        BinaryOrOctalSpecifier = BinarySpecifier | OctalSpecifier,
        /* @internal */
        NumericLiteralFlags = Scientific | Octal | HexSpecifier | BinaryOrOctalSpecifier | ContainsSeparator,
        /* @internal */
        TemplateLiteralLikeFlags = ContainsInvalidEscape,
    };

    enum CharacterCodes {
        nullCharacter = 0,
        maxAsciiCharacter = 0x7F,

        lineFeed = 0x0A,              // \n
        carriageReturn = 0x0D,        // \r
        lineSeparator = 0x2028,
        paragraphSeparator = 0x2029,
        nextLine = 0x0085,

        // Unicode 3.0 space characters
        space = 0x0020,   // " "
        nonBreakingSpace = 0x00A0,   //
        enQuad = 0x2000,
        emQuad = 0x2001,
        enSpace = 0x2002,
        emSpace = 0x2003,
        threePerEmSpace = 0x2004,
        fourPerEmSpace = 0x2005,
        sixPerEmSpace = 0x2006,
        figureSpace = 0x2007,
        punctuationSpace = 0x2008,
        thinSpace = 0x2009,
        hairSpace = 0x200A,
        zeroWidthSpace = 0x200B,
        narrowNoBreakSpace = 0x202F,
        ideographicSpace = 0x3000,
        mathematicalSpace = 0x205F,
        ogham = 0x1680,

        _ = 0x5F,
        $ = 0x24,

        _0 = 0x30,
        _1 = 0x31,
        _2 = 0x32,
        _3 = 0x33,
        _4 = 0x34,
        _5 = 0x35,
        _6 = 0x36,
        _7 = 0x37,
        _8 = 0x38,
        _9 = 0x39,

        a = 0x61,
        b = 0x62,
        c = 0x63,
        d = 0x64,
        e = 0x65,
        f = 0x66,
        g = 0x67,
        h = 0x68,
        i = 0x69,
        j = 0x6A,
        k = 0x6B,
        l = 0x6C,
        m = 0x6D,
        n = 0x6E,
        o = 0x6F,
        p = 0x70,
        q = 0x71,
        r = 0x72,
        s = 0x73,
        t = 0x74,
        u = 0x75,
        v = 0x76,
        w = 0x77,
        x = 0x78,
        y = 0x79,
        z = 0x7A,

        A = 0x41,
        B = 0x42,
        C = 0x43,
        D = 0x44,
        E = 0x45,
        F = 0x46,
        G = 0x47,
        H = 0x48,
        I = 0x49,
        J = 0x4A,
        K = 0x4B,
        L = 0x4C,
        M = 0x4D,
        N = 0x4E,
        O = 0x4F,
        P = 0x50,
        Q = 0x51,
        R = 0x52,
        S = 0x53,
        T = 0x54,
        U = 0x55,
        V = 0x56,
        W = 0x57,
        X = 0x58,
        Y = 0x59,
        Z = 0x5a,

        ampersand = 0x26,             // &
        asterisk = 0x2A,              // *
        at = 0x40,                    // @
        backslash = 0x5C,             // \
//        backtick = 0x60,              // `
        backtick = 0x60,              // `
        bar = 0x7C,                   // |
        caret = 0x5E,                 // ^
        closeBrace = 0x7D,            // }
        closeBracket = 0x5D,          // ]
        closeParen = 0x29,            // )
        colon = 0x3A,                 // :
        comma = 0x2C,                 // ,
        dot = 0x2E,                   // .
        doubleQuote = 0x22,           // "
        equals = 0x3D,                // =
        exclamation = 0x21,           // !
        greaterThan = 0x3E,           // >
        hash = 0x23,                  // #
        lessThan = 0x3C,              // <
        minus = 0x2D,                 // -
        openBrace = 0x7B,             // {
        openBracket = 0x5B,           // [
        openParen = 0x28,             // (
        percent = 0x25,               // %
        plus = 0x2B,                  // +
        question = 0x3F,              // ?
        semicolon = 0x3B,             // ;
        singleQuote = 0x27,           // '
        slash = 0x2F,                 // /
        tilde = 0x7E,                 // ~

        backspace = 0x08,             // \b
        formFeed = 0x0C,              // \f
        byteOrderMark = 0xFEFF,
        tab = 0x09,                   // \t
        verticalTab = 0x0B,           // \v
    };

    enum SyntaxKind {
        Unknown,
        EndOfFileToken,
        SingleLineCommentTrivia,
        MultiLineCommentTrivia,
        NewLineTrivia,
        WhitespaceTrivia,
        // We detect and preserve #! on the first line
        ShebangTrivia,
        // We detect and provide better error recovery when we encounter a git merge marker.  This
        // allows us to edit files with git-conflict markers in them in a much more pleasant manner.
        ConflictMarkerTrivia,
        // Literals
        NumericLiteral,
        BigIntLiteral,
        StringLiteral,
        JsxText,
        JsxTextAllWhiteSpaces,
        RegularExpressionLiteral,
        NoSubstitutionTemplateLiteral,
        // Pseudo-literals
        TemplateHead,
        TemplateMiddle,
        TemplateTail,
        // Punctuation
        OpenBraceToken,
        CloseBraceToken,
        OpenParenToken,
        CloseParenToken,
        OpenBracketToken,
        CloseBracketToken,
        DotToken,
        DotDotDotToken,
        SemicolonToken,
        CommaToken,
        QuestionDotToken,
        LessThanToken,
        LessThanSlashToken,
        GreaterThanToken,
        LessThanEqualsToken,
        GreaterThanEqualsToken,
        EqualsEqualsToken,
        ExclamationEqualsToken,
        EqualsEqualsEqualsToken,
        ExclamationEqualsEqualsToken,
        EqualsGreaterThanToken,
        PlusToken,
        MinusToken,
        AsteriskToken,
        AsteriskAsteriskToken,
        SlashToken,
        PercentToken,
        PlusPlusToken,
        MinusMinusToken,
        LessThanLessThanToken,
        GreaterThanGreaterThanToken,
        GreaterThanGreaterThanGreaterThanToken,
        AmpersandToken,
        BarToken,
        CaretToken,
        ExclamationToken,
        TildeToken,
        AmpersandAmpersandToken,
        BarBarToken,
        QuestionToken,
        ColonToken,
        AtToken,
        QuestionQuestionToken,
        /** Only the JSDoc scanner produces BacktickToken. The normal scanner produces NoSubstitutionTemplateLiteral and related kinds. */
        BacktickToken,
        /** Only the JSDoc scanner produces HashToken. The normal scanner produces PrivateIdentifier. */
        HashToken,
        // Assignments
        EqualsToken,
        PlusEqualsToken,
        MinusEqualsToken,
        AsteriskEqualsToken,
        AsteriskAsteriskEqualsToken,
        SlashEqualsToken,
        PercentEqualsToken,
        LessThanLessThanEqualsToken,
        GreaterThanGreaterThanEqualsToken,
        GreaterThanGreaterThanGreaterThanEqualsToken,
        AmpersandEqualsToken,
        BarEqualsToken,
        BarBarEqualsToken,
        AmpersandAmpersandEqualsToken,
        QuestionQuestionEqualsToken,
        CaretEqualsToken,
        // Identifiers and PrivateIdentifiers
        Identifier,
        PrivateIdentifier,
        // Reserved words
        BreakKeyword,
        CaseKeyword,
        CatchKeyword,
        ClassKeyword,
        ConstKeyword,
        ContinueKeyword,
        DebuggerKeyword,
        DefaultKeyword,
        DeleteKeyword,
        DoKeyword,
        ElseKeyword,
        EnumKeyword,
        ExportKeyword,
        ExtendsKeyword,
        FalseKeyword,
        FinallyKeyword,
        ForKeyword,
        FunctionKeyword,
        IfKeyword,
        ImportKeyword,
        InKeyword,
        InstanceOfKeyword,
        NewKeyword,
        NullKeyword,
        ReturnKeyword,
        SuperKeyword,
        SwitchKeyword,
        ThisKeyword,
        ThrowKeyword,
        TrueKeyword,
        TryKeyword,
        TypeOfKeyword,
        VarKeyword,
        VoidKeyword,
        WhileKeyword,
        WithKeyword,
        // Strict mode reserved words
        ImplementsKeyword,
        InterfaceKeyword,
        LetKeyword,
        PackageKeyword,
        PrivateKeyword,
        ProtectedKeyword,
        PublicKeyword,
        StaticKeyword,
        YieldKeyword,
        // Contextual keywords
        AbstractKeyword,
        AsKeyword,
        AssertsKeyword,
        AssertKeyword,
        AnyKeyword,
        AsyncKeyword,
        AwaitKeyword,
        BooleanKeyword,
        ConstructorKeyword,
        DeclareKeyword,
        GetKeyword,
        InferKeyword,
        IntrinsicKeyword,
        IsKeyword,
        KeyOfKeyword,
        ModuleKeyword,
        NamespaceKeyword,
        NeverKeyword,
        OutKeyword,
        ReadonlyKeyword,
        RequireKeyword,
        NumberKeyword,
        ObjectKeyword,
        SetKeyword,
        StringKeyword,
        SymbolKeyword,
        TypeKeyword,
        UndefinedKeyword,
        UniqueKeyword,
        UnknownKeyword,
        FromKeyword,
        GlobalKeyword,
        BigIntKeyword,
        OverrideKeyword,
        OfKeyword, // LastKeyword and LastToken and LastContextualKeyword

        // Parse tree nodes

        // Names
        QualifiedName,
        ComputedPropertyName,
        // Signature elements
        TypeParameter,
        Parameter,
        Decorator,
        // TypeMember
        PropertySignature,
        PropertyDeclaration,
        MethodSignature,
        MethodDeclaration,
        ClassStaticBlockDeclaration,
        Constructor,
        GetAccessor,
        SetAccessor,
        CallSignature,
        ConstructSignature,
        IndexSignature,
        // Type
        TypePredicate,
        TypeReference,
        FunctionType,
        ConstructorType,
        TypeQuery,
        TypeLiteral,
        ArrayType,
        TupleType,
        OptionalType,
        RestType,
        UnionType,
        IntersectionType,
        ConditionalType,
        InferType,
        ParenthesizedType,
        ThisType,
        TypeOperator,
        IndexedAccessType,
        MappedType,
        LiteralType,
        NamedTupleMember,
        TemplateLiteralType,
        TemplateLiteralTypeSpan,
        ImportType,
        // Binding patterns
        ObjectBindingPattern,
        ArrayBindingPattern,
        BindingElement,
        // Expression
        ArrayLiteralExpression,
        ObjectLiteralExpression,
        PropertyAccessExpression,
        ElementAccessExpression,
        CallExpression,
        NewExpression,
        TaggedTemplateExpression,
        TypeAssertionExpression,
        ParenthesizedExpression,
        FunctionExpression,
        ArrowFunction,
        DeleteExpression,
        TypeOfExpression,
        VoidExpression,
        AwaitExpression,
        PrefixUnaryExpression,
        PostfixUnaryExpression,
        BinaryExpression,
        ConditionalExpression,
        TemplateExpression,
        YieldExpression,
        SpreadElement,
        ClassExpression,
        OmittedExpression,
        ExpressionWithTypeArguments,
        AsExpression,
        NonNullExpression,
        MetaProperty,
        SyntheticExpression,

        // Misc
        TemplateSpan,
        SemicolonClassElement,
        // Element
        Block,
        EmptyStatement,
        VariableStatement,
        ExpressionStatement,
        IfStatement,
        DoStatement,
        WhileStatement,
        ForStatement,
        ForInStatement,
        ForOfStatement,
        ContinueStatement,
        BreakStatement,
        ReturnStatement,
        WithStatement,
        SwitchStatement,
        LabeledStatement,
        ThrowStatement,
        TryStatement,
        DebuggerStatement,
        VariableDeclaration,
        VariableDeclarationList,
        FunctionDeclaration,
        ClassDeclaration,
        InterfaceDeclaration,
        TypeAliasDeclaration,
        EnumDeclaration,
        ModuleDeclaration,
        ModuleBlock,
        CaseBlock,
        NamespaceExportDeclaration,
        ImportEqualsDeclaration,
        ImportDeclaration,
        ImportClause,
        NamespaceImport,
        NamedImports,
        ImportSpecifier,
        ExportAssignment,
        ExportDeclaration,
        NamedExports,
        NamespaceExport,
        ExportSpecifier,
        MissingDeclaration,

        // Module references
        ExternalModuleReference,

        // JSX
        JsxElement,
        JsxSelfClosingElement,
        JsxOpeningElement,
        JsxClosingElement,
        JsxFragment,
        JsxOpeningFragment,
        JsxClosingFragment,
        JsxAttribute,
        JsxAttributes,
        JsxSpreadAttribute,
        JsxExpression,

        // Clauses
        CaseClause,
        DefaultClause,
        HeritageClause,
        CatchClause,
        AssertClause,
        AssertEntry,
        ImportTypeAssertionContainer,

        // Property assignments
        PropertyAssignment,
        ShorthandPropertyAssignment,
        SpreadAssignment,

        // Enum
        EnumMember,
        // Unparsed
        UnparsedPrologue,
        UnparsedPrepend,
        UnparsedText,
        UnparsedInternalText,
        UnparsedSyntheticReference,

        // Top-level nodes
        SourceFile,
        Bundle,
        UnparsedSource,
        InputFiles,

        // JSDoc nodes
        JSDocTypeExpression,
        JSDocNameReference,
        JSDocMemberName, // C#p
        JSDocAllType, // The * type
        JSDocUnknownType, // The ? type
        JSDocNullableType,
        JSDocNonNullableType,
        JSDocOptionalType,
        JSDocFunctionType,
        JSDocVariadicType,
        JSDocNamepathType, // https://jsdoc.app/about-namepaths.html
        /** @deprecated Use SyntaxKind.JSDoc */
        JSDocComment,
        JSDocText,
        JSDocTypeLiteral,
        JSDocSignature,
        JSDocLink,
        JSDocLinkCode,
        JSDocLinkPlain,
        JSDocTag,
        JSDocAugmentsTag,
        JSDocImplementsTag,
        JSDocAuthorTag,
        JSDocDeprecatedTag,
        JSDocClassTag,
        JSDocPublicTag,
        JSDocPrivateTag,
        JSDocProtectedTag,
        JSDocReadonlyTag,
        JSDocOverrideTag,
        JSDocCallbackTag,
        JSDocEnumTag,
        JSDocParameterTag,
        JSDocReturnTag,
        JSDocThisTag,
        JSDocTypeTag,
        JSDocTemplateTag,
        JSDocTypedefTag,
        JSDocSeeTag,
        JSDocPropertyTag,

        // Synthesized list
        SyntaxList,

        // Transformation nodes
        NotEmittedStatement,
        PartiallyEmittedExpression,
        CommaListExpression,
        MergeDeclarationMarker,
        EndOfDeclarationMarker,
        SyntheticReferenceExpression,

        // Enum value count
        Count,

        // Markers
        FirstAssignment = EqualsToken,
        LastAssignment = CaretEqualsToken,
        FirstCompoundAssignment = PlusEqualsToken,
        LastCompoundAssignment = CaretEqualsToken,
        FirstReservedWord = BreakKeyword,
        LastReservedWord = WithKeyword,
        FirstKeyword = BreakKeyword,
        LastKeyword = OfKeyword,
        FirstFutureReservedWord = ImplementsKeyword,
        LastFutureReservedWord = YieldKeyword,
        FirstTypeNode = TypePredicate,
        LastTypeNode = ImportType,
        FirstPunctuation = OpenBraceToken,
        LastPunctuation = CaretEqualsToken,
        FirstToken = Unknown,
        LastToken = LastKeyword,
        FirstTriviaToken = SingleLineCommentTrivia,
        LastTriviaToken = ConflictMarkerTrivia,
        FirstLiteralToken = NumericLiteral,
        LastLiteralToken = NoSubstitutionTemplateLiteral,
        FirstTemplateToken = NoSubstitutionTemplateLiteral,
        LastTemplateToken = TemplateTail,
        FirstBinaryOperator = LessThanToken,
        LastBinaryOperator = CaretEqualsToken,
        FirstStatement = VariableStatement,
        LastStatement = DebuggerStatement,
        FirstNode = QualifiedName,
        FirstJSDocNode = JSDocTypeExpression,
        LastJSDocNode = JSDocPropertyTag,
        FirstJSDocTagNode = JSDocTag,
        LastJSDocTagNode = JSDocPropertyTag,
        /* @internal */ FirstContextualKeyword = AbstractKeyword,
        /* @internal */ LastContextualKeyword = OfKeyword,
        JSDoc = JSDocComment,
    };

    enum class NodeFlags {
        None = 0,
        Let = 1 << 0,  // Variable declaration
        Const = 1 << 1,  // Variable declaration
        NestedNamespace = 1 << 2,  // Namespace declaration
        Synthesized = 1 << 3,  // BaseNode was synthesized during transformation
        Namespace = 1 << 4,  // Namespace declaration
        OptionalChain = 1 << 5,  // Chained MemberExpression rooted to a pseudo-OptionalExpression
        ExportContext = 1 << 6,  // Export context (initialized by binding)
        ContainsThis = 1 << 7,  // Interface contains references to "this"
        HasImplicitReturn = 1 << 8,  // If function implicitly returns on one of codepaths (initialized by binding)
        HasExplicitReturn = 1 << 9,  // If function has explicit reachable return on one of codepaths (initialized by binding)
        GlobalAugmentation = 1 << 10,  // Set if module declaration is an augmentation for the global scope
        HasAsyncFunctions = 1 << 11, // If the file has async functions (initialized by binding)
        DisallowInContext = 1 << 12, // If BaseNode was parsed in a context where 'in-expressions' are not allowed
        YieldContext = 1 << 13, // If BaseNode was parsed in the 'yield' context created when parsing a generator
        DecoratorContext = 1 << 14, // If BaseNode was parsed as part of a decorator
        AwaitContext = 1 << 15, // If BaseNode was parsed in the 'await' context created when parsing an async function
        DisallowConditionalTypesContext = 1 << 16, // If BaseNode was parsed in a context where conditional types are not allowed
        ThisNodeHasError = 1 << 17, // If the parser encountered an error when parsing the code that created this node
        JavaScriptFile = 1 << 18, // If BaseNode was parsed in a JavaScript
        ThisNodeOrAnySubNodesHasError = 1 << 19, // If this BaseNode or any of its children had an error
        HasAggregatedChildData = 1 << 20, // If we've computed data from children and cached it in this node

        // These flags will be set when the parser encounters a dynamic import expression or 'import.meta' to avoid
        // walking the tree if the flags are not set. However, these flags are just a approximation
        // (hence why it's named "PossiblyContainsDynamicImport") because once set, the flags never get cleared.
        // During editing, if a dynamic import is removed, incremental parsing will *NOT* clear this flag.
        // This means that the tree will always be traversed during module resolution, or when looking for external module indicators.
        // However, the removal operation should not occur often and in the case of the
        // removal, it is likely that users will add the import anyway.
        // The advantage of this approach is its simplicity. For the case of batch compilation,
        // we guarantee that users won't have to pay the price of walking the tree if a dynamic import isn't used.
        /* @internal */ PossiblyContainsDynamicImport = 1 << 21,
        /* @internal */ PossiblyContainsImportMeta = 1 << 22,

        JSDoc = 1 << 23, // If BaseNode was parsed inside jsdoc
        /* @internal */ Ambient = 1 << 24, // If BaseNode was inside an ambient context -- a declaration file, or inside something with the `declare` modifier.
        /* @internal */ InWithStatement = 1 << 25, // If any ancestor of BaseNode was the `statement` of a WithStatement (not the `expression`)
        JsonFile = 1 << 26, // If BaseNode was parsed in a Json
        /* @internal */ TypeCached = 1 << 27, // If a type was cached for BaseNode at any point
        /* @internal */ Deprecated = 1 << 28, // If has '@deprecated' JSDoc tag

        BlockScoped = Let | Const,

        ReachabilityCheckFlags = HasImplicitReturn | HasExplicitReturn,
        ReachabilityAndEmitFlags = ReachabilityCheckFlags | HasAsyncFunctions,

        // Parsing context flags
        ContextFlags = DisallowInContext | DisallowConditionalTypesContext | YieldContext | DecoratorContext | AwaitContext | JavaScriptFile | InWithStatement | Ambient,

        // Exclude these flags when parsing a Type
        TypeExcludesFlags = YieldContext | AwaitContext,

        // Represents all flags that are potentially set once and
        // never cleared on SourceFiles which get re-used in between incremental parses.
        // See the comment above on `PossiblyContainsDynamicImport` and `PossiblyContainsImportMeta`.
        /* @internal */ PermanentlySetIncrementalFlags = PossiblyContainsDynamicImport | PossiblyContainsImportMeta,
    };

    enum class ModifierFlags {
        None = 0,
        Export = 1 << 0,  // Declarations
        Ambient = 1 << 1,  // Declarations
        Public = 1 << 2,  // Property/Method
        Private = 1 << 3,  // Property/Method
        Protected = 1 << 4,  // Property/Method
        Static = 1 << 5,  // Property/Method
        Readonly = 1 << 6,  // Property/Method
        Abstract = 1 << 7,  // Class/Method/ConstructSignature
        Async = 1 << 8,  // Property/Method/Function
        Default = 1 << 9,  // Function/Class (export default declaration)
        Const = 1 << 11, // Const enum
        HasComputedJSDocModifiers = 1 << 12, // Indicates the computed modifier flags include modifiers from JSDoc.

        Deprecated = 1 << 13, // Deprecated tag.
        Override = 1 << 14, // Override method.
        In = 1 << 15, // Contravariance modifier
        Out = 1 << 16, // Covariance modifier
        HasComputedFlags = 1 << 29, // Modifier flags have been computed

        AccessibilityModifier = Public | Private | Protected,
        // Accessibility modifiers and 'readonly' can be attached to a parameter in a constructor to make it a property.
        ParameterPropertyModifier = AccessibilityModifier | Readonly | Override,
        NonPublicAccessibilityModifier = Private | Protected,

        TypeScriptModifier = Ambient | Public | Private | Protected | Readonly | Abstract | Const | Override | In | Out,
        ExportDefault = Export | Default,
        All = Export | Ambient | Public | Private | Protected | Static | Readonly | Abstract | Async | Default | Const | Deprecated | Override | In | Out
    };

    enum class TransformFlags {
        None = 0,

        // Facts
        // - Flags used to indicate that a BaseNode or subtree contains syntax that requires transformation.
        ContainsTypeScript = 1 << 0,
        ContainsJsx = 1 << 1,
        ContainsESNext = 1 << 2,
        ContainsES2022 = 1 << 3,
        ContainsES2021 = 1 << 4,
        ContainsES2020 = 1 << 5,
        ContainsES2019 = 1 << 6,
        ContainsES2018 = 1 << 7,
        ContainsES2017 = 1 << 8,
        ContainsES2016 = 1 << 9,
        ContainsES2015 = 1 << 10,
        ContainsGenerator = 1 << 11,
        ContainsDestructuringAssignment = 1 << 12,

        // Markers
        // - Flags used to indicate that a subtree contains a specific transformation.
        ContainsTypeScriptClassSyntax = 1 << 12, // Decorators, Property Initializers, Parameter Property Initializers
        ContainsLexicalThis = 1 << 13,
        ContainsRestOrSpread = 1 << 14,
        ContainsObjectRestOrSpread = 1 << 15,
        ContainsComputedPropertyName = 1 << 16,
        ContainsBlockScopedBinding = 1 << 17,
        ContainsBindingPattern = 1 << 18,
        ContainsYield = 1 << 19,
        ContainsAwait = 1 << 20,
        ContainsHoistedDeclarationOrCompletion = 1 << 21,
        ContainsDynamicImport = 1 << 22,
        ContainsClassFields = 1 << 23,
        ContainsPossibleTopLevelAwait = 1 << 24,
        ContainsLexicalSuper = 1 << 25,
        ContainsUpdateExpressionForIdentifier = 1 << 26,
        // Please leave this as 1 << 29.
        // It is the maximum bit we can set before we outgrow the size of a v8 small integer (SMI) on an x86 system.
        // It is a good reminder of how much room we have left
        HasComputedFlags = 1 << 29, // Transform flags have been computed.

        // Assertions
        // - Bitmasks that are used to assert facts about the syntax of a BaseNode and its subtree.
        AssertTypeScript = ContainsTypeScript,
        AssertJsx = ContainsJsx,
        AssertESNext = ContainsESNext,
        AssertES2022 = ContainsES2022,
        AssertES2021 = ContainsES2021,
        AssertES2020 = ContainsES2020,
        AssertES2019 = ContainsES2019,
        AssertES2018 = ContainsES2018,
        AssertES2017 = ContainsES2017,
        AssertES2016 = ContainsES2016,
        AssertES2015 = ContainsES2015,
        AssertGenerator = ContainsGenerator,
        AssertDestructuringAssignment = ContainsDestructuringAssignment,

        // Scope Exclusions
        // - Bitmasks that exclude flags from propagating out of a specific context
        //   into the subtree flags of their container.
        OuterExpressionExcludes = HasComputedFlags,
        PropertyAccessExcludes = OuterExpressionExcludes,
        NodeExcludes = PropertyAccessExcludes,
        ArrowFunctionExcludes = NodeExcludes | ContainsTypeScriptClassSyntax | ContainsBlockScopedBinding | ContainsYield | ContainsAwait | ContainsHoistedDeclarationOrCompletion | ContainsBindingPattern | ContainsObjectRestOrSpread | ContainsPossibleTopLevelAwait,
        FunctionExcludes = NodeExcludes | ContainsTypeScriptClassSyntax | ContainsLexicalThis | ContainsLexicalSuper | ContainsBlockScopedBinding | ContainsYield | ContainsAwait | ContainsHoistedDeclarationOrCompletion | ContainsBindingPattern | ContainsObjectRestOrSpread | ContainsPossibleTopLevelAwait,
        ConstructorExcludes = NodeExcludes | ContainsLexicalThis | ContainsLexicalSuper | ContainsBlockScopedBinding | ContainsYield | ContainsAwait | ContainsHoistedDeclarationOrCompletion | ContainsBindingPattern | ContainsObjectRestOrSpread | ContainsPossibleTopLevelAwait,
        MethodOrAccessorExcludes = NodeExcludes | ContainsLexicalThis | ContainsLexicalSuper | ContainsBlockScopedBinding | ContainsYield | ContainsAwait | ContainsHoistedDeclarationOrCompletion | ContainsBindingPattern | ContainsObjectRestOrSpread,
        PropertyExcludes = NodeExcludes | ContainsLexicalThis | ContainsLexicalSuper,
        ClassExcludes = NodeExcludes | ContainsTypeScriptClassSyntax | ContainsComputedPropertyName,
        ModuleExcludes = NodeExcludes | ContainsTypeScriptClassSyntax | ContainsLexicalThis | ContainsLexicalSuper | ContainsBlockScopedBinding | ContainsHoistedDeclarationOrCompletion | ContainsPossibleTopLevelAwait,
        TypeExcludes = ~ ContainsTypeScript,
        ObjectLiteralExcludes = NodeExcludes | ContainsTypeScriptClassSyntax | ContainsComputedPropertyName | ContainsObjectRestOrSpread,
        ArrayLiteralOrCallOrNewExcludes = NodeExcludes | ContainsRestOrSpread,
        VariableDeclarationListExcludes = NodeExcludes | ContainsBindingPattern | ContainsObjectRestOrSpread,
        ParameterExcludes = NodeExcludes,
        CatchClauseExcludes = NodeExcludes | ContainsObjectRestOrSpread,
        BindingPatternExcludes = NodeExcludes | ContainsRestOrSpread,
        ContainsLexicalThisOrSuper = ContainsLexicalThis | ContainsLexicalSuper,

        // Propagating flags
        // - Bitmasks for flags that should propagate from a child
        PropertyNamePropagatingFlags = ContainsLexicalThis | ContainsLexicalSuper,

        // Masks
        // - Additional bitmasks
    };
}

namespace ts {
    using namespace std;

    using types::SyntaxKind;

    template<typename T>
    void printElem(vector<SyntaxKind> &kinds, const T &x) {
        kinds.push_back(x.kind);
//        std::cout << x.kind << ',';
    };

    template<typename TupleT, std::size_t... Is>
    void printTupleManual(vector<SyntaxKind> &kinds, const TupleT &tp, std::index_sequence<Is...>) {
        (printElem(kinds, std::get<Is>(tp)), ...);
    }

    template<typename TupleT, std::size_t TupSize = std::tuple_size_v<TupleT>>
    vector<SyntaxKind> getKinds(const TupleT &tp) {
        vector<types::SyntaxKind> kinds;
        printTupleManual(kinds, tp, std::make_index_sequence<TupSize>{});
        return kinds;
    }

    struct ReadonlyTextRange {
        int pos;
        int end;
    };

    struct Decorator;
    struct Modifier;

//    struct Unknown {
//        SyntaxKind kind = SyntaxKind::Unknown;
//    };

//    template<class T>
//    int extractKind2(){
//    //    auto a = declval<T>();
//        std::cout << static_cast<Unknown>(declval<T>()).KIND << "\n";
//        return false;
////        return T::KIND;
//    }

//    struct Node {
//        Unknown *data = nullptr;
//        shared_ptr<Node> parent;
//
//        Node() {
//        }
//
//        Node(Unknown *data) {
//            this->data = data;
//        }
//
//        ~Node();
//        explicit operator bool() const { return data != nullptr; };
//
//        SyntaxKind kind();
//
//        template<class T>
//        bool is() {
//            return data && data->kind == T::KIND;
//        }
//
//        template<typename T>
//        T &to() {
//            auto valid = data && data->kind == T::KIND;
//            if (! valid) throw std::runtime_error("Can not convert Node, invalid kind or no data set");
//            return *reinterpret_cast<T *>(data);
//        }
//
////        BaseNodeStructure &toBase() {
////            if (! data) throw std::runtime_error("Can not convert Node, no data set");
////            return *reinterpret_cast<BaseNodeStructure *>(data);
////        }
//
//        template<typename T>
//        T &toBase() {
//            if (! data) throw std::runtime_error("Can not convert Node, no data set");
//            return *reinterpret_cast<T *>(data);
//        }
//
////        template<typename T>
////        NodeType &toUnion() {
////            T i;
////            if (!data) throw std::runtime_error("Can not convert Node, no data set");
////
////            auto types = i.types();
////
////            for (auto kind: i.kinds()) {
////                if (data->kind == kind) {
////                    auto t = std::get<0>(types);
////                    cout << kind << " FOUND " << t.kind << "\n";
//////                    return *dynamic_cast<t *>(data);
////                }
////            }
////
////            throw std::runtime_error("Can not convert Node, no valid kind");
////        }
//    };


//    template<typename ... T>
//    struct NodeType: public Node {
//        using ETypes = std::tuple<decltype(T{})...>;
//        ETypes types;
//
//        vector<int> kinds() {
//
//        }
//    };

    template<SyntaxKind Kind, class ... B>
    struct BrandKind: B ... {
        constexpr static auto KIND = Kind;
        BrandKind() {
            this->kind = Kind;
        }
    };

    class Node;

    struct BaseNodeArray {
        vector<Node> list;
        int pos;
        int end;
        bool hasTrailingComma = false;

        int length() {
            return list.size();
        }
    };

    template<class ... T>
    struct NodeArray: BaseNodeArray {};

    /**
     * Union is like Node, it is the owner of the data
     */
    struct BaseUnion {
        shared_ptr<Node> node = make_shared<Node>();
        SyntaxKind kind();

        bool empty();
    };

    /**
     * All BaseNode pointers are owned by SourceFile. If SourceFile destroys, all its Nodes are destroyed as well.
     *
     * There are a big variety of sub types: All have in common that they are the owner of their data (except *parent).
     */
    class Node: ReadonlyTextRange {
    protected:
        Node &parent = *this;                                 // Parent BaseNode (initialized by binding)
    public:
        SyntaxKind kind = SyntaxKind::Unknown;
        /* types::NodeFlags */ int flags;
        /* @internal */ /* types::ModifierFlags */ int modifierFlagsCache;
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        /* @internal */ /* types::TransformFlags */ int transformFlags; // Flags for transforms
////        /* @internal */ id?: NodeId;                          // Unique id (used to look up NodeLinks)
//        /* @internal */ original?: Node;                      // The original BaseNode if this is an updated node.
//        /* @internal */ symbol: Symbol;                       // Symbol declared by BaseNode (initialized by binding)
//        /* @internal */ locals?: SymbolTable;                 // Locals associated with BaseNode (initialized by binding)
//        /* @internal */ nextContainer?: Node;                 // Next container in declaration order (initialized by binding)
//        /* @internal */ localSymbol?: Symbol;                 // Local symbol declared by BaseNode (initialized by binding only for exported nodes)
//        /* @internal */ flowNode?: FlowNode;                  // Associated FlowNode (initialized by binding)
//        /* @internal */ emitNode?: EmitNode;                  // Associated EmitNode (initialized by transforms)
//        /* @internal */ contextualType?: Type;                // Used to temporarily assign a contextual type during overload resolution
//        /* @internal */ inferenceContext?: InferenceContext;  // Inference context for contextual type

        bool hasParent() {
            return &parent != this;
        }

        Node &getParent() {
            if (! hasParent()) throw std::runtime_error("Node has no parent set");
            return parent;
        }

        template<class T>
        bool is() {
            if (T::KIND == SyntaxKind::Unknown) throw runtime_error("Passed Node type has unknown kind.");
            return this->kind == T::KIND;
        }

        template<typename T>
        T &to() {
            if (T::KIND == SyntaxKind::Unknown) throw runtime_error("Passed Node type has unknown kind.");
            if (kind != T::KIND) throw std::runtime_error(format("Can not convert Node, from kind %d to %d", kind, T::KIND));
            return *reinterpret_cast<T *>(this);
        }

        //if you know what you are doing
        template<typename T>
        T &cast() {
            return *reinterpret_cast<T *>(this);
        }
    };

//    template<class T>
//    bool is(Node &node) {
//        return data && node.kind == T::KIND;
//    }

    template<typename Default, typename ... Ts>
    struct Union: BaseUnion {
//        std::variant<shared_ptr<Default>, shared_ptr<Ts>...> value = make_shared<Default>();
//    auto types() {
//        using ETypes = std::tuple<decltype(Ts{})...>;
//        ETypes types;
//        return types;
//    }

        Union(){
            this->node = make_shared<Default>();
        }

        operator Node() {
            return *this->node;
        }
//
//        operator const Node() {
//            return *this->node;
//        }

//        operator Node&() {
//            return *this->node;
//        }
//
//        operator const Node&() {
//            return *this->node;
//        }

        operator optional<const Node>() {
            if (node->kind == types::Unknown) return nullopt;
            return *this->node;
        }

//        operator reference_wrapper<Node>() {
//            return *this->node;
//        }

//        operator optional<Node>() {
//            if (node->kind == types::Unknown) return nullopt;
//            return *this->node;
//        }

        optional<Node> lol() {
            if (node->kind == types::Unknown) return nullopt;
            return *this->node;
        }

//        optional<Node> operator=(OptionalNode other) {
//            if (node->kind == types::Unknown) return nullopt;
//            return *this->node;
//        }

        template<typename T>
        bool is() {
            if (T::KIND == SyntaxKind::Unknown) throw runtime_error("Passed Node type has unknown kind.");
            return node->kind == T::KIND; //std::holds_alternative<shared_ptr<T>>(value);
        }

        template<typename T>
        T &to() {
            if (T::KIND == SyntaxKind::Unknown) throw runtime_error("Passed Node type has unknown kind.");
            if (! is<T>()) {
                node = make_shared<T>();
            }
            return node; //*std::get<shared_ptr<T>>(value);
        }
    };

    using OptionalNode = variant<monostate, reference_wrapper<BaseUnion>, optional<reference_wrapper<const BaseUnion>>, optional<reference_wrapper<const Node>>>;

    inline bool empty(OptionalNode v) {
        if (holds_alternative<monostate>(v)) return true;
        if (holds_alternative<reference_wrapper<BaseUnion>>(v)) return false;
        if (holds_alternative<optional<reference_wrapper<const BaseUnion>>>(v)) {
            auto u = get<optional<reference_wrapper<const BaseUnion>>>(v);
            if (! u.has_value()) return true;
            return false;
        }
        auto opt = get<optional<reference_wrapper<const Node>>>(v);
        return opt.has_value();
    }

    inline Node &resolve(OptionalNode v) {
        if (holds_alternative<monostate>(v)) {
            throw runtime_error("OptionalNode is empty");
        }
        if (holds_alternative<reference_wrapper<BaseUnion>>(v)) {
            return *get<reference_wrapper<BaseUnion>>(v).get().node;
        }
        if (holds_alternative<optional<reference_wrapper<const BaseUnion>>>(v)) {
            auto u = get<optional<reference_wrapper<const BaseUnion>>>(v);
            if (! u.has_value()) throw runtime_error("OptionalNode is empty");
            return *u->get().node;
        }

        auto opt = get<optional<reference_wrapper<const Node>>>(v);

        if (! opt.has_value()) throw runtime_error("Optional is empty");

        return *(Node *) (&(opt.value().get()));
    }

    template<typename Default, typename ... Ts>
    struct NodeUnion: Union<Default, Ts...> {
        NodeUnion() {
            this->node = make_shared<Default>();
        }

        NodeUnion(auto node) {
            this->node = make_shared<decltype(node)>();
        }

        operator OptionalNode () {
            return *this->node;
        }

//        operator optional<const Node> () {
//            throw runtime_error("Ads");
//        }

//        NodeUnion& operator=(const SourceFile &node) {
//            this->node = node;
//            return *this;
//        }

//        template<typename T>
//        NodeUnion(const T &node) {
//            this->node = node;
//        }

        /**
         * Casts whatever is current hold as Node.
         */
        Node &getNode() {
            Node *b = nullptr;

            std::visit([&b](auto &arg) {
                b = reinterpret_cast<Node *>(&(*arg));
            }, this->value);

            if (! b) throw std::runtime_error("Union does not hold a Node");

            return *b;
        }
    };

    struct DeclarationName;

    struct Statement: Node {};

    struct NamedDeclaration: Statement {
        optional<reference_wrapper<DeclarationName>> name;
    };

    struct Expression: Node {};

    struct UnaryExpression: Expression {};

    struct UpdateExpression: UnaryExpression {};

    struct LeftHandSideExpression: UpdateExpression {};

    struct MemberExpression: LeftHandSideExpression {};

    struct PrimaryExpression: MemberExpression {};

    struct PrivateIdentifier: BrandKind<SyntaxKind::PrivateIdentifier, PrimaryExpression> {};

    template<SyntaxKind T>
    struct Token: BrandKind<T, Node> {};

    struct DotToken: Token<SyntaxKind::DotToken> {};
    struct DotDotDotToken: Token<SyntaxKind::DotDotDotToken> {};
    struct QuestionToken: Token<SyntaxKind::QuestionToken> {};
    struct ExclamationToken: Token<SyntaxKind::ExclamationToken> {};
    struct ColonToken: Token<SyntaxKind::ColonToken> {};
    struct EqualsToken: Token<SyntaxKind::EqualsToken> {};
    struct AsteriskToken: Token<SyntaxKind::AsteriskToken> {};
    struct EqualsGreaterThanToken: Token<SyntaxKind::EqualsGreaterThanToken> {};
    struct PlusToken: Token<SyntaxKind::PlusToken> {};
    struct MinusToken: Token<SyntaxKind::MinusToken> {};
    struct QuestionDotToken: Token<SyntaxKind::QuestionDotToken> {};

    struct AssertsKeyword: Token<SyntaxKind::AssertsKeyword> {};
    struct AssertKeyword: Token<SyntaxKind::AssertKeyword> {};
    struct AwaitKeyword: Token<SyntaxKind::AwaitKeyword> {};

    struct AbstractKeyword: Token<SyntaxKind::AbstractKeyword> {};
    struct AsyncKeyword: Token<SyntaxKind::AsyncKeyword> {};
    struct ConstKeyword: Token<SyntaxKind::ConstKeyword> {};
    struct DeclareKeyword: Token<SyntaxKind::DeclareKeyword> {};
    struct DefaultKeyword: Token<SyntaxKind::DefaultKeyword> {};
    struct ExportKeyword: Token<SyntaxKind::ExportKeyword> {};
    struct InKeyword: Token<SyntaxKind::InKeyword> {};
    struct PrivateKeyword: Token<SyntaxKind::PrivateKeyword> {};
    struct ProtectedKeyword: Token<SyntaxKind::ProtectedKeyword> {};
    struct PublicKeyword: Token<SyntaxKind::PublicKeyword> {};
    struct ReadonlyKeyword: Token<SyntaxKind::ReadonlyKeyword> {};
    struct OutKeyword: Token<SyntaxKind::OutKeyword> {};
    struct OverrideKeyword: Token<SyntaxKind::OverrideKeyword> {};
    struct StaticKeyword: Token<SyntaxKind::StaticKeyword> {};

    struct NullLiteral: BrandKind<SyntaxKind::NullKeyword, PrimaryExpression> {};

    struct TrueLiteral: BrandKind<SyntaxKind::TrueKeyword, PrimaryExpression> {};

    struct FalseLiteral: BrandKind<SyntaxKind::FalseKeyword, PrimaryExpression> {};

    struct BooleanLiteral: NodeUnion<TrueLiteral, FalseLiteral> {};

    struct ThisExpression: BrandKind<SyntaxKind::ThisKeyword, PrimaryExpression> {};

    struct SuperExpression: BrandKind<SyntaxKind::SuperKeyword, PrimaryExpression> {};

    struct ImportExpression: BrandKind<SyntaxKind::ImportKeyword, PrimaryExpression> {};

    using PostfixUnaryOperator = SyntaxKind; //SyntaxKind.PlusPlusToken | SyntaxKind.MinusMinusToken

    struct PrefixUnaryExpression: BrandKind<SyntaxKind::PrefixUnaryExpression, UpdateExpression> {
        LeftHandSideExpression operand;
        PostfixUnaryOperator operatorKind;
    };

    struct PartiallyEmittedExpression: BrandKind<SyntaxKind::PartiallyEmittedExpression, LeftHandSideExpression> {
        Expression expression;
    };

    struct PostfixUnaryExpression: BrandKind<SyntaxKind::PostfixUnaryExpression, UpdateExpression> {
        LeftHandSideExpression operand;
        PostfixUnaryOperator operatorKind;
    };

    struct DeleteExpression: BrandKind<SyntaxKind::DeleteExpression, UnaryExpression> {
        UnaryExpression expression;
    };

    struct TypeOfExpression: BrandKind<SyntaxKind::TypeOfExpression, UnaryExpression> {
        UnaryExpression expression;
    };

    struct VoidExpression: BrandKind<SyntaxKind::VoidExpression, UnaryExpression> {
        UnaryExpression expression;
    };

    struct AwaitExpression: BrandKind<SyntaxKind::AwaitExpression, UnaryExpression> {
        UnaryExpression expression;
    };

    struct YieldExpression: BrandKind<SyntaxKind::YieldExpression, Expression> {
        optional<AsteriskToken> asteriskToken;
        optional<Expression> expression;
    };

    //this seems to be related to instantiated types
    struct Type {};

    struct ParameterDeclaration;
    struct NamedTupleMember;

    struct SyntheticExpression: BrandKind<SyntaxKind::SyntheticExpression, Expression> {
        bool isSpread;
        Type type;
        optional<NodeUnion<ParameterDeclaration, NamedTupleMember>> tupleNameSource;
    };

    struct TypeNode: Node {};

    /** @deprecated Use `AwaitKeyword` instead. */
    using AwaitKeywordToken = AwaitKeyword;

    /** @deprecated Use `AssertsKeyword` instead. */
    using AssertsToken = AssertsKeyword;

    /** @deprecated Use `ReadonlyKeyword` instead. */
    using ReadonlyToken = ReadonlyKeyword;

    struct Modifier: NodeUnion<
            AbstractKeyword, AsyncKeyword, ConstKeyword, DeclareKeyword, DefaultKeyword, ExportKeyword, InKeyword, PrivateKeyword, ProtectedKeyword, PublicKeyword, OutKeyword, OverrideKeyword, ReadonlyKeyword, StaticKeyword> {
    };

    struct ModifiersArray: NodeArray<Modifier> {};

    struct LiteralLikeNode {
        std::string text;
        bool isUnterminated; //optional
        bool hasExtendedUnicodeEscape; //optional
    };

    struct LiteralExpression: LiteralLikeNode, PrimaryExpression {};

    struct StringLiteral: BrandKind<SyntaxKind::StringLiteral, LiteralExpression> {};

    struct Identifier: BrandKind<SyntaxKind::Identifier, PrimaryExpression> {
        /**
         * Prefer to use `id.unescapedText`. (Note: This is available only in services, not internally to the TypeScript compiler.)
         * Text of identifier, but if the identifier begins with two underscores, this will begin with three.
         */
        string escapedText;
        SyntaxKind originalKeywordKind = SyntaxKind::Unknown;
    };

    enum class FlowFlags {
        Unreachable = 1 << 0,  // Unreachable code
        Start = 1 << 1,  // Start of flow graph
        BranchLabel = 1 << 2,  // Non-looping junction
        LoopLabel = 1 << 3,  // Looping junction
        Assignment = 1 << 4,  // Assignment
        TrueCondition = 1 << 5,  // Condition known to be true
        FalseCondition = 1 << 6,  // Condition known to be false
        SwitchClause = 1 << 7,  // Switch statement clause
        ArrayMutation = 1 << 8,  // Potential array mutation
        Call = 1 << 9,  // Potential assertion call
        ReduceLabel = 1 << 10, // Temporarily reduce antecedents of label
        Referenced = 1 << 11, // Referenced as antecedent once
        Shared = 1 << 12, // Referenced as antecedent more than once

        Label = BranchLabel | LoopLabel,
        Condition = TrueCondition | FalseCondition,
    };

    struct Block: BrandKind<SyntaxKind::Block, Statement> {
        NodeArray<Statement> statements;
        /*@internal*/ bool multiLine;
    };

    struct TemplateLiteralLikeNode: LiteralLikeNode {
        optional<string> rawText;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct NoSubstitutionTemplateLiteral: BrandKind<SyntaxKind::NoSubstitutionTemplateLiteral, LiteralExpression, TemplateLiteralLikeNode> {
        optional<types::TokenFlags> templateFlags;
    };

    struct NumericLiteral: BrandKind<SyntaxKind::NumericLiteral, LiteralExpression> {
        types::TokenFlags numericLiteralFlags;
    };

    struct ComputedPropertyName: BrandKind<SyntaxKind::ComputedPropertyName> {
        Expression expression;
    };

    struct QualifiedName;

    #define EntityName Identifier, QualifiedName

    struct QualifiedName: BrandKind<SyntaxKind::QualifiedName, Node> {
        NodeUnion<EntityName> left;
        Identifier right;
        /*@internal*/ optional<int> jsdocDotPos; // QualifiedName occurs in JSDoc-style generic: Id1.Id2.<T>
    };

    struct ElementAccessExpression: BrandKind<SyntaxKind::ElementAccessExpression, MemberExpression> {
        LeftHandSideExpression expression;
        optional<QuestionDotToken> questionDotToken;
        Expression argumentExpression;
    };

    struct OmittedExpression: BrandKind<SyntaxKind::OmittedExpression, Node> {};

    struct VariableDeclaration;
    struct ParameterDeclaration;
    struct ObjectBindingPattern;
    struct ArrayBindingPattern;
    struct ArrayBindingPattern;
    using BindingPattern = NodeUnion<ObjectBindingPattern, ArrayBindingPattern>;
    using BindingName = NodeUnion<Identifier, BindingPattern>;

    using PropertyName = NodeUnion<Identifier, StringLiteral, NumericLiteral, ComputedPropertyName, PrivateIdentifier>;

    struct BindingElement: BrandKind<SyntaxKind::BindingElement, NamedDeclaration> {
        optional<PropertyName> propertyName;        // Binding property name (in object binding pattern)
        optional<DotDotDotToken> dotDotDotToken;    // Present on rest element (in object binding pattern)
        BindingName name;                  // Declared binding element name
        optional<Expression> initializer;           // Optional initializer
    };

    using ArrayBindingElement = NodeUnion<BindingElement, OmittedExpression>;

    struct ObjectBindingPattern: BrandKind<SyntaxKind::ObjectBindingPattern, Node> {
        NodeArray<BindingElement> elements;
        NodeUnion<VariableDeclaration, ParameterDeclaration, BindingElement> parent;
    };

    struct ArrayBindingPattern: BrandKind<SyntaxKind::ArrayBindingPattern, Node> {
        NodeUnion<VariableDeclaration, ParameterDeclaration, BindingElement> parent;
        NodeArray<ArrayBindingElement> elements;
    };

    struct ExpressionStatement: BrandKind<SyntaxKind::ExpressionStatement, Statement> {
        Expression expression;
    };

    struct PrologueDirective: ExpressionStatement {
        StringLiteral expression;
    };

    struct IfStatement: BrandKind<SyntaxKind::IfStatement, Statement> {
        Expression expression;
        Statement thenStatement;
        optional<Statement> elseStatement;
    };

//    export type ForInitializer =
//        | VariableDeclarationList
//        | Expression
//        ;

//    export type ForInOrOfStatement =
//        | ForInStatement
//        | ForOfStatement
//        ;


    struct BreakStatement: BrandKind<SyntaxKind::BreakStatement, Statement> {
        optional<Identifier> label;
    };

    struct ContinueStatement: BrandKind<SyntaxKind::ContinueStatement, Statement> {
        optional<Identifier> label;
    };

//    export type BreakOrContinueStatement =
//        | BreakStatement
//        | ContinueStatement
//        ;

    struct ReturnStatement: BrandKind<SyntaxKind::ReturnStatement, Statement> {
        optional<Expression> expression;
    };

    struct WithStatement: BrandKind<SyntaxKind::WithStatement, Statement> {
        Expression expression;
        Statement statement;
    };

    struct SwitchStatement;
    struct CaseClause;
    struct DefaultClause;

    struct CaseBlock: BrandKind<SyntaxKind::CaseBlock, Node> {
        SwitchStatement &parent;
        NodeArray<CaseClause, DefaultClause> clauses;
    };

    struct SwitchStatement: BrandKind<SyntaxKind::SwitchStatement, Statement> {
        Expression expression;
        CaseBlock caseBlock;
        bool possiblyExhaustive; // initialized by binding
    };

    struct CaseClause: BrandKind<SyntaxKind::CaseClause, Node> {
        CaseBlock parent;
        Expression expression;
        NodeArray<Statement> statements;
    };

    struct DefaultClause: BrandKind<SyntaxKind::DefaultClause, Node> {
        CaseBlock &parent;
        NodeArray<Statement> statements;
    };

//    export type CaseOrDefaultClause =
//        | CaseClause
//        | DefaultClause
//        ;

    struct LabeledStatement: BrandKind<SyntaxKind::LabeledStatement, Statement> {
        Identifier label;
        Statement statement;
    };

    struct ThrowStatement: BrandKind<SyntaxKind::ThrowStatement, Statement> {
        Expression expression;
    };

    struct IterationStatement: Statement {
        Statement statement;
    };

    struct DoStatement: BrandKind<SyntaxKind::DoStatement, IterationStatement> {
        Expression expression;
    };

    struct WhileStatement: BrandKind<SyntaxKind::WhileStatement, IterationStatement> {
        Expression expression;
    };

    struct VariableDeclarationList;

    using ForInitializer = NodeUnion<VariableDeclarationList, Expression>;

    struct ForStatement: BrandKind<SyntaxKind::ForStatement, IterationStatement> {
        optional<ForInitializer> initializer;
        optional<Expression> condition;
        optional<Expression> incrementor;
    };

    struct ForOfStatement: BrandKind<SyntaxKind::ForOfStatement, IterationStatement> {
        AwaitKeyword awaitModifier;
        ForInitializer initializer;
        Expression expression;
    };

    struct ForInStatement: BrandKind<SyntaxKind::ForInStatement, IterationStatement> {
        ForInitializer initializer;
        Expression expression;
    };

    struct VariableStatement;

    struct VariableDeclarationList: BrandKind<SyntaxKind::VariableDeclarationList, Node> {
        NodeUnion<VariableStatement, ForStatement, ForOfStatement, ForInStatement> parent;
        NodeArray<VariableDeclaration> declarations;
    };

    struct VariableStatement: BrandKind<SyntaxKind::VariableStatement, Statement> {
//        /* @internal*/ optional<NodeArray<Decorator>> decorators; // Present for use with reporting a grammar error
        VariableDeclarationList declarationList;
    };

    struct CatchClause;

    struct TryStatement: BrandKind<SyntaxKind::TryStatement, Statement> {
        Block tryBlock;
        optional<reference_wrapper<CatchClause>> catchClause;
        optional<Block> finallyBlock;
    };

    struct CatchClause: BrandKind<SyntaxKind::CatchClause, Node> {
        TryStatement &parent;
        optional<reference_wrapper<VariableDeclaration>> variableDeclaration;
        Block block;
    };

    struct VariableDeclaration: BrandKind<SyntaxKind::VariableDeclaration, NamedDeclaration> {
        BindingName name;                    // Declared variable name
        NodeUnion<VariableDeclarationList, CatchClause> &parent;
        optional<ExclamationToken> exclamationToken;  // Optional definite assignment assertion
        optional<TypeNode> type;                      // Optional type annotation
        optional<Expression> initializer;             // Optional initializer
    };

    struct MemberName: NodeUnion<Identifier, PrivateIdentifier> {};

    struct PropertyAccessExpression: BrandKind<SyntaxKind::PropertyAccessExpression, MemberExpression, NamedDeclaration> {
        LeftHandSideExpression expression;
        optional<QuestionDotToken> questionDotToken;
        MemberName name;
    };

    struct PropertyAccessEntityNameExpression;

    struct EntityNameExpression: NodeUnion<Identifier, PropertyAccessEntityNameExpression> {};

    struct PropertyAccessEntityNameExpression: PropertyAccessExpression {
        EntityNameExpression expression;
        Identifier name;
    };

    using PropertyName = NodeUnion<Identifier, StringLiteral, NumericLiteral, ComputedPropertyName, PrivateIdentifier>;

    using StringLiteralLike = NodeUnion<StringLiteral, NoSubstitutionTemplateLiteral>;
    struct DeclarationName: NodeUnion<Identifier, PrivateIdentifier, StringLiteralLike, NumericLiteral, ComputedPropertyName, ElementAccessExpression, BindingPattern, EntityNameExpression> {};

    struct MetaProperty: BrandKind<SyntaxKind::MetaProperty, PrimaryExpression> {
        SyntaxKind keywordToken = SyntaxKind::NewKeyword; //: SyntaxKind.NewKeyword | SyntaxKind.ImportKeyword;
        Identifier name;
    };

    struct ObjectLiteralElement: NamedDeclaration {
        optional<PropertyName> name;
    };

    struct ClassElement: NamedDeclaration {
        optional<PropertyName> name;
    };

    struct TypeElement: NamedDeclaration {
        optional<PropertyName> name;
        optional<QuestionToken> questionToken;
    };

    struct SpreadAssignment: BrandKind<SyntaxKind::SpreadAssignment, Node> {
        Expression expression;
    };

    struct TypeLiteralNode: BrandKind<SyntaxKind::TypeLiteral, TypeNode> {
        NodeArray<TypeElement> members;
    };

#define ClassLikeDeclaration ClassDeclaration, ClassExpression

    struct ShorthandPropertyAssignment: BrandKind<SyntaxKind::ShorthandPropertyAssignment, ObjectLiteralElement> {
        Identifier name;
        optional<QuestionToken> questionToken;
        optional<ExclamationToken> exclamationToken;

        // used when ObjectLiteralExpression is used in ObjectAssignmentPattern
        // it is a grammar error to appear in actual object initializer:
        optional<EqualsToken> equalsToken;
        optional<Expression> objectAssignmentInitializer;
    };

//    struct VariableDeclaration: BrandKind<SyntaxKind::VariableDeclaration, NamedDeclaration> {
//        BindingNameNode name;                    // Declared variable name
////            readonly kind: SyntaxKind.VariableDeclaration;
////            readonly parent: VariableDeclarationList | CatchClause;
//        optional <NodeType<ExclamationToken>> exclamationToken;  // Optional definite assignment assertion
//        optional <TypeNode> type; // Optional type annotation
//        optional <NodeType<Expression>> initializer; // Optional initializer
//    };

    struct TypeParameterDeclaration: BrandKind<SyntaxKind::TypeParameter, NamedDeclaration> {
        inline static auto kind = SyntaxKind::TypeParameter;
//        BaseNode *parent; //: DeclarationWithTypeParameterChildren | InferTypeNode;
        Identifier name;
        /** Note: Consider calling `getEffectiveConstraintOfTypeParameter` */
        TypeNode constraint;
        TypeNode defaultType;

        // For error recovery purposes.
        optional<Expression> expression;
    };

    struct ParameterDeclaration: BrandKind<SyntaxKind::Parameter, NamedDeclaration> {
        optional<DotDotDotToken> dotDotDotToken;
        BindingName name;
        optional<QuestionToken> questionToken;
        optional<TypeNode> type;
        optional<Expression> initializer;
    };

    struct PropertyDeclaration: BrandKind<SyntaxKind::PropertyDeclaration, ClassElement> {
        optional<DotDotDotToken> dotDotDotToken;
        BindingName name;
        optional<QuestionToken> questionToken;
        optional<ExclamationToken> exclamationToken;
        optional<TypeNode> type;
        optional<Expression> initializer;
    };

    struct SignatureDeclarationBase: NamedDeclaration {
        optional<NodeUnion<Identifier, StringLiteral, NumericLiteral, ComputedPropertyName, PrivateIdentifier>> name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        NodeArray<ParameterDeclaration> parameters;
        optional<TypeNode> type;
        optional<NodeArray<TypeNode>> typeArguments;
    };

    struct PropertyAssignment: BrandKind<SyntaxKind::PropertyAssignment, ObjectLiteralElement> {
        PropertyName name;
        optional<QuestionToken> questionToken;
        optional<ExclamationToken> exclamationToken;
        optional<Expression> initializer;
    };

    struct FunctionLikeDeclarationBase: SignatureDeclarationBase {
        optional<AsteriskToken> asteriskToken;
        optional<QuestionToken> questionToken;
        optional<ExclamationToken> exclamationToken;
        optional<NodeUnion<Block, Expression>> body;

//        /* @internal */ optional<NodeType<FlowNode>> endFlowNode;
//        /* @internal */ optional<NodeType<FlowNode>> returnFlowNode;
    };

#define FunctionBody Block
#define ConciseBody FunctionBody, Expression

    struct ArrowFunction: BrandKind<SyntaxKind::ArrowFunction, Expression, FunctionLikeDeclarationBase> {
        EqualsGreaterThanToken equalsGreaterThanToken;
        NodeUnion<ConciseBody> body;
    };

    struct HeritageClause;

    struct ClassLikeDeclarationBase: NamedDeclaration {
        optional<Identifier> name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        optional<NodeArray<HeritageClause>> heritageClauses;
        NodeArray<ClassElement> members;
    };

    struct DeclarationStatement: NamedDeclaration {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        optional<NodeUnion<Identifier, StringLiteral, NumericLiteral>> name;
    };

    struct EmptyStatement: BrandKind<SyntaxKind::EmptyStatement, Statement> {};

    struct DebuggerStatement: BrandKind<SyntaxKind::DebuggerStatement, Statement> {};

    struct CommaListExpression: BrandKind<SyntaxKind::CommaListExpression, Expression> {
        NodeArray<Expression> elements;
    };

    struct MissingDeclaration: BrandKind<SyntaxKind::MissingDeclaration, DeclarationStatement> {
        optional<Identifier> name;
    };

    struct ClassDeclaration: BrandKind<SyntaxKind::ClassDeclaration, ClassLikeDeclarationBase, DeclarationStatement> {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        optional<Identifier> name;
    };

    struct ClassExpression: BrandKind<SyntaxKind::ClassExpression, ClassLikeDeclarationBase, PrimaryExpression> {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
    };

    struct HeritageClause;

    struct InterfaceDeclaration: BrandKind<SyntaxKind::InterfaceDeclaration, DeclarationStatement> {
        Identifier name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        optional<NodeArray<HeritageClause>> heritageClauses;
        NodeArray<TypeElement> members;
    };

    struct NodeWithTypeArguments: TypeNode {
        optional<NodeArray<TypeNode>> typeArguments;
    };

    struct ExpressionWithTypeArguments: BrandKind<SyntaxKind::ExpressionWithTypeArguments, MemberExpression, NodeWithTypeArguments> {
        LeftHandSideExpression expression;
    };

    struct HeritageClause: BrandKind<SyntaxKind::HeritageClause, Node> {
        NodeUnion<InterfaceDeclaration, ClassLikeDeclaration> parent;
        SyntaxKind token; //SyntaxKind.ExtendsKeyword | SyntaxKind.ImplementsKeyword
        NodeArray<ExpressionWithTypeArguments> types;
    };

    struct TypeAliasDeclaration: BrandKind<SyntaxKind::TypeAliasDeclaration, DeclarationStatement> {
        Identifier name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        TypeNode type;
    };

    struct EnumMember;

    struct EnumDeclaration: BrandKind<SyntaxKind::EnumDeclaration, DeclarationStatement> {
        Identifier name;
        NodeArray<EnumMember> members;
    };

    struct EnumMember: BrandKind<SyntaxKind::EnumMember, NamedDeclaration> {
        EnumDeclaration parent;
        // This does include ComputedPropertyName, but the parser will give an error
        // if it parses a ComputedPropertyName in an EnumMember
        PropertyName name;
        optional<Expression> initializer;
    };

    struct ClassStaticBlockDeclaration: BrandKind<SyntaxKind::ClassStaticBlockDeclaration, ClassElement> {
        NodeUnion<ClassDeclaration, ClassExpression> parent;
        Block body;
//        /* @internal */ endFlowNode?: FlowNode;
//        /* @internal */ returnFlowNode?: FlowNode;
    };

    struct PropertySignature: BrandKind<SyntaxKind::PropertySignature, TypeElement> {
        PropertyName name;
        optional<TypeNode> type;
        optional<Expression> initializer;
    };

    struct TypeReferenceNode: BrandKind<SyntaxKind::TypeReference, NodeWithTypeArguments> {
        NodeUnion<EntityName> typeName;
    };

#define ModuleName Identifier, StringLiteral
#define NamespaceBody ModuleBlock, NamespaceDeclaration
#define ModuleBody NamespaceBody

    struct ModuleBlock;
    struct NamespaceDeclaration;

    struct ModuleDeclaration: BrandKind<SyntaxKind::ModuleDeclaration, DeclarationStatement> {
        NodeUnion<ModuleBlock, NamespaceDeclaration, SourceFile> parent;
        NodeUnion<ModuleName> name;
        optional<NodeUnion<ModuleBody>> body;
    };

    struct ModuleBlock: BrandKind<SyntaxKind::ModuleBlock, Statement> {
        ModuleDeclaration &parent;
        NodeArray<Statement> statements;
    };

    struct NamespaceDeclaration: BrandKind<SyntaxKind::ModuleDeclaration, DeclarationStatement> {
        NodeUnion<ModuleBody, SourceFile> parent;
        Identifier name;
        optional<NodeUnion<ModuleBody>> body;
    };

    struct ExternalModuleReference;

#define ModuleReference EntityName, ExternalModuleReference

/**
 * One of:
 * - import x = require("mod");
 * - import x = M.x;
 */
    struct ImportEqualsDeclaration: BrandKind<SyntaxKind::ImportEqualsDeclaration, DeclarationStatement> {
        NodeUnion<SourceFile, ModuleBlock> parent;
        Identifier name;
        bool isTypeOnly;

        // 'EntityName' for an internal module reference, 'ExternalModuleReference' for an external
        // module reference.
        NodeUnion<ModuleReference> moduleReference;
    };

    struct ExternalModuleReference: BrandKind<SyntaxKind::ImportEqualsDeclaration, Node> {
        ImportEqualsDeclaration parent;
        Expression expression;
    };

    struct ImportDeclaration;

    struct ImportClause;
    struct NamespaceImport: BrandKind<SyntaxKind::NamespaceImport, NamedDeclaration> {
        ImportClause &parent;
        Identifier name;
    };

    struct NamedImports;
    struct ImportSpecifier: BrandKind<SyntaxKind::ImportSpecifier, NamedDeclaration> {
        NamedImports &parent;
        optional<Identifier> propertyName;  // Name preceding "as" keyword (or undefined when "as" is absent)
        Identifier name;           // Declared name
        bool isTypeOnly;
    };

    struct NamedImports: BrandKind<SyntaxKind::NamedImports, Node> {
        ImportClause &parent;
        NodeArray<ImportSpecifier> elements;
    };

#define NamedImportBindings NamespaceImport, NamedImports
#define NamedExportBindings NamespaceExport, NamedExports

#define AssertionKey Identifier, StringLiteral

    struct AssertClause;
    struct AssertEntry: BrandKind<SyntaxKind::AssertEntry, Node> {
        AssertClause &parent;
        NodeUnion<AssertionKey> name;
        Expression value;
    };

    struct ExportDeclaration;
    struct AssertClause: BrandKind<SyntaxKind::AssertClause, Node> {
        NodeUnion<ImportDeclaration, ExportDeclaration> parent;
        NodeArray<AssertEntry> elements;
        bool multiLine;
    };

    // In case of:
    // import d from "mod" => name = d, namedBinding = undefined
    // import * as ns from "mod" => name = undefined, namedBinding: NamespaceImport = { name: ns }
    // import d, * as ns from "mod" => name = d, namedBinding: NamespaceImport = { name: ns }
    // import { a, b as x } from "mod" => name = undefined, namedBinding: NamedImports = { elements: [{ name: a }, { name: x, propertyName: b}]}
    // import d, { a, b as x } from "mod" => name = d, namedBinding: NamedImports = { elements: [{ name: a }, { name: x, propertyName: b}]}
    struct ImportClause: BrandKind<SyntaxKind::ImportClause, NamedDeclaration> {
        ImportDeclaration &parent;
        bool isTypeOnly;
        optional<Identifier> name; // Default binding
        optional<NodeUnion<NamedImportBindings>> namedBindings;
    };

    // In case of:
    // import "mod"  => importClause = undefined, moduleSpecifier = "mod"
    // In rest of the cases, module specifier is string literal corresponding to module
    // ImportClause information is shown at its declaration below.
    struct ImportDeclaration: BrandKind<SyntaxKind::ImportDeclaration, Statement> {
        NodeUnion<SourceFile, ModuleBlock> parent;
        optional<ImportClause> importClause;
        /** If this is not a StringLiteral it will be a grammar error. */
        Expression moduleSpecifier;
        optional<AssertClause> assertClause;
    };

    struct NamespaceExport: BrandKind<SyntaxKind::NamespaceExport, NamedDeclaration> {
        ExportDeclaration &parent;
        Identifier name;
    };

    struct NamedExports;
    struct ExportSpecifier: BrandKind<SyntaxKind::ExportSpecifier, NamedDeclaration> {
        NamedExports &parent;
        bool isTypeOnly;
        optional<Identifier> propertyName;  // Name preceding "as" keyword (or undefined when "as" is absent)
        Identifier name;           // Declared name
    };

    struct NamedExports: BrandKind<SyntaxKind::NamedExports, Node> {
        ExportDeclaration &parent;
        NodeArray<ExportSpecifier> elements;
    };

    struct NamespaceExportDeclaration: BrandKind<SyntaxKind::NamespaceExportDeclaration, DeclarationStatement> {
        Identifier name;
        /* @internal */ optional<NodeArray<Decorator>> decorators; // Present for use with reporting a grammar error
        /* @internal */ optional<ModifiersArray> modifiers; // Present for use with reporting a grammar error
    };

    struct ExportDeclaration: BrandKind<SyntaxKind::ExportDeclaration, DeclarationStatement> {
        NodeUnion<SourceFile, ModuleBlock> parent;
        bool isTypeOnly;
        /** Will not be assigned in the case of `export * from "foo";` */
        optional<NodeUnion<NamespaceExport, NamedExports>> exportClause;
        /** If this is not a StringLiteral it will be a grammar error. */
        optional<Expression> moduleSpecifier;
        optional<AssertClause> assertClause;
    };

#define NamedImportsOrExports NamedImports, NamedExports
#define ImportOrExportSpecifier ImportSpecifier, ExportSpecifier
#define TypeOnlyCompatibleAliasDeclaration ImportClause, ImportEqualsDeclaration, NamespaceImport, ImportOrExportSpecifier;

//    struct TypeOnlyAliasDeclaration =
//        | ImportClause & { readonly isTypeOnly: true, readonly name: Identifier }
//        | ImportEqualsDeclaration & { readonly isTypeOnly: true }
//        | NamespaceImport & { readonly parent: ImportClause & { readonly isTypeOnly: true } }
//        | ImportSpecifier & ({ readonly isTypeOnly: true } | { readonly parent: NamedImports & { readonly parent: ImportClause & { readonly isTypeOnly: true } } })
//        | ExportSpecifier & ({ readonly isTypeOnly: true } | { readonly parent: NamedExports & { readonly parent: ExportDeclaration & { readonly isTypeOnly: true } } })
//        ;

/**
 * This is either an `export =` or an `export default` declaration.
 * Unless `isExportEquals` is set, this node was parsed as an `export default`.
 */
    struct ExportAssignment: BrandKind<SyntaxKind::ExportAssignment, DeclarationStatement> {
        SourceFile &parent;
        bool isExportEquals;
        Expression expression;
    };

    struct ImportTypeNode;

    struct ImportTypeAssertionContainer: BrandKind<SyntaxKind::ImportTypeAssertionContainer, Node> {
        ImportTypeNode &parent;
        AssertClause assertClause;
        bool multiLine = false;
    };

    struct ImportTypeNode: BrandKind<SyntaxKind::ImportType, NodeWithTypeArguments> {
        bool isTypeOf;
        TypeNode argument;
        optional<ImportTypeAssertionContainer> assertions;
        optional<NodeUnion<EntityName>> qualifier;
    };

    struct ThisTypeNode: BrandKind<SyntaxKind::ThisType, TypeNode> {};

    struct CallSignatureDeclaration: BrandKind<SyntaxKind::CallSignature, SignatureDeclarationBase, TypeElement> {};
    struct ConstructSignatureDeclaration: BrandKind<SyntaxKind::ConstructSignature, SignatureDeclarationBase, TypeElement> {};

#define ObjectTypeDeclaration ClassLikeDeclaration, InterfaceDeclaration, TypeLiteralNode

    struct MethodSignature: BrandKind<SyntaxKind::MethodSignature, SignatureDeclarationBase, TypeElement> {
        NodeUnion<ObjectTypeDeclaration> &parent;
        PropertyName name;
    };

    struct IndexSignatureDeclaration: BrandKind<SyntaxKind::IndexSignature, SignatureDeclarationBase, ClassElement, TypeElement> {
        NodeUnion<ObjectTypeDeclaration> &parent;
//        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
//        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        TypeNode type;
    };

    struct FunctionOrConstructorTypeNodeBase: TypeNode, SignatureDeclarationBase {
        TypeNode type;
    };

    struct FunctionTypeNode: BrandKind<SyntaxKind::FunctionType, FunctionOrConstructorTypeNodeBase> {};

    struct ConstructorTypeNode: BrandKind<SyntaxKind::ConstructorType, FunctionOrConstructorTypeNodeBase> {};

    struct FunctionDeclaration: FunctionLikeDeclarationBase, DeclarationStatement, BrandKind<types::FunctionDeclaration> {
        optional<Identifier> name;
        optional<FunctionBody> body;
    };

    struct ObjectLiteralExpression;

    // Note that a MethodDeclaration is considered both a ClassElement and an ObjectLiteralElement.
    // Both the grammars for ClassDeclaration and ObjectLiteralExpression allow for MethodDeclarations
    // as child elements, and so a MethodDeclaration satisfies both interfaces.  This avoids the
    // alternative where we would need separate kinds/types for ClassMethodDeclaration and
    // ObjectLiteralMethodDeclaration, which would look identical.
    //
    // Because of this, it may be necessary to determine what sort of MethodDeclaration you have
    // at later stages of the compiler pipeline.  In that case, you can either check the parent kind
    // of the method, or use helpers like isObjectLiteralMethodDeclaration
    struct MethodDeclaration: BrandKind<SyntaxKind::MethodDeclaration, FunctionLikeDeclarationBase, ClassElement, ObjectLiteralElement> {
        NodeUnion<ClassLikeDeclaration, ObjectLiteralExpression> parent;
        PropertyName name;
        NodeUnion<FunctionBody> body;
        /* @internal*/ optional<ExclamationToken> exclamationToken; // Present for use with reporting a grammar error
    };

    struct ConstructorDeclaration: FunctionLikeDeclarationBase, ClassElement, BrandKind<types::Constructor> {
        NodeUnion<ClassLikeDeclaration> &parent;
        optional<FunctionBody> body;

        /* @internal */ optional<NodeArray<TypeParameterDeclaration>> typeParameters; // Present for use with reporting a grammar error
        /* @internal */ optional<TypeNode> type; // Present for use with reporting a grammar error
    };

    // See the comment on MethodDeclaration for the intuition behind GetAccessorDeclaration being a
    // ClassElement and an ObjectLiteralElement.
    struct GetAccessorDeclaration: BrandKind<SyntaxKind::GetAccessor, FunctionLikeDeclarationBase, ClassElement, TypeElement, ObjectLiteralElement> {
        NodeUnion<ClassLikeDeclaration, ObjectLiteralExpression, TypeLiteralNode, InterfaceDeclaration> parent;
        PropertyName name;
        optional<FunctionBody> body;
        /* @internal */ optional<NodeArray<TypeParameterDeclaration>> typeParameters; // Present for use with reporting a grammar error
    };

    // See the comment on MethodDeclaration for the intuition behind SetAccessorDeclaration being a
    // ClassElement and an ObjectLiteralElement.
    struct SetAccessorDeclaration: BrandKind<SyntaxKind::SetAccessor, FunctionLikeDeclarationBase, ClassElement, TypeElement, ObjectLiteralElement> {
        NodeUnion<ClassLikeDeclaration, ObjectLiteralExpression, TypeLiteralNode, InterfaceDeclaration> parent;
        PropertyName name;
        optional<NodeUnion<FunctionBody>> body;
        /* @internal */ optional<NodeArray<TypeParameterDeclaration>> typeParameters; // Present for use with reporting a grammar error
        /* @internal */ optional<TypeNode> type; // Present for use with reporting a grammar error
    };

#define AccessorDeclaration GetAccessorDeclaration, SetAccessorDeclaration

    using ObjectLiteralElementLike = NodeUnion<PropertyAssignment, ShorthandPropertyAssignment, SpreadAssignment, MethodDeclaration, AccessorDeclaration>;

    template<class T>
    struct ObjectLiteralExpressionBase: PrimaryExpression {
        NodeArray<T> properties;
    };

    struct ObjectLiteralExpression: BrandKind<SyntaxKind::ObjectLiteralExpression, ObjectLiteralExpressionBase<ObjectLiteralElementLike>> {
        /* @internal */ bool multiLine;
    };

    struct BinaryExpression: Expression, BrandKind<types::BinaryExpression> {
        Expression left;
        Node operatorToken; //BinaryOperatorToken uses a lot of different NodeType<T>
        Expression right;
    };

//    using AssignmentOperatorToken = Token<AssignmentOperator>;

    struct AssignmentExpression: BinaryExpression {
        LeftHandSideExpression left;
    };

    struct ObjectDestructuringAssignment: BinaryExpression {
        ObjectLiteralExpression left;
        EqualsToken operatorToken;
    };

//    export type DestructuringAssignment =
//        | ObjectDestructuringAssignment
//        | ArrayDestructuringAssignment
//        ;
//
//    export type BindingOrAssignmentElement =
//        | VariableDeclaration
//        | ParameterDeclaration
//        | ObjectBindingOrAssignmentElement
//        | ArrayBindingOrAssignmentElement
//        ;
//
//    export type ObjectBindingOrAssignmentElement =
//        | BindingElement
//        | PropertyAssignment // AssignmentProperty
//        | ShorthandPropertyAssignment // AssignmentProperty
//        | SpreadAssignment // AssignmentRestProperty
//        ;
//
//    export type ArrayBindingOrAssignmentElement =
//        | BindingElement
//        | OmittedExpression // Elision
//        | SpreadElement // AssignmentRestElement
//        | ArrayLiteralExpression // ArrayAssignmentPattern
//        | ObjectLiteralExpression // ObjectAssignmentPattern
//        | AssignmentExpression<EqualsToken> // AssignmentElement
//        | Identifier // DestructuringAssignmentTarget
//        | PropertyAccessExpression // DestructuringAssignmentTarget
//        | ElementAccessExpression // DestructuringAssignmentTarget
//        ;
//
//    export type BindingOrAssignmentElementRestIndicator =
//        | DotDotDotToken // from BindingElement
//        | SpreadElement // AssignmentRestElement
//        | SpreadAssignment // AssignmentRestProperty
//        ;
//
//    struct BindingOrAssignmentElementTarget: NodeType<
//        BindingOrAssignmentPattern,
//        Identifier,
//        PropertyAccessExpression,
//        ElementAccessExpression,
//        OmittedExpression> {};
//
//    export type ObjectBindingOrAssignmentPattern =
//        | ObjectBindingPattern
//        | ObjectLiteralExpression // ObjectAssignmentPattern
//        ;
//
//    export type ArrayBindingOrAssignmentPattern =
//        | ArrayBindingPattern
//        | ArrayLiteralExpression // ArrayAssignmentPattern
//        ;
//
//    export type AssignmentPattern = ObjectLiteralExpression | ArrayLiteralExpression;
//
//    export type BindingOrAssignmentPattern = ObjectBindingOrAssignmentPattern | ArrayBindingOrAssignmentPattern;

    struct ConditionalExpression: Expression, BrandKind<types::ConditionalExpression> {
        Expression condition;
        QuestionToken questionToken;
        Expression whenTrue;
        ColonToken colonToken;
        Expression whenFalse;
    };

    struct FunctionExpression: PrimaryExpression, FunctionLikeDeclarationBase, BrandKind<types::FunctionExpression> {
        optional<Identifier> name;
        optional<FunctionBody> body; // Required, whereas the member inherited from FunctionDeclaration is optional
    };

//    struct SignatureDeclaration: NodeType<CallSignatureDeclaration, ConstructSignatureDeclaration, MethodSignature, IndexSignatureDeclaration, FunctionTypeNode, ConstructorTypeNode, JSDocFunctionType, FunctionDeclaration, MethodDeclaration, ConstructorDeclaration, AccessorDeclaration, FunctionExpression, ArrowFunction> {};

    struct TypePredicateNode: BrandKind<SyntaxKind::TypePredicate, TypeNode> {
        NodeUnion<CallSignatureDeclaration, ConstructSignatureDeclaration, MethodSignature, IndexSignatureDeclaration, FunctionTypeNode, ConstructorTypeNode, FunctionDeclaration, MethodDeclaration, ConstructorDeclaration, AccessorDeclaration, FunctionExpression, ArrowFunction> parent;
        optional<AssertsKeyword> assertsModifier;
        NodeUnion<Identifier, ThisTypeNode> parameterName;
        optional<TypeNode> type;
    };

    struct ArrayLiteralExpression: BrandKind<SyntaxKind::ArrayLiteralExpression, PrimaryExpression> {
        NodeArray<Expression> elements;
        /* @internal */
        bool multiLine; //optional
    };

    struct ArrayDestructuringAssignment: BinaryExpression {
        ArrayLiteralExpression left;
        EqualsToken operatorToken;
    };

    struct CallExpression: BrandKind<SyntaxKind::CallExpression, LeftHandSideExpression> {
        LeftHandSideExpression expression;
        optional<QuestionDotToken> questionDotToken;
        optional<NodeArray<TypeNode>> typeArguments;
        NodeArray<Expression> arguments;
    };

    struct CallChain: CallExpression {};

/* @internal */
    struct CallChainRoot: CallChain {};

    struct NewExpression: BrandKind<SyntaxKind::NewExpression, PrimaryExpression> {
        LeftHandSideExpression expression;
        optional<NodeArray<TypeNode>> typeArguments;
        optional<NodeArray<Expression>> arguments;
    };

    struct TypeAssertion: BrandKind<SyntaxKind::TypeAssertionExpression, UnaryExpression> {
        TypeNode type;
        UnaryExpression expression;
    };

    struct TemplateExpression;
    struct TemplateLiteralTypeNode;
    struct TemplateSpan;
    struct TemplateLiteralTypeSpan;

    struct TemplateHead: BrandKind<SyntaxKind::TemplateHead, TemplateLiteralLikeNode, Node> {
        NodeUnion<TemplateExpression, TemplateLiteralTypeNode> parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateMiddle: BrandKind<SyntaxKind::TemplateMiddle, TemplateLiteralLikeNode, Node> {
        NodeUnion<TemplateSpan, TemplateLiteralTypeSpan> &parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateTail: BrandKind<SyntaxKind::TemplateTail, TemplateLiteralLikeNode, Node> {
        NodeUnion<TemplateSpan, TemplateLiteralTypeSpan> parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateLiteralTypeSpan: BrandKind<SyntaxKind::TemplateLiteralTypeSpan, TypeNode> {
        TemplateLiteralTypeNode &parent;
        TypeNode type;
        NodeUnion<TemplateMiddle, TemplateTail> literal;
    };

    struct TemplateLiteralTypeNode: BrandKind<SyntaxKind::TemplateLiteralType, TypeNode> {
        TemplateHead head;
        NodeArray<TemplateLiteralTypeSpan> templateSpans;
    };

    struct TemplateExpression: BrandKind<SyntaxKind::TemplateExpression, PrimaryExpression> {
        TemplateHead head;
        NodeArray<TemplateSpan> templateSpans;
    };

#define TemplateLiteral TemplateExpression, NoSubstitutionTemplateLiteral

    struct TaggedTemplateExpression: BrandKind<SyntaxKind::TaggedTemplateExpression, MemberExpression> {
        LeftHandSideExpression tag;
        optional<NodeArray<TypeNode>> typeArguments;
        NodeUnion<TemplateLiteral> templateLiteral;
        /*@internal*/ optional<QuestionDotToken> questionDotToken; // NOTE: Invalid syntax, only used to report a grammar error.
    };

    struct TemplateSpan: BrandKind<SyntaxKind::TemplateSpan, Node> {
        TemplateExpression parent;
        Expression expression;
        NodeUnion<TemplateMiddle, TemplateTail> literal;
    };

    struct AsExpression: BrandKind<SyntaxKind::AsExpression, Expression> {
        Expression expression;
        TypeNode type;
    };

    struct NonNullExpression: BrandKind<SyntaxKind::NonNullExpression, LeftHandSideExpression> {
        Expression expression;
    };

    struct ParenthesizedExpression: BrandKind<SyntaxKind::ParenthesizedExpression, PrimaryExpression> {
        Expression expression;
    };

    struct SpreadElement: BrandKind<SyntaxKind::SpreadElement, Expression> {
        NodeUnion<ArrayLiteralExpression, CallExpression, NewExpression> parent;
        Expression expression;
    };

    struct TypeQueryNode: BrandKind<SyntaxKind::TypeQuery, NodeWithTypeArguments> {
        NodeUnion<EntityName> exprName;
    };

    struct ArrayTypeNode: BrandKind<SyntaxKind::ArrayType, TypeNode> {
        TypeNode elementType;
    };

    struct NamedTupleMember: BrandKind<SyntaxKind::NamedTupleMember, TypeNode> {
        optional<DotDotDotToken> dotDotDotToken;
        Identifier name;
        optional<QuestionToken> questionToken;
        TypeNode type;
    };

    struct OptionalTypeNode: BrandKind<SyntaxKind::OptionalType, TypeNode> {
        TypeNode type;
    };

    struct RestTypeNode: BrandKind<SyntaxKind::RestType, TypeNode> {
        TypeNode type;
    };

    struct UnionTypeNode: BrandKind<SyntaxKind::UnionType, TypeNode> {
        NodeArray<TypeNode> types;
    };

    struct IntersectionTypeNode: BrandKind<SyntaxKind::IntersectionType, TypeNode> {
        NodeArray<TypeNode> types;
    };

#define UnionOrIntersectionTypeNode UnionTypeNode, IntersectionTypeNode

    struct ConditionalTypeNode: BrandKind<SyntaxKind::ConditionalType, TypeNode> {
        TypeNode checkType;
        TypeNode extendsType;
        TypeNode trueType;
        TypeNode falseType;
    };

    struct InferTypeNode: BrandKind<SyntaxKind::InferType, TypeNode> {
        TypeParameterDeclaration typeParameter;
    };

    struct ParenthesizedTypeNode: BrandKind<SyntaxKind::ParenthesizedType, TypeNode> {
        TypeNode type;
    };

    struct TypeOperatorNode: BrandKind<SyntaxKind::TypeOperator, TypeNode> {
        SyntaxKind operatorKind;
        TypeNode type;
    };

/* @internal */
    struct UniqueTypeOperatorNode: TypeOperatorNode {
        SyntaxKind operatorKind = SyntaxKind::UniqueKeyword;
    };

    struct IndexedAccessTypeNode: BrandKind<SyntaxKind::IndexedAccessType, TypeNode> {
        TypeNode objectType;
        TypeNode indexType;
    };

    struct MappedTypeNode: BrandKind<SyntaxKind::MappedType, TypeNode> {
        optional<NodeUnion<ReadonlyKeyword, PlusToken, MinusToken>> readonlyToken;
        TypeParameterDeclaration typeParameter;
        optional<TypeNode> nameType;
        optional<NodeUnion<QuestionToken, PlusToken, MinusToken>> questionToken;
        optional<TypeNode> type;
        /** Used only to produce grammar errors */
        optional<NodeArray<TypeElement>> members;
    };

#define JsxChild JsxText, JsxExpression, JsxElement, JsxSelfClosingElement, JsxFragment
#define JsxAttributeValue StringLiteral, JsxExpression, JsxElement, JsxSelfClosingElement, JsxFragment
#define JsxAttributeLike JsxAttribute, JsxSpreadAttribute
#define JsxTagNameExpression Identifier, ThisExpression, JsxTagNamePropertyAccess
#define JsxOpeningLikeElement JsxSelfClosingElement, JsxOpeningElement

    struct JsxElement;
    struct JsxSelfClosingElement;
    struct JsxOpeningElement;
    struct JsxAttributes;

    struct JsxTagNamePropertyAccess: PropertyAccessExpression {
        NodeUnion<JsxTagNameExpression> expression;
    };

    struct JsxAttribute;
    struct JsxSpreadAttribute;
    struct JsxFragment;

    struct JsxExpression: BrandKind<SyntaxKind::JsxExpression, Expression> {
        NodeUnion<JsxElement, JsxFragment, JsxAttributeLike> parent;
        optional<DotDotDotToken> dotDotDotToken;
        optional<Expression> expression;
    };

    struct JsxAttribute: BrandKind<SyntaxKind::JsxAttribute, ObjectLiteralElement> {
        JsxAttributes &parent;
        Identifier name;
        /// JSX attribute initializers are optional; <X y /> is sugar for <X y={true} />
        optional<NodeUnion<JsxAttributeValue>> initializer;
    };

    struct JsxAttributes: BrandKind<SyntaxKind::JsxAttributes, PrimaryExpression> {
        NodeUnion<JsxOpeningLikeElement> &parent;
        NodeArray<JsxAttributeLike> properties;
    };

    // The opening element of a <Tag>...</Tag> JsxElement
    struct JsxOpeningElement: BrandKind<SyntaxKind::JsxOpeningElement, Expression> {
        JsxElement &parent;
        NodeUnion<JsxTagNameExpression> tagName;
        optional<NodeArray<TypeNode>> typeArguments;
        JsxAttributes attributes;
    };

    struct JsxText: BrandKind<SyntaxKind::JsxText, LiteralLikeNode> {
        NodeUnion<JsxElement, JsxFragment> parent;
        bool containsOnlyTriviaWhiteSpaces;
    };

    struct JsxClosingElement: BrandKind<SyntaxKind::JsxClosingElement, Node> {
        JsxElement &parent;
        NodeUnion<JsxTagNameExpression> tagName;
    };

    /// A JSX expression of the form <TagName attrs>...</TagName>
    struct JsxElement: BrandKind<SyntaxKind::JsxElement, PrimaryExpression> {
        JsxOpeningElement openingElement;
        NodeArray<JsxChild> children;
        JsxClosingElement closingElement;
    };

    struct JsxAttribute;
    struct JsxFragment;
    struct JsxSpreadAttribute;
    struct JsxClosingFragment;

    // A JSX expression of the form <TagName attrs />
    struct JsxSelfClosingElement: BrandKind<SyntaxKind::JsxSelfClosingElement, PrimaryExpression> {
        NodeUnion<JsxTagNameExpression> tagName;
        optional<NodeArray<TypeNode>> typeArguments;
        JsxAttributes attributes;
    };

    struct JsxFragment;
    /// The opening element of a <>...</> JsxFragment
    struct JsxOpeningFragment: BrandKind<SyntaxKind::JsxOpeningFragment, Expression> {
        JsxFragment &parent;
    };

    /// The closing element of a <>...</> JsxFragment
    struct JsxClosingFragment: BrandKind<SyntaxKind::JsxClosingFragment, Expression> {
        JsxFragment &parent;
    };

    /// A JSX expression of the form <>...</>
    struct JsxFragment: BrandKind<SyntaxKind::JsxFragment, PrimaryExpression> {
        JsxOpeningFragment openingFragment;
        NodeArray<JsxChild> children;
        JsxClosingFragment closingFragment;
    };

    struct JsxSpreadAttribute: BrandKind<SyntaxKind::JsxSpreadAttribute, ObjectLiteralElement> {
        JsxAttributes parent;
        Expression expression;
    };

    struct LiteralTypeNode: BrandKind<SyntaxKind::LiteralType, TypeNode> {
        NodeUnion<NullLiteral, BooleanLiteral, LiteralExpression, PrefixUnaryExpression> literal;
    };

    struct TupleTypeNode: BrandKind<SyntaxKind::TupleType, TypeNode> {
        NodeArray<TypeNode, NamedTupleMember> elements;
    };

//    using AccessibilityModifier = NodeType<PublicKeyword, PrivateKeyword, ProtectedKeyword>;
//    using ParameterPropertyModifier = NodeType<AccessibilityModifier, ReadonlyKeyword>;
//    using ClassMemberModifier = NodeType<AccessibilityModifier, ReadonlyKeyword, StaticKeyword>;

    struct Decorator: BrandKind<SyntaxKind::Decorator, Node> {
        NamedDeclaration parent;
        Expression expression;
    };

    struct EndOfFileToken: Token<SyntaxKind::EndOfFileToken> {};

    struct SourceFile: BrandKind<SyntaxKind::SourceFile, Node> {
        string fileName;
        NodeArray<Statement> statements;
        EndOfFileToken endOfFileToken;

        optional<types::ModuleKind> impliedNodeFormat;

        OptionalNode externalModuleIndicator;
    };
}