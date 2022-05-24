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
        int start = -1; //-1 = undefined
        int length = -1; //-1 = undefined
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
        // version that supports both pattern trailers - *but*, BaseNode 16 is the first version that also supports ECMASCript 2022.
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
        PrecedingLineBreak = 1<<0,
        /* @internal */
        PrecedingJSDocComment = 1<<1,
        /* @internal */
        Unterminated = 1<<2,
        /* @internal */
        ExtendedUnicodeEscape = 1<<3,
        Scientific = 1<<4,        // e.g. `10e2`
        Octal = 1<<5,             // e.g. `0777`
        HexSpecifier = 1<<6,      // e.g. `0x00000000`
        BinarySpecifier = 1<<7,   // e.g. `0b0110010000000000`
        OctalSpecifier = 1<<8,    // e.g. `0o777`
        /* @internal */
        ContainsSeparator = 1<<9, // e.g. `0b1100_0101`
        /* @internal */
        UnicodeEscape = 1<<10,
        /* @internal */
        ContainsInvalidEscape = 1<<11,    // e.g. `\uhello`
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
        Let = 1<<0,  // Variable declaration
        Const = 1<<1,  // Variable declaration
        NestedNamespace = 1<<2,  // Namespace declaration
        Synthesized = 1<<3,  // BaseNode was synthesized during transformation
        Namespace = 1<<4,  // Namespace declaration
        OptionalChain = 1<<5,  // Chained MemberExpression rooted to a pseudo-OptionalExpression
        ExportContext = 1<<6,  // Export context (initialized by binding)
        ContainsThis = 1<<7,  // Interface contains references to "this"
        HasImplicitReturn = 1<<8,  // If function implicitly returns on one of codepaths (initialized by binding)
        HasExplicitReturn = 1<<9,  // If function has explicit reachable return on one of codepaths (initialized by binding)
        GlobalAugmentation = 1<<10,  // Set if module declaration is an augmentation for the global scope
        HasAsyncFunctions = 1<<11, // If the file has async functions (initialized by binding)
        DisallowInContext = 1<<12, // If BaseNode was parsed in a context where 'in-expressions' are not allowed
        YieldContext = 1<<13, // If BaseNode was parsed in the 'yield' context created when parsing a generator
        DecoratorContext = 1<<14, // If BaseNode was parsed as part of a decorator
        AwaitContext = 1<<15, // If BaseNode was parsed in the 'await' context created when parsing an async function
        DisallowConditionalTypesContext = 1<<16, // If BaseNode was parsed in a context where conditional types are not allowed
        ThisNodeHasError = 1<<17, // If the parser encountered an error when parsing the code that created this node
        JavaScriptFile = 1<<18, // If BaseNode was parsed in a JavaScript
        ThisNodeOrAnySubNodesHasError = 1<<19, // If this BaseNode or any of its children had an error
        HasAggregatedChildData = 1<<20, // If we've computed data from children and cached it in this node

        // These flags will be set when the parser encounters a dynamic import expression or 'import.meta' to avoid
        // walking the tree if the flags are not set. However, these flags are just a approximation
        // (hence why it's named "PossiblyContainsDynamicImport") because once set, the flags never get cleared.
        // During editing, if a dynamic import is removed, incremental parsing will *NOT* clear this flag.
        // This means that the tree will always be traversed during module resolution, or when looking for external module indicators.
        // However, the removal operation should not occur often and in the case of the
        // removal, it is likely that users will add the import anyway.
        // The advantage of this approach is its simplicity. For the case of batch compilation,
        // we guarantee that users won't have to pay the price of walking the tree if a dynamic import isn't used.
        /* @internal */ PossiblyContainsDynamicImport = 1<<21,
        /* @internal */ PossiblyContainsImportMeta = 1<<22,

        JSDoc = 1<<23, // If BaseNode was parsed inside jsdoc
        /* @internal */ Ambient = 1<<24, // If BaseNode was inside an ambient context -- a declaration file, or inside something with the `declare` modifier.
        /* @internal */ InWithStatement = 1<<25, // If any ancestor of BaseNode was the `statement` of a WithStatement (not the `expression`)
        JsonFile = 1<<26, // If BaseNode was parsed in a Json
        /* @internal */ TypeCached = 1<<27, // If a type was cached for BaseNode at any point
        /* @internal */ Deprecated = 1<<28, // If has '@deprecated' JSDoc tag

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
        Export = 1<<0,  // Declarations
        Ambient = 1<<1,  // Declarations
        Public = 1<<2,  // Property/Method
        Private = 1<<3,  // Property/Method
        Protected = 1<<4,  // Property/Method
        Static = 1<<5,  // Property/Method
        Readonly = 1<<6,  // Property/Method
        Abstract = 1<<7,  // Class/Method/ConstructSignature
        Async = 1<<8,  // Property/Method/Function
        Default = 1<<9,  // Function/Class (export default declaration)
        Const = 1<<11, // Const enum
        HasComputedJSDocModifiers = 1<<12, // Indicates the computed modifier flags include modifiers from JSDoc.

        Deprecated = 1<<13, // Deprecated tag.
        Override = 1<<14, // Override method.
        In = 1<<15, // Contravariance modifier
        Out = 1<<16, // Covariance modifier
        HasComputedFlags = 1<<29, // Modifier flags have been computed

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
        ContainsTypeScript = 1<<0,
        ContainsJsx = 1<<1,
        ContainsESNext = 1<<2,
        ContainsES2022 = 1<<3,
        ContainsES2021 = 1<<4,
        ContainsES2020 = 1<<5,
        ContainsES2019 = 1<<6,
        ContainsES2018 = 1<<7,
        ContainsES2017 = 1<<8,
        ContainsES2016 = 1<<9,
        ContainsES2015 = 1<<10,
        ContainsGenerator = 1<<11,
        ContainsDestructuringAssignment = 1<<12,

        // Markers
        // - Flags used to indicate that a subtree contains a specific transformation.
        ContainsTypeScriptClassSyntax = 1<<12, // Decorators, Property Initializers, Parameter Property Initializers
        ContainsLexicalThis = 1<<13,
        ContainsRestOrSpread = 1<<14,
        ContainsObjectRestOrSpread = 1<<15,
        ContainsComputedPropertyName = 1<<16,
        ContainsBlockScopedBinding = 1<<17,
        ContainsBindingPattern = 1<<18,
        ContainsYield = 1<<19,
        ContainsAwait = 1<<20,
        ContainsHoistedDeclarationOrCompletion = 1<<21,
        ContainsDynamicImport = 1<<22,
        ContainsClassFields = 1<<23,
        ContainsPossibleTopLevelAwait = 1<<24,
        ContainsLexicalSuper = 1<<25,
        ContainsUpdateExpressionForIdentifier = 1<<26,
        // Please leave this as 1 << 29.
        // It is the maximum bit we can set before we outgrow the size of a v8 small integer (SMI) on an x86 system.
        // It is a good reminder of how much room we have left
        HasComputedFlags = 1<<29, // Transform flags have been computed.

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
        TypeExcludes = ~ContainsTypeScript,
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

//    class NodeArray;

    struct Decorator;
    struct Modifier;

    struct BaseNodeStructure;

    template<SyntaxKind Kind>
    struct BrandKind {
        constexpr static auto KIND = Kind;
        SyntaxKind kind = Kind;
    };

    struct Unknown {
        SyntaxKind kind = SyntaxKind::Unknown;
    };

//    template<class T>
//    int extractKind2(){
//    //    auto a = declval<T>();
//        std::cout << static_cast<Unknown>(declval<T>()).KIND << "\n";
//        return false;
////        return T::KIND;
//    }

    struct Node {
        Unknown *data = nullptr;
        shared_ptr<Node> parent;

        Node() {
        }

        Node(Unknown *data) {
            this->data = data;
        }

        ~Node();
        explicit operator bool() const { return data != nullptr; };

        SyntaxKind kind();

        template<class T>
        bool is() {
            return data && data->kind == T::KIND;
        }

        template<typename T>
        T &to() {
            auto valid = data && data->kind == T::KIND;
            if (!valid) throw std::runtime_error("Can not convert Node, invalid kind or no data set");
            return *reinterpret_cast<T *>(data);
        }

        BaseNodeStructure & toBase() {
            if (!data) throw std::runtime_error("Can not convert Node, no data set");
            return *reinterpret_cast<BaseNodeStructure *>(data);
        }

        template<typename T>
        T &toBase() {
            if (!data) throw std::runtime_error("Can not convert Node, no data set");
            return *reinterpret_cast<T *>(data);
        }

//        template<typename T>
//        NodeType &toUnion() {
//            T i;
//            if (!data) throw std::runtime_error("Can not convert Node, no data set");
//
//            auto types = i.types();
//
//            for (auto kind: i.kinds()) {
//                if (data->kind == kind) {
//                    auto t = std::get<0>(types);
//                    cout << kind << " FOUND " << t.kind << "\n";
////                    return *dynamic_cast<t *>(data);
//                }
//            }
//
//            throw std::runtime_error("Can not convert Node, no valid kind");
//        }
    };

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
    struct NodeArray: BaseNodeArray {
    };
//
//    template<typename ... T>
//    struct NodeType: public Node {
//        using ETypes = std::tuple<decltype(T{})...>;
//        ETypes types;
//
//        vector<int> kinds() {
//
//        }
//    };

    /**
     * note: its important to not embed Ts in statically way to not initialize their constructor when Node is crreated.
     * Otherwise this would mean an insane amount of memory allocation.
     */
    template<typename ... Ts>
    struct NodeType: public Node {
        auto types() {
            using ETypes = std::tuple<decltype(Ts{})...>;
            ETypes types;
            return types;
        }

        vector<SyntaxKind> kinds() {
            using ETypes = std::tuple<decltype(Ts{})...>;
            ETypes types;
            return getKinds(types);
        }

        bool contains(types::SyntaxKind kind) {
            return has(kinds(), kind);
        }
    };

    /**
     * All BaseNode pointers are owned by SourceFile. If SourceFile destroys, all its Nodes are destroyed as well.
     *
     * There are a big variety of sub types: All have in common that they are the owner of their data (except *parent).
     */
    struct BaseNodeStructureWithoutDecorators: ReadonlyTextRange {
        /* types::NodeFlags */ int flags;
        /* @internal */ /* types::ModifierFlags */ int modifierFlagsCache;
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        /* @internal */ /* types::TransformFlags */ int transformFlags; // Flags for transforms
////        /* @internal */ id?: NodeId;                          // Unique id (used to look up NodeLinks)
        NodeType<Unknown> parent;                                 // Parent BaseNode (initialized by binding)
//        /* @internal */ original?: Node;                      // The original BaseNode if this is an updated node.
//        /* @internal */ symbol: Symbol;                       // Symbol declared by BaseNode (initialized by binding)
//        /* @internal */ locals?: SymbolTable;                 // Locals associated with BaseNode (initialized by binding)
//        /* @internal */ nextContainer?: Node;                 // Next container in declaration order (initialized by binding)
//        /* @internal */ localSymbol?: Symbol;                 // Local symbol declared by BaseNode (initialized by binding only for exported nodes)
//        /* @internal */ flowNode?: FlowNode;                  // Associated FlowNode (initialized by binding)
//        /* @internal */ emitNode?: EmitNode;                  // Associated EmitNode (initialized by transforms)
//        /* @internal */ contextualType?: Type;                // Used to temporarily assign a contextual type during overload resolution
//        /* @internal */ inferenceContext?: InferenceContext;  // Inference context for contextual type
    };

    struct BaseNodeStructure: BaseNodeStructureWithoutDecorators {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
    };

    template<typename ...T>
    struct ParentAccess {
        NodeType<T...> parent;
    };

    struct DeclarationName;

    struct Declaration: BaseNodeStructure {};

    struct NamedDeclaration: Declaration {
        optional<NodeType<DeclarationName>> name;
    };

    struct Expression: BaseNodeStructure {};

    struct UnaryExpression: Expression {};

    struct UpdateExpression: UnaryExpression {};

    struct LeftHandSideExpression: UpdateExpression {};

    struct MemberExpression: LeftHandSideExpression {};

    struct PrimaryExpression: MemberExpression {};

    struct PrivateIdentifier: PrimaryExpression, BrandKind<SyntaxKind::PrivateIdentifier> {};

    template<SyntaxKind T>
    struct Token: BaseNodeArray, BrandKind<T> {};

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

    struct NullLiteral: PrimaryExpression, BrandKind<SyntaxKind::NullKeyword> {};

    struct TrueLiteral: PrimaryExpression, BrandKind<SyntaxKind::TrueKeyword> {};

    struct FalseLiteral: PrimaryExpression, BrandKind<SyntaxKind::FalseKeyword> {};

    struct BooleanLiteral: NodeType<TrueLiteral, FalseLiteral> {};

    struct ThisExpression: PrimaryExpression, BrandKind<SyntaxKind::ThisKeyword> {};

    struct SuperExpression: PrimaryExpression, BrandKind<SyntaxKind::SuperKeyword> {};

    struct ImportExpression: PrimaryExpression, BrandKind<SyntaxKind::ImportKeyword> {};

    using PostfixUnaryOperator = SyntaxKind; //SyntaxKind.PlusPlusToken | SyntaxKind.MinusMinusToken

    struct PrefixUnaryExpression: UpdateExpression, BrandKind<SyntaxKind::PrefixUnaryExpression> {
        NodeType<LeftHandSideExpression> operand;
        PostfixUnaryOperator operatorKind;
    };

    struct PartiallyEmittedExpression: LeftHandSideExpression, BrandKind<SyntaxKind::PartiallyEmittedExpression> {
        NodeType <Expression> expression;
    };

    struct PostfixUnaryExpression: UpdateExpression, BrandKind<SyntaxKind::PostfixUnaryExpression> {
        NodeType<LeftHandSideExpression> operand;
        PostfixUnaryOperator operatorKind;
    };

    struct DeleteExpression: UnaryExpression, BrandKind<SyntaxKind::DeleteExpression> {
        NodeType<UnaryExpression> expression;
    };

    struct TypeOfExpression: UnaryExpression, BrandKind<SyntaxKind::TypeOfExpression> {
        NodeType<UnaryExpression> expression;
    };

    struct VoidExpression: UnaryExpression, BrandKind<SyntaxKind::VoidExpression> {
        NodeType<UnaryExpression> expression;
    };

    struct AwaitExpression: UnaryExpression, BrandKind<SyntaxKind::AwaitExpression> {
        NodeType<UnaryExpression> expression;
    };

    struct YieldExpression: Expression, BrandKind<SyntaxKind::YieldExpression> {
        optional<NodeType<AsteriskToken>> asteriskToken;
        optional<NodeType<Expression>> expression;
    };

    //this seems to be related to instantiated types
    struct Type {
    };

    struct ParameterDeclaration;
    struct NamedTupleMember;

    struct SyntheticExpression: Expression, BrandKind<SyntaxKind::SyntheticExpression> {
        bool isSpread;
        Type type;
        optional<NodeType<ParameterDeclaration, NamedTupleMember>> tupleNameSource;
    };

    struct TypeNode: BaseNodeStructure {};

    /** @deprecated Use `AwaitKeyword` instead. */
    using AwaitKeywordToken = AwaitKeyword;

    /** @deprecated Use `AssertsKeyword` instead. */
    using AssertsToken = AssertsKeyword;

    /** @deprecated Use `ReadonlyKeyword` instead. */
    using ReadonlyToken = ReadonlyKeyword;

    struct Modifier: NodeType<
            AbstractKeyword, AsyncKeyword, ConstKeyword, DeclareKeyword, DefaultKeyword, ExportKeyword, InKeyword, PrivateKeyword, ProtectedKeyword, PublicKeyword, OutKeyword, OverrideKeyword, ReadonlyKeyword, StaticKeyword> {
    };

    struct ModifiersArray: NodeArray<Modifier> {};

    struct LiteralLikeNode: BaseNodeStructure {
        std::string text;
        bool isUnterminated; //optional
        bool hasExtendedUnicodeEscape; //optional
    };

    struct LiteralExpression: LiteralLikeNode, PrimaryExpression {};

    struct StringLiteral: LiteralExpression, Declaration, BrandKind<SyntaxKind::StringLiteral> {};

    struct Identifier: PrimaryExpression, Declaration, BrandKind<SyntaxKind::Identifier> {
        /**
         * Prefer to use `id.unescapedText`. (Note: This is available only in services, not internally to the TypeScript compiler.)
         * Text of identifier, but if the identifier begins with two underscores, this will begin with three.
         */
        string escapedText;
        SyntaxKind originalKeywordKind = SyntaxKind::Unknown;
    };

    struct EntityName;

    struct Statement: BaseNodeStructure {
    };

    enum class FlowFlags {
        Unreachable = 1<<0,  // Unreachable code
        Start = 1<<1,  // Start of flow graph
        BranchLabel = 1<<2,  // Non-looping junction
        LoopLabel = 1<<3,  // Looping junction
        Assignment = 1<<4,  // Assignment
        TrueCondition = 1<<5,  // Condition known to be true
        FalseCondition = 1<<6,  // Condition known to be false
        SwitchClause = 1<<7,  // Switch statement clause
        ArrayMutation = 1<<8,  // Potential array mutation
        Call = 1<<9,  // Potential assertion call
        ReduceLabel = 1<<10, // Temporarily reduce antecedents of label
        Referenced = 1<<11, // Referenced as antecedent once
        Shared = 1<<12, // Referenced as antecedent more than once

        Label = BranchLabel | LoopLabel,
        Condition = TrueCondition | FalseCondition,
    };

    struct Block: Statement, BrandKind<SyntaxKind::Block> {
        NodeArray<Statement> statements;
        /*@internal*/ bool multiLine;
    };

    struct TemplateLiteralLikeNode: LiteralLikeNode {
        optional<string> rawText;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct NoSubstitutionTemplateLiteral: LiteralExpression, TemplateLiteralLikeNode, Declaration, BrandKind<SyntaxKind::NoSubstitutionTemplateLiteral> {
        optional<types::TokenFlags> templateFlags;
    };

    struct NumericLiteral: LiteralExpression, ParentAccess<Declaration>, BrandKind<SyntaxKind::NumericLiteral> {
        types::TokenFlags numericLiteralFlags;
    };

    struct ComputedPropertyName: ParentAccess<Declaration>, BrandKind<SyntaxKind::ComputedPropertyName> {
        NodeType<Expression> expression;
    };

    struct QualifiedName: BaseNodeStructure, BrandKind<SyntaxKind::QualifiedName> {
        NodeType<EntityName> left;
        NodeType<NodeType<Identifier>> right;
        /*@internal*/ optional<int> jsdocDotPos; // QualifiedName occurs in JSDoc-style generic: Id1.Id2.<T>
    };

    struct ElementAccessExpression: MemberExpression, BrandKind<SyntaxKind::ElementAccessExpression> {
        NodeType<LeftHandSideExpression> expression;
        optional<NodeType<QuestionDotToken>> questionDotToken;
        NodeType<Expression> argumentExpression;
    };

    struct OmittedExpression: BaseNodeStructure, BrandKind<SyntaxKind::OmittedExpression> {};

    struct VariableDeclaration;
    struct ParameterDeclaration;
    struct BindingName;
    struct PropertyName;
    struct BindingElement;

    struct ArrayBindingElement: NodeType<BindingElement, OmittedExpression> {};

    struct ObjectBindingPattern: BaseNodeStructure, BrandKind<SyntaxKind::ObjectBindingPattern> {
        NodeArray<BindingElement> elements;
        NodeType<VariableDeclaration, ParameterDeclaration, BindingElement> parent;
    };

    struct ArrayBindingPattern: BaseNodeStructure, BrandKind<SyntaxKind::ArrayBindingPattern> {
        NodeType<VariableDeclaration, ParameterDeclaration, BindingElement> parent;
        NodeArray<ArrayBindingElement> elements;
    };

    struct BindingPattern: NodeType<ObjectBindingPattern, ArrayBindingPattern> {};

    struct BindingElement: NamedDeclaration, ParentAccess<BindingPattern>, BrandKind<SyntaxKind::BindingElement> {
        optional<NodeType<PropertyName>> propertyName;        // Binding property name (in object binding pattern)
        optional<NodeType<DotDotDotToken>> dotDotDotToken;    // Present on rest element (in object binding pattern)
        NodeType<BindingName> name;                  // Declared binding element name
        optional<NodeType<Expression>> initializer;           // Optional initializer
    };

    struct VariableDeclarationList;

    struct VariableStatement: Statement, BrandKind<SyntaxKind::VariableStatement> {
        /* @internal*/ optional<NodeArray<Decorator>> decorators; // Present for use with reporting a grammar error
        NodeType<VariableDeclarationList> declarationList;
    };

    struct ExpressionStatement: Statement, BrandKind<SyntaxKind::ExpressionStatement> {
        NodeType<Expression> expression;
    };

    struct PrologueDirective: ExpressionStatement {
        NodeType<StringLiteral> expression;
    };

    struct IfStatement: Statement, BrandKind<SyntaxKind::IfStatement> {
        NodeType<Expression> expression;
        NodeType<Statement> thenStatement;
        optional<NodeType<Statement>> elseStatement;
    };

//    export type ForInitializer =
//        | VariableDeclarationList
//        | Expression
//        ;

//    export type ForInOrOfStatement =
//        | ForInStatement
//        | ForOfStatement
//        ;


    struct BreakStatement: Statement, BrandKind<SyntaxKind::BreakStatement> {
        optional<NodeType<Identifier>> label;
    };

    struct ContinueStatement: Statement, BrandKind<SyntaxKind::ContinueStatement> {
        optional<NodeType<Identifier>> label;
    };

//    export type BreakOrContinueStatement =
//        | BreakStatement
//        | ContinueStatement
//        ;

    struct ReturnStatement: Statement, BrandKind<SyntaxKind::ReturnStatement> {
        optional<NodeType<Expression>> expression;
    };

    struct WithStatement: Statement, BrandKind<SyntaxKind::WithStatement> {
        NodeType<Expression> expression;
        NodeType<Statement> statement;
    };

    struct CaseBlock;

    struct SwitchStatement: Statement, BrandKind<SyntaxKind::SwitchStatement> {
        NodeType<Expression> expression;
        NodeType<CaseBlock> caseBlock;
        bool possiblyExhaustive; // initialized by binding
    };

    struct CaseClause;
    struct DefaultClause;

    struct CaseBlock: BaseNodeStructure, BrandKind<SyntaxKind::CaseBlock> {
        NodeType<SwitchStatement> parent;
        NodeArray<CaseClause, DefaultClause> clauses;
    };

    struct CaseClause: BaseNodeStructure, BrandKind<SyntaxKind::CaseClause> {
        NodeType<CaseBlock> parent;
        NodeType<Expression> expression;
        NodeArray<Statement> statements;
    };

    struct DefaultClause: BaseNodeStructure, BrandKind<SyntaxKind::DefaultClause> {
        NodeType<CaseBlock> parent;
        NodeArray<Statement> statements;
    };

//    export type CaseOrDefaultClause =
//        | CaseClause
//        | DefaultClause
//        ;

    struct LabeledStatement: Statement, BrandKind<SyntaxKind::LabeledStatement> {
        NodeType<Identifier> label;
        NodeType<Statement> statement;
    };

    struct ThrowStatement: Statement, BrandKind<SyntaxKind::ThrowStatement> {
        NodeType<Expression> expression;
    };

    struct IterationStatement: Statement {
        NodeType<Statement> statement;
    };

    struct DoStatement: IterationStatement, BrandKind<SyntaxKind::DoStatement> {
        NodeType<Expression> expression;
    };

    struct WhileStatement: IterationStatement, BrandKind<SyntaxKind::WhileStatement> {
        NodeType<Expression> expression;
    };

    struct ForInitializer: NodeType<VariableDeclarationList, Expression> {};

    struct ForStatement: IterationStatement, BrandKind<SyntaxKind::ForStatement> {
        optional<NodeType<ForInitializer>> initializer;
        optional<NodeType<Expression>> condition;
        optional<NodeType<Expression>> incrementor;
    };

    struct ForOfStatement: IterationStatement, BrandKind<SyntaxKind::ForOfStatement> {
        NodeType<AwaitKeyword> awaitModifier;
        NodeType<ForInitializer> initializer;
        NodeType<Expression> expression;
    };

    struct ForInStatement: IterationStatement, BrandKind<SyntaxKind::ForInStatement> {
        NodeType<ForInitializer> initializer;
        NodeType<Expression> expression;
    };

    struct VariableDeclarationList: BaseNodeStructure, BrandKind<SyntaxKind::VariableDeclarationList> {
        NodeType<VariableStatement, ForStatement, ForOfStatement, ForInStatement> parent;
        NodeArray<VariableDeclaration> declarations;
    };

    struct CatchClause;

    struct TryStatement: Statement, BrandKind<SyntaxKind::TryStatement> {
        NodeType<Block> tryBlock;
        optional<NodeType<CatchClause>> catchClause;
        optional<NodeType<Block>> finallyBlock;
    };

    struct CatchClause: BaseNodeStructure, BrandKind<types::CatchClause> {
        NodeType<TryStatement> parent;
        optional<NodeType<VariableDeclaration>> variableDeclaration;
        NodeType<Block> block;
    };

    struct VariableDeclaration: NamedDeclaration, BrandKind<SyntaxKind::VariableDeclaration> {
        NodeType<BindingName> name;                    // Declared variable name
        NodeType<VariableDeclarationList, CatchClause> parent;
        optional<NodeType<ExclamationToken>> exclamationToken;  // Optional definite assignment assertion
        optional<NodeType<TypeNode>> type;                      // Optional type annotation
        optional<NodeType<Expression>> initializer;             // Optional initializer
    };

    struct MemberName: NodeType<Identifier, PrivateIdentifier> {};

    struct PropertyAccessExpression: MemberExpression, NamedDeclaration, BrandKind<SyntaxKind::PropertyAccessExpression> {
        NodeType<LeftHandSideExpression> expression;
        optional<NodeType<QuestionDotToken>> questionDotToken;
        NodeType<MemberName> name;
    };

    struct PropertyAccessEntityNameExpression;

    struct EntityNameExpression: NodeType<Identifier, PropertyAccessEntityNameExpression> {};

    struct PropertyAccessEntityNameExpression: PropertyAccessExpression {
        NodeType<EntityNameExpression> expression;
        NodeType<Identifier> name;
    };

    struct PropertyName: NodeType<Identifier, StringLiteral, NumericLiteral, ComputedPropertyName, PrivateIdentifier> {};

    struct StringLiteralLike: NodeType<StringLiteral, NoSubstitutionTemplateLiteral> {};
    struct DeclarationName: NodeType<Identifier, PrivateIdentifier, StringLiteralLike, NumericLiteral, ComputedPropertyName, ElementAccessExpression, BindingPattern, EntityNameExpression> {};
    struct EntityName: NodeType<Identifier, QualifiedName> {};

    struct MetaProperty: PrimaryExpression, BrandKind<SyntaxKind::MetaProperty> {
        SyntaxKind keywordToken = SyntaxKind::NewKeyword; //: SyntaxKind.NewKeyword | SyntaxKind.ImportKeyword;
        NodeType<Identifier> name;
    };

    struct ObjectLiteralElement: NamedDeclaration {
        optional<NodeType<PropertyName>> name;
    };

    template<class T>
    struct ObjectLiteralExpressionBase: PrimaryExpression, Declaration {
        NodeArray<T> properties;
    };

    struct PropertyAssignment;
    struct ShorthandPropertyAssignment;
    struct SpreadAssignment;
    struct MethodDeclaration;
    struct AccessorDeclaration;

    using ObjectLiteralElementLike = NodeType<PropertyAssignment, ShorthandPropertyAssignment, SpreadAssignment, MethodDeclaration, AccessorDeclaration>;

    struct ObjectLiteralExpression: ObjectLiteralExpressionBase<ObjectLiteralElementLike>, BrandKind<SyntaxKind::ObjectLiteralExpression> {
        /* @internal */ bool multiLine;
    };

    struct ShorthandPropertyAssignment: ObjectLiteralElement, ParentAccess<ObjectLiteralExpression>, BrandKind<SyntaxKind::ShorthandPropertyAssignment> {
        NodeType<Identifier> name;
        optional<NodeType<QuestionToken>> questionToken;
        optional<NodeType<ExclamationToken>> exclamationToken;

        // used when ObjectLiteralExpression is used in ObjectAssignmentPattern
        // it is a grammar error to appear in actual object initializer:
        optional<NodeType<EqualsToken>> equalsToken;
        optional<NodeType<Expression>> objectAssignmentInitializer;
    };

//    struct VariableDeclaration: NamedDeclaration, BrandKind<SyntaxKind::VariableDeclaration> {
//        BindingNameNode name;                    // Declared variable name
////            readonly kind: SyntaxKind.VariableDeclaration;
////            readonly parent: VariableDeclarationList | CatchClause;
//        optional <NodeType<ExclamationToken>> exclamationToken;  // Optional definite assignment assertion
//        optional <TypeNode> type; // Optional type annotation
//        optional <NodeType<NodeType<Expression>>> initializer; // Optional initializer
//    };

    struct TypeParameterDeclaration: NamedDeclaration, BrandKind<SyntaxKind::TypeParameter> {
        inline static auto kind = SyntaxKind::TypeParameter;
//        BaseNode *parent; //: DeclarationWithTypeParameterChildren | InferTypeNode;
        NodeType<Identifier> name;
        /** Note: Consider calling `getEffectiveConstraintOfTypeParameter` */
        NodeType<NodeType<TypeNode>> constraint;
        NodeType<NodeType<TypeNode>> defaultType;

        // For error recovery purposes.
        optional<NodeType<Expression>> expression;
    };

    struct ParameterDeclaration: NamedDeclaration, BrandKind<SyntaxKind::Parameter> {
        optional<NodeType<DotDotDotToken>> dotDotDotToken;
        NodeType<BindingName> name;
        optional<NodeType<QuestionToken>> questionToken;
        optional<NodeType<TypeNode>> type;
        optional<NodeType<Expression>> initializer;
    };

    struct ClassElement: NamedDeclaration {
        optional<NodeType<PropertyName>> name;
    };

    struct PropertyDeclaration: ClassElement, BrandKind<SyntaxKind::PropertyDeclaration> {
        optional<NodeType<DotDotDotToken>> dotDotDotToken;
        NodeType<BindingName> name;
        optional<NodeType<QuestionToken>> questionToken;
        optional<NodeType<ExclamationToken>> exclamationToken;
        optional<NodeType<TypeNode>> type;
        optional<NodeType<Expression>> initializer;
    };

    struct SignatureDeclarationBase: NamedDeclaration {
        optional<NodeType<PropertyName>> name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        NodeArray<ParameterDeclaration> parameters;
        optional<NodeType<TypeNode>> type;
        optional<NodeArray<TypeNode>> typeArguments;
    };

    struct PropertyAssignment: ObjectLiteralElement, BrandKind<SyntaxKind::PropertyAssignment> {
        NodeType<PropertyName> name;
        optional<NodeType<QuestionToken>> questionToken;
        optional<NodeType<ExclamationToken>> exclamationToken;
        optional<NodeType<NodeType<Expression>>> initializer;
    };

    struct FunctionLikeDeclarationBase: SignatureDeclarationBase {
        optional<NodeType<AsteriskToken>> asteriskToken;
        optional<NodeType<QuestionToken>> questionToken;
        optional<NodeType<ExclamationToken>> exclamationToken;
        optional<NodeType<Block, Expression>> body;

//        /* @internal */ optional<NodeType<FlowNode>> endFlowNode;
//        /* @internal */ optional<NodeType<FlowNode>> returnFlowNode;
    };

    struct FunctionBody: NodeType<FunctionBody> {};
    struct ConciseBody: NodeType<FunctionBody, Expression> {};

    struct ArrowFunction: Expression, FunctionLikeDeclarationBase, BrandKind<SyntaxKind::ArrowFunction> {
        NodeType<EqualsGreaterThanToken> equalsGreaterThanToken;
        NodeType<ConciseBody> body;
    };

    struct HeritageClause;

    struct ClassLikeDeclarationBase: NamedDeclaration {
        optional<NodeType<Identifier>> name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        optional<NodeArray<HeritageClause>> heritageClauses;
        NodeArray<ClassElement> members;
    };

    struct DeclarationStatement: NamedDeclaration, Statement {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        optional<NodeType<Identifier, StringLiteral, NumericLiteral>> name;
    };

    struct EmptyStatement: Statement, BrandKind<SyntaxKind::EmptyStatement> {
    };

    struct DebuggerStatement: Statement, BrandKind<SyntaxKind::DebuggerStatement> {
    };

    struct CommaListExpression: Expression, BrandKind<SyntaxKind::CommaListExpression> {
        NodeArray<Expression> elements;
    };

    struct MissingDeclaration: DeclarationStatement, BrandKind<SyntaxKind::MissingDeclaration> {
        optional<NodeType<Identifier>> name;
    };

    struct ClassDeclaration: ClassLikeDeclarationBase, DeclarationStatement, BrandKind<SyntaxKind::ClassDeclaration> {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        optional<NodeType<Identifier>> name;
    };

    struct ClassExpression: ClassLikeDeclarationBase, PrimaryExpression, BrandKind<SyntaxKind::ClassExpression> {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
    };

    struct ClassLikeDeclaration: NodeType<ClassDeclaration, ClassExpression> {};

    struct TypeElement: NamedDeclaration {
        optional<NodeType<PropertyName>> name;
        optional<NodeType<QuestionToken>> questionToken;
    };

    struct HeritageClause;

    struct InterfaceDeclaration: DeclarationStatement, BrandKind<SyntaxKind::InterfaceDeclaration> {
        NodeType<Identifier> name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        optional<NodeArray<HeritageClause>> heritageClauses;
        NodeArray<TypeElement> members;
    };

    struct NodeWithTypeArguments: TypeNode {
        optional<NodeArray<TypeNode>> typeArguments;
    };

    struct ExpressionWithTypeArguments: MemberExpression, NodeWithTypeArguments, BrandKind<SyntaxKind::ExpressionWithTypeArguments> {
        NodeType<LeftHandSideExpression> expression;
    };

    struct HeritageClause: BaseNodeStructure, BrandKind<SyntaxKind::HeritageClause> {
        NodeType<InterfaceDeclaration, ClassLikeDeclaration> parent;
        SyntaxKind token; //SyntaxKind.ExtendsKeyword | SyntaxKind.ImplementsKeyword
        NodeArray<ExpressionWithTypeArguments> types;
    };

    struct TypeAliasDeclaration: DeclarationStatement, BrandKind<SyntaxKind::TypeAliasDeclaration> {
        NodeType<Identifier> name;
        optional<NodeArray<TypeParameterDeclaration>> typeParameters;
        NodeType<TypeNode> type;
    };

    struct EnumDeclaration;

    struct EnumMember: NamedDeclaration, BrandKind<SyntaxKind::EnumMember> {
        NodeType<EnumDeclaration> parent;
        // This does include ComputedPropertyName, but the parser will give an error
        // if it parses a ComputedPropertyName in an EnumMember
        PropertyName name;
        optional<NodeType<Expression>> initializer;
    };

    struct EnumDeclaration: DeclarationStatement, BrandKind<SyntaxKind::EnumDeclaration> {
        NodeType<Identifier> name;
        NodeArray<EnumMember> members;
    };

    struct ClassStaticBlockDeclaration: ClassElement, BrandKind<SyntaxKind::ClassStaticBlockDeclaration> {
        NodeType<ClassDeclaration, ClassExpression> parent;
        NodeType<Block> body;
//        /* @internal */ endFlowNode?: FlowNode;
//        /* @internal */ returnFlowNode?: FlowNode;
    };

    struct PropertySignature: TypeElement, BrandKind<SyntaxKind::PropertySignature> {
        NodeType<PropertyName> name;
        optional<NodeType<TypeNode>> type;
        optional<NodeType<NodeType<Expression>>> initializer;
    };

    struct TypeReferenceNode: NodeWithTypeArguments, BrandKind<SyntaxKind::TypeReference> {
        EntityName typeName;
    };

    struct ModuleDeclaration;
    struct ModuleBlock;
    struct ModuleBody;

    struct NamespaceDeclaration: DeclarationStatement, BrandKind<SyntaxKind::ModuleDeclaration> {
        NodeType<ModuleBody, SourceFile> parent;
        NodeType<Identifier> name;
        optional<NodeType<ModuleBody>> body;
    };

    struct NamespaceBody: NodeType<ModuleBlock, NamespaceDeclaration> {};
    struct ModuleName: NodeType<Identifier, StringLiteral> {};
    struct ModuleBody: NodeType<NamespaceBody> {};

    struct ModuleDeclaration: DeclarationStatement, BrandKind<SyntaxKind::ModuleDeclaration> {
        NodeType<ModuleBody, SourceFile> parent;
        NodeType<ModuleName> name;
        optional<NodeType<ModuleBody>> body;
    };

    struct ModuleBlock: Statement, BrandKind<SyntaxKind::ModuleBlock> {
        NodeType<ModuleDeclaration> parent;
        NodeArray<Statement> statements;
    };

    struct ImportEqualsDeclaration;
    struct ExternalModuleReference;

    struct ModuleReference: NodeType<EntityName, ExternalModuleReference> {};

    /**
     * One of:
     * - import x = require("mod");
     * - import x = M.x;
     */
    struct ImportEqualsDeclaration: DeclarationStatement, BrandKind<SyntaxKind::ImportEqualsDeclaration> {
        NodeType<SourceFile, ModuleBlock> parent;
        NodeType<Identifier> name;
        bool isTypeOnly;

        // 'EntityName' for an internal module reference, 'ExternalModuleReference' for an external
        // module reference.
        NodeType<ModuleReference> moduleReference;
    };

    struct ExternalModuleReference: BaseNodeStructure, BrandKind<SyntaxKind::ImportEqualsDeclaration> {
        NodeType<ImportEqualsDeclaration> parent;
        NodeType<Expression> expression;
    };

    struct ImportClause;
    struct AssertClause;
    struct NamespaceImport;
    struct NamedImports;
    struct NamespaceExport;
    struct NamedExports;
    struct ExportDeclaration;

    // In case of:
    // import "mod"  => importClause = undefined, moduleSpecifier = "mod"
    // In rest of the cases, module specifier is string literal corresponding to module
    // ImportClause information is shown at its declaration below.
    struct ImportDeclaration: Statement, BrandKind<SyntaxKind::ImportDeclaration> {
        NodeType<SourceFile, ModuleBlock> parent;
        optional<NodeType<ImportClause>> importClause;
        /** If this is not a StringLiteral it will be a grammar error. */
        NodeType<Expression> moduleSpecifier;
        optional<NodeType<AssertClause>> assertClause;
    };

    struct NamedImportBindings: NodeType<NamespaceImport, NamedImports> {};
    struct NamedExportBindings: NodeType<NamespaceExport, NamedExports> {};

    // In case of:
    // import d from "mod" => name = d, namedBinding = undefined
    // import * as ns from "mod" => name = undefined, namedBinding: NamespaceImport = { name: ns }
    // import d, * as ns from "mod" => name = d, namedBinding: NamespaceImport = { name: ns }
    // import { a, b as x } from "mod" => name = undefined, namedBinding: NamedImports = { elements: [{ name: a }, { name: x, propertyName: b}]}
    // import d, { a, b as x } from "mod" => name = d, namedBinding: NamedImports = { elements: [{ name: a }, { name: x, propertyName: b}]}
    struct ImportClause: NamedDeclaration, BrandKind<SyntaxKind::ImportClause> {
        NodeType<ImportDeclaration> parent;
        bool isTypeOnly;
        optional<NodeType<Identifier>> name; // Default binding
        optional<NodeType<NamedImportBindings>> namedBindings;
    };

    struct AssertionKey: NodeType<Identifier, StringLiteral> {};

    struct AssertEntry: BaseNodeStructure, BrandKind<SyntaxKind::AssertEntry> {
        NodeType<AssertClause> parent;
        NodeType<AssertionKey> name;
        NodeType<Expression> value;
    };

    struct AssertClause: BaseNodeStructure, BrandKind<SyntaxKind::AssertClause> {
        NodeType<ImportDeclaration, ExportDeclaration> parent;
        NodeArray<AssertEntry> elements;
        bool multiLine;
    };

    struct NamespaceImport: NamedDeclaration, BrandKind<SyntaxKind::NamespaceImport> {
        NodeType<ImportClause> parent;
        NodeType<Identifier> name;
    };

    struct NamespaceExport: NamedDeclaration, BrandKind<SyntaxKind::NamespaceExport> {
        NodeType<ExportDeclaration> parent;
        NodeType<Identifier> name;
    };

    struct NamespaceExportDeclaration: DeclarationStatement, BrandKind<SyntaxKind::NamespaceExportDeclaration> {
        NodeType<Identifier> name;
        /* @internal */ optional<NodeArray<Decorator>> decorators; // Present for use with reporting a grammar error
        /* @internal */ optional<ModifiersArray> modifiers; // Present for use with reporting a grammar error
    };

    struct ExportDeclaration: DeclarationStatement, BrandKind<SyntaxKind::ExportDeclaration> {
        NodeType<SourceFile, ModuleBlock> parent;
        bool isTypeOnly;
        /** Will not be assigned in the case of `export * from "foo";` */
        optional<NodeType<NamedExportBindings>> exportClause;
        /** If this is not a StringLiteral it will be a grammar error. */
        optional<NodeType<Expression>> moduleSpecifier;
        optional<NodeType<AssertClause>> assertClause;
    };

    struct ImportSpecifier;
    struct ExportSpecifier;

    struct NamedImports: BaseNodeStructure, BrandKind<SyntaxKind::NamedImports> {
        NodeType<ImportClause> parent;
        NodeArray<ImportSpecifier> elements;
    };

    struct NamedExports: BaseNodeStructure, BrandKind<SyntaxKind::NamedExports> {
        NodeType<ExportDeclaration> parent;
        NodeArray<ExportSpecifier> elements;
    };

    struct NamedImportsOrExports: NodeType<NamedImports, NamedExports> {};

    struct ImportSpecifier: NamedDeclaration, BrandKind<SyntaxKind::ImportSpecifier> {
        NodeType<NamedImports> parent;
        optional<NodeType<Identifier>> propertyName;  // Name preceding "as" keyword (or undefined when "as" is absent)
        NodeType<Identifier> name;           // Declared name
        bool isTypeOnly;
    };

    struct ExportSpecifier: NamedDeclaration, BrandKind<SyntaxKind::ExportSpecifier> {
        NodeType<NamedExports> parent;
        bool isTypeOnly;
        optional<NodeType<Identifier>> propertyName;  // Name preceding "as" keyword (or undefined when "as" is absent)
        NodeType<Identifier> name;           // Declared name
    };

    struct ImportOrExportSpecifier: NodeType<ImportSpecifier, ExportSpecifier> {};

    struct TypeOnlyCompatibleAliasDeclaration: NodeType<ImportClause, ImportEqualsDeclaration, NamespaceImport, ImportOrExportSpecifier> {};

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
    struct ExportAssignment: DeclarationStatement, BrandKind<SyntaxKind::ExportAssignment> {
        NodeType<SourceFile> parent;
        bool isExportEquals;
        NodeType<Expression> expression;
    };

    struct ImportTypeNode;

    struct ImportTypeAssertionContainer: BaseNodeStructure, BrandKind<SyntaxKind::ImportTypeAssertionContainer> {
        NodeType<ImportTypeNode> parent;
        NodeType<AssertClause> assertClause;
        bool multiLine = false;
    };

    struct ImportTypeNode: NodeWithTypeArguments, BrandKind<SyntaxKind::ImportType> {
        bool isTypeOf;
        NodeType<TypeNode> argument;
        optional<NodeType<ImportTypeAssertionContainer>> assertions;
        optional<NodeType<EntityName>> qualifier;
    };

    struct ThisTypeNode: TypeNode, BrandKind<SyntaxKind::ThisType> {};

    struct CallSignatureDeclaration: SignatureDeclarationBase, TypeElement, BrandKind<SyntaxKind::CallSignature> {};
    struct ConstructSignatureDeclaration: SignatureDeclarationBase, TypeElement, BrandKind<SyntaxKind::ConstructSignature> {};

    struct TypeLiteralNode: TypeNode, Declaration, BrandKind<SyntaxKind::TypeLiteral> {
        NodeArray<TypeElement> members;
    };

    struct ObjectTypeDeclaration: NodeType<ClassLikeDeclaration, InterfaceDeclaration, TypeLiteralNode> {};

    struct MethodSignature: SignatureDeclarationBase, TypeElement, BrandKind<SyntaxKind::MethodSignature> {
        NodeType<ObjectTypeDeclaration> parent;
        NodeType<PropertyName> name;
    };

    struct IndexSignatureDeclaration: SignatureDeclarationBase, ClassElement, TypeElement, BrandKind<SyntaxKind::IndexSignature> {
        optional<NodeArray<Decorator>> decorators;           // Array of decorators (in document order)
        optional<NodeArray<Modifier>> modifiers;            // Array of modifiers
        NodeType<ObjectTypeDeclaration> parent;
        NodeType<TypeNode> type;
    };

    struct FunctionOrConstructorTypeNodeBase: TypeNode, SignatureDeclarationBase {
        NodeType<TypeNode> type;
    };

    struct FunctionTypeNode: FunctionOrConstructorTypeNodeBase, BrandKind<SyntaxKind::FunctionType> {};

    struct ConstructorTypeNode: FunctionOrConstructorTypeNodeBase, BrandKind<SyntaxKind::ConstructorType> {};

    struct FunctionDeclaration: FunctionLikeDeclarationBase, DeclarationStatement, BrandKind<types::FunctionDeclaration> {
        optional<NodeType<Identifier>> name;
        optional<NodeType<FunctionBody>> body;
    };

    struct ConstructorDeclaration: FunctionLikeDeclarationBase, ClassElement, BrandKind<types::Constructor> {
        NodeType<ClassLikeDeclaration> parent;
        optional<NodeType<FunctionBody>> body;

        /* @internal */ optional<NodeArray<TypeParameterDeclaration>> typeParameters; // Present for use with reporting a grammar error
        /* @internal */ optional<NodeType<TypeNode>> type; // Present for use with reporting a grammar error
    };

    struct BinaryExpression: Expression, Declaration, BrandKind<types::BinaryExpression> {
        NodeType<Expression> left;
        NodeType<Unknown> operatorToken; //BinaryOperatorToken uses a lot of different NodeType<T>
        NodeType<Expression> right;
    };

//    using AssignmentOperatorToken = Token<AssignmentOperator>;

    struct AssignmentExpression: BinaryExpression {
        NodeType<LeftHandSideExpression> left;
    };

    struct ObjectDestructuringAssignment: BinaryExpression {
        NodeType<ObjectLiteralExpression> left;
        NodeType<EqualsToken> operatorToken;
    };

    struct ArrayLiteralExpression;

    struct ArrayDestructuringAssignment: BinaryExpression {
        NodeType<ArrayLiteralExpression> left;
        NodeType<EqualsToken> operatorToken;
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
        NodeType<Expression> condition;
        NodeType<QuestionToken> questionToken;
        NodeType<Expression> whenTrue;
        NodeType<ColonToken> colonToken;
        NodeType<Expression> whenFalse;
    };

    struct FunctionExpression: PrimaryExpression, FunctionLikeDeclarationBase, BrandKind<types::FunctionExpression> {
        optional<NodeType<Identifier>> name;
        optional<NodeType<FunctionBody>> body; // Required, whereas the member inherited from FunctionDeclaration is optional
    };

//    struct SignatureDeclaration: NodeType<CallSignatureDeclaration, ConstructSignatureDeclaration, MethodSignature, IndexSignatureDeclaration, FunctionTypeNode, ConstructorTypeNode, JSDocFunctionType, FunctionDeclaration, MethodDeclaration, ConstructorDeclaration, AccessorDeclaration, FunctionExpression, ArrowFunction> {};

    struct TypePredicateNode: TypeNode, BrandKind<SyntaxKind::TypePredicate> {
        NodeType<CallSignatureDeclaration, ConstructSignatureDeclaration, MethodSignature, IndexSignatureDeclaration, FunctionTypeNode, ConstructorTypeNode, FunctionDeclaration, MethodDeclaration, ConstructorDeclaration, AccessorDeclaration, FunctionExpression, ArrowFunction> parent;
        optional<NodeType<AssertsKeyword>> assertsModifier;
        NodeType<Identifier, ThisTypeNode> parameterName;
        optional<NodeType<TypeNode>> type;
    };

    struct ArrayLiteralExpression: PrimaryExpression, BrandKind<SyntaxKind::ArrayLiteralExpression> {
        NodeArray<Expression> elements;
        /* @internal */
        bool multiLine; //optional
    };

    struct CallExpression: LeftHandSideExpression, Declaration, BrandKind<SyntaxKind::CallExpression> {
        NodeType<LeftHandSideExpression> expression;
        optional<NodeType<QuestionDotToken>> questionDotToken;
        optional<NodeArray<TypeNode>> typeArguments;
        NodeArray<Expression> arguments;
    };

    struct CallChain: CallExpression {};

    /* @internal */
    struct CallChainRoot: CallChain {};

    struct NewExpression: PrimaryExpression, Declaration, BrandKind<SyntaxKind::NewExpression> {
        NodeType<LeftHandSideExpression> expression;
        optional<NodeArray<TypeNode>> typeArguments;
        optional<NodeArray<Expression>> arguments;
    };

    struct TypeAssertion: UnaryExpression, BrandKind<SyntaxKind::TypeAssertionExpression> {
        NodeType<TypeNode> type;
        NodeType<UnaryExpression> expression;
    };

    struct TemplateSpan;
    struct TemplateHead;
    struct TemplateLiteralTypeNode;
    struct TemplateMiddle;
    struct TemplateTail;

    struct TemplateExpression: PrimaryExpression, BrandKind<SyntaxKind::TemplateExpression> {
        NodeType<TemplateHead> head;
        NodeArray<TemplateSpan> templateSpans;
    };

    struct TemplateLiteralTypeSpan: TypeNode, BrandKind<SyntaxKind::TemplateLiteralTypeSpan> {
        NodeType<TemplateLiteralTypeNode> parent;
        NodeType<TypeNode> type;
        NodeType<TemplateMiddle, TemplateTail> literal;
    };

    struct TemplateLiteralTypeNode: TypeNode, BrandKind<SyntaxKind::TemplateLiteralType> {
        NodeType<TemplateHead> head;
        NodeArray<TemplateLiteralTypeSpan> templateSpans;
    };

    struct TemplateHead: TemplateLiteralLikeNode, BrandKind<SyntaxKind::TemplateHead> {
        NodeType<TemplateExpression, TemplateLiteralTypeNode> parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateMiddle: TemplateLiteralLikeNode, BrandKind<SyntaxKind::TemplateMiddle> {
        NodeType<TemplateSpan, TemplateLiteralTypeSpan> parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateTail: TemplateLiteralLikeNode, BrandKind<SyntaxKind::TemplateTail> {
        NodeType<TemplateSpan, TemplateLiteralTypeSpan> parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateLiteral: NodeType<TemplateExpression, NoSubstitutionTemplateLiteral> {};

    struct TaggedTemplateExpression: MemberExpression, BrandKind<SyntaxKind::TaggedTemplateExpression> {
        NodeType<LeftHandSideExpression> tag;
        optional<NodeArray<TypeNode>> typeArguments;
        TemplateLiteral templateLiteral;
        /*@internal*/ optional<NodeType<QuestionDotToken>> questionDotToken; // NOTE: Invalid syntax, only used to report a grammar error.
    };

    struct TemplateSpan: BaseNodeArray, BrandKind<SyntaxKind::TemplateSpan> {
        NodeType<TemplateExpression> parent;
        NodeType<Expression> expression;
        NodeType<TemplateMiddle, TemplateTail> literal;
    };

    struct AsExpression: Expression, BrandKind<SyntaxKind::AsExpression> {
        NodeType<Expression> expression;
        NodeType<TypeNode> type;
    };

    struct NonNullExpression: LeftHandSideExpression, BrandKind<SyntaxKind::NonNullExpression> {
        NodeType<Expression> expression;
    };

    struct ParenthesizedExpression: PrimaryExpression, BrandKind<SyntaxKind::ParenthesizedExpression> {
        NodeType<Expression> expression;
    };

    struct SpreadElement: Expression, BrandKind<SyntaxKind::SpreadElement> {
        NodeType<ArrayLiteralExpression, CallExpression, NewExpression> parent;
        NodeType<Expression> expression;
    };

    struct SpreadAssignment: BaseNodeStructure, BrandKind<SyntaxKind::SpreadAssignment> {
        NodeType<NodeType<Expression>> expression;
    };

    struct TypeQueryNode: NodeWithTypeArguments, BrandKind<SyntaxKind::TypeQuery> {
        EntityName exprName;
    };

    struct ArrayTypeNode: TypeNode, BrandKind<SyntaxKind::ArrayType> {
        NodeType<TypeNode> elementType;
    };

    struct NamedTupleMember: TypeNode, Declaration, BrandKind<SyntaxKind::NamedTupleMember> {
        optional<NodeType<DotDotDotToken>> dotDotDotToken;
        NodeType<Identifier> name;
        optional<NodeType<QuestionToken>> questionToken;
        NodeType<TypeNode> type;
    };

    struct OptionalTypeNode: TypeNode, BrandKind<SyntaxKind::OptionalType> {
        NodeType <TypeNode> type;
    };

    struct RestTypeNode: TypeNode, BrandKind<SyntaxKind::RestType> {
        NodeType <TypeNode> type;
    };

    struct UnionTypeNode: TypeNode, BrandKind<SyntaxKind::UnionType> {
        NodeArray<TypeNode> types;
    };

    struct IntersectionTypeNode: TypeNode, BrandKind<SyntaxKind::IntersectionType> {
        NodeArray<TypeNode> types;
    };

    struct UnionOrIntersectionTypeNode: NodeType<UnionTypeNode, IntersectionTypeNode> {};

    struct ConditionalTypeNode: TypeNode, BrandKind<SyntaxKind::ConditionalType> {
        NodeType<TypeNode> checkType;
        NodeType<TypeNode> extendsType;
        NodeType<TypeNode> trueType;
        NodeType<TypeNode> falseType;
    };

    struct InferTypeNode: TypeNode, BrandKind<SyntaxKind::InferType> {
        NodeType<TypeParameterDeclaration> typeParameter;
    };

    struct ParenthesizedTypeNode: TypeNode, BrandKind<SyntaxKind::ParenthesizedType> {
        NodeType<TypeNode> type;
    };

    struct TypeOperatorNode: TypeNode, BrandKind<SyntaxKind::TypeOperator> {
        SyntaxKind operatorKind;
        NodeType<TypeNode> type;
    };

    /* @internal */
    struct UniqueTypeOperatorNode: TypeOperatorNode {
        SyntaxKind operatorKind = SyntaxKind::UniqueKeyword;
    };

    struct IndexedAccessTypeNode: TypeNode, BrandKind<SyntaxKind::IndexedAccessType> {
        NodeType<TypeNode> objectType;
        NodeType<TypeNode> indexType;
    };

    struct MappedTypeNode: TypeNode, Declaration, BrandKind<SyntaxKind::MappedType> {
        optional<NodeType<ReadonlyKeyword, PlusToken, MinusToken>> readonlyToken;
        NodeType<TypeParameterDeclaration> typeParameter;
        optional<NodeType<TypeNode>> nameType;
        optional<NodeType<QuestionToken, PlusToken, MinusToken>> questionToken;
        optional<NodeType<TypeNode>> type;
        /** Used only to produce grammar errors */
        optional<NodeArray<TypeElement>> members;
    };

    struct JsxOpeningElement;
    struct JsxClosingElement;
    struct JsxText;
    struct JsxExpression;
    struct JsxElement;
    struct JsxSelfClosingElement;
    struct JsxFragment;

    #define JsxChild JsxText, JsxExpression, JsxElement, JsxSelfClosingElement, JsxFragment
    #define JsxAttributeValue StringLiteral, JsxExpression, JsxElement, JsxSelfClosingElement, JsxFragment
    #define JsxAttributeLike JsxAttribute, JsxSpreadAttribute

    /// A JSX expression of the form <TagName attrs>...</TagName>
    struct JsxElement: PrimaryExpression, BrandKind<SyntaxKind::JsxElement> {
        NodeType<JsxOpeningElement> openingElement;
        NodeArray<JsxChild> children;
        NodeType<JsxClosingElement> closingElement;
    };

//    /// Either the opening tag in a <Tag>...</Tag> pair or the lone <Tag /> in a self-closing form
//    export type JsxOpeningLikeElement =
//        | JsxSelfClosingElement
//        | JsxOpeningElement
//        ;
//
//    export type JsxAttributeLike =
//        | JsxAttribute
//        | JsxSpreadAttribute
//        ;
//
//    export type JsxTagNameExpression =
//        | Identifier
//        | ThisExpression
//        | JsxTagNamePropertyAccess
//        ;

    struct JsxTagNameExpression;
    struct JsxOpeningLikeElement;
    struct JsxAttribute;
    struct JsxFragment;
    struct JsxSpreadAttribute;
    struct JsxClosingFragment;

    struct JsxTagNamePropertyAccess: PropertyAccessExpression {
        NodeType<JsxTagNameExpression> expression;
    };

    struct JsxAttributes: PrimaryExpression, Declaration, BrandKind<SyntaxKind::JsxAttributes> {
        NodeType<JsxOpeningLikeElement> parent;
        NodeArray<JsxAttribute, JsxSpreadAttribute> properties;
    };

    /// The opening element of a <Tag>...</Tag> JsxElement
    struct JsxOpeningElement: Expression, BrandKind<SyntaxKind::JsxOpeningElement> {
        NodeType<JsxElement> parent;
        NodeType<JsxTagNameExpression> tagName;
        optional<NodeArray<TypeNode>> typeArguments;
        NodeType<JsxAttributes> attributes;
    };

    /// A JSX expression of the form <TagName attrs />
    struct JsxSelfClosingElement: PrimaryExpression, BrandKind<SyntaxKind::JsxSelfClosingElement> {
        NodeType<JsxTagNameExpression> tagName;
        optional<NodeArray<TypeNode>> typeArguments;
        NodeType<JsxAttributes> attributes;
    };

    /// The opening element of a <>...</> JsxFragment
    struct JsxOpeningFragment: Expression, BrandKind<SyntaxKind::JsxOpeningFragment> {
        NodeType<JsxFragment> parent;
    };

    /// A JSX expression of the form <>...</>
    struct JsxFragment: PrimaryExpression, BrandKind<SyntaxKind::JsxFragment> {
        NodeType<JsxOpeningFragment> openingFragment;
        NodeArray<JsxChild> children;
        NodeType<JsxClosingFragment> closingFragment;
    };

    /// The closing element of a <>...</> JsxFragment
    struct JsxClosingFragment: Expression, BrandKind<SyntaxKind::JsxClosingFragment> {
        NodeType<JsxFragment> parent;
    };

    struct JsxAttribute: ObjectLiteralElement, BrandKind<SyntaxKind::JsxAttribute> {
        NodeType<JsxAttributes> parent;
        NodeType<Identifier> name;
        /// JSX attribute initializers are optional; <X y /> is sugar for <X y={true} />
        optional<NodeType<JsxAttributeValue>> initializer;
    };

//    export type JsxAttributeValue =
//        | StringLiteral
//        | JsxExpression
//        | JsxElement
//        | JsxSelfClosingElement
//        | JsxFragment;

    struct JsxSpreadAttribute: ObjectLiteralElement, BrandKind<SyntaxKind::JsxSpreadAttribute> {
        NodeType<JsxAttributes> parent;
        NodeType<Expression> expression;
    };

    struct JsxClosingElement: Node, BrandKind<SyntaxKind::JsxClosingElement> {
        NodeType<JsxElement> parent;
        NodeType<JsxTagNameExpression> tagName;
    };

    struct JsxExpression: Expression, BrandKind<SyntaxKind::JsxExpression> {
        NodeType<JsxElement, JsxFragment, JsxAttributeLike> parent;
        optional<NodeType<DotDotDotToken>> dotDotDotToken;
        optional<NodeType<Expression>> expression;
    };

    struct JsxText: LiteralLikeNode, BrandKind<SyntaxKind::JsxText> {
        NodeType<JsxElement, JsxFragment> parent;
        bool containsOnlyTriviaWhiteSpaces;
    };

//    export type JsxChild =
//        | JsxText
//        | JsxExpression
//        | JsxElement
//        | JsxSelfClosingElement
//        | JsxFragment
//        ;
//
//

    struct LiteralTypeNode: TypeNode, BrandKind<SyntaxKind::LiteralType> {
        NodeType<NullLiteral, BooleanLiteral, LiteralExpression, PrefixUnaryExpression> literal;
    };

    struct TupleTypeNode: TypeNode, BrandKind<SyntaxKind::TupleType> {
        NodeArray<TypeNode, NamedTupleMember> elements;
    };

    using AccessibilityModifier = NodeType<PublicKeyword, PrivateKeyword, ProtectedKeyword>;
    using ParameterPropertyModifier = NodeType<AccessibilityModifier, ReadonlyKeyword>;
    using ClassMemberModifier = NodeType<AccessibilityModifier, ReadonlyKeyword, StaticKeyword>;

    struct Decorator: BaseNodeStructureWithoutDecorators, BrandKind<SyntaxKind::Decorator> {
        NodeType<NamedDeclaration> parent;
        NodeType<Expression> expression;
    };

    struct EndOfFileToken: Token<SyntaxKind::EndOfFileToken> {};

    struct SourceFile: BaseNodeStructure, BrandKind<SyntaxKind::SourceFile> {
        string fileName;
        NodeArray<Statement> statements;
        NodeType<EndOfFileToken> endOfFileToken;

        optional<types::ModuleKind> impliedNodeFormat;

        optional<Node> externalModuleIndicator;
    };
}