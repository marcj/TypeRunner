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
#include <fmt/core.h>
#include <fmt/format.h>
#include "magic_enum.hpp"

namespace ts {
    struct SourceFile;
}

namespace ts::types {
    using namespace std;
    struct CompilerOptions;

    /**
     * This represents a string whose leading underscore have been escaped by adding extra leading underscores.
     *
     * The shape of this brand is rather unique compared to others we've used.
     * Instead of just an intersection of a string and an object, it is that union-ed
     * with an intersection of void and an object. This makes it wholly incompatible
     * with a normal string (which is good, it cannot be misused on assignment or on usage),
     * while still being comparable with a normal string via === (also good) and castable from a string.
     */
    using __String = std::string;

    /* @internal */
    enum class Comparison {
        LessThan = -1,
        EqualTo = 0,
        GreaterThan = 1
    };

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

    enum class CommentDirectiveType {
        ExpectError,
        Ignore,
    };

    struct TextRange {
        int pos = -1;
        int end = -1;
    };

    struct CommentDirective {
        TextRange range;
        CommentDirectiveType type;
    };

    enum class LanguageVariant {
        Standard,
        JSX
    };
    enum class DiagnosticCategory {
        Warning,
        Error,
        Suggestion,
        Message
    };

    struct DiagnosticMessage {
        int code;
        DiagnosticCategory category;
        string_view key;
        string_view message;
        bool reportsUnnecessary;
        /* @internal */
        bool elidedInCompatabilityPyramid;
        bool reportsDeprecated;

        DiagnosticMessage(int code, DiagnosticCategory category, const string_view &key, const string_view &message, bool reportsUnnecessary, bool elidedInCompatabilityPyramid, bool reportsDeprecated);
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
        string_view messageText;
        DiagnosticMessageChain *messageChain = nullptr;
    };

    struct Diagnostic: DiagnosticRelatedInformation {
        /** May store more in future. For now, this will simply be `true` to indicate when a diagnostic is an unused-identifier diagnostic. */
        bool reportsUnnecessary = false;

        bool reportsDeprecated = false;
        string source;
        optional<vector<DiagnosticRelatedInformation>> relatedInformation;
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

    struct Extension {
        constexpr static auto Ts = ".ts",
                Tsx = ".tsx",
                Dts = ".d.ts",
                Js = ".js",
                Jsx = ".jsx",
                Json = ".json",
                TsBuildInfo = ".tsbuildinfo",
                Mjs = ".mjs",
                Mts = ".mts",
                Dmts = ".d.mts",
                Cjs = ".cjs",
                Cts = ".cts",
                Dcts = ".d.cts";
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

    enum class SyntaxKind {
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

    enum class EmitFlags {
        None = 0,
        SingleLine = 1 << 0,                    // The contents of this node should be emitted on a single line.
        AdviseOnEmitNode = 1 << 1,              // The printer should invoke the onEmitNode callback when printing this node.
        NoSubstitution = 1 << 2,                // Disables further substitution of an expression.
        CapturesThis = 1 << 3,                  // The function captures a lexical `this`
        NoLeadingSourceMap = 1 << 4,            // Do not emit a leading source map location for this node.
        NoTrailingSourceMap = 1 << 5,           // Do not emit a trailing source map location for this node.
        NoSourceMap = NoLeadingSourceMap | NoTrailingSourceMap, // Do not emit a source map location for this node.
        NoNestedSourceMaps = 1 << 6,            // Do not emit source map locations for children of this node.
        NoTokenLeadingSourceMaps = 1 << 7,      // Do not emit leading source map location for token nodes.
        NoTokenTrailingSourceMaps = 1 << 8,     // Do not emit trailing source map location for token nodes.
        NoTokenSourceMaps = NoTokenLeadingSourceMaps | NoTokenTrailingSourceMaps, // Do not emit source map locations for tokens of this node.
        NoLeadingComments = 1 << 9,             // Do not emit leading comments for this node.
        NoTrailingComments = 1 << 10,           // Do not emit trailing comments for this node.
        NoComments = NoLeadingComments | NoTrailingComments, // Do not emit comments for this node.
        NoNestedComments = 1 << 11,
        HelperName = 1 << 12,                   // The Identifier refers to an *unscoped* emit helper (one that is emitted at the top of the file)
        ExportName = 1 << 13,                   // Ensure an export prefix is added for an identifier that points to an exported declaration with a local name (see SymbolFlags.ExportHasLocal).
        LocalName = 1 << 14,                    // Ensure an export prefix is not added for an identifier that points to an exported declaration.
        InternalName = 1 << 15,                 // The name is internal to an ES5 class body function.
        Indented = 1 << 16,                     // Adds an explicit extra indentation level for class and function bodies when printing (used to match old emitter).
        NoIndentation = 1 << 17,                // Do not indent the node.
        AsyncFunctionBody = 1 << 18,
        ReuseTempVariableScope = 1 << 19,       // Reuse the existing temp variable scope during emit.
        CustomPrologue = 1 << 20,               // Treat the statement as if it were a prologue directive (NOTE: Prologue directives are *not* transformed).
        NoHoisting = 1 << 21,                   // Do not hoist this declaration in --module system
        HasEndOfDeclarationMarker = 1 << 22,    // Declaration has an associated NotEmittedStatement to mark the end of the declaration
        Iterator = 1 << 23,                     // The expression to a `yield*` should be treated as an Iterator when down-leveling, not an Iterable.
        NoAsciiEscaping = 1 << 24,              // When synthesizing nodes that lack an original node or textSourceNode, we want to write the text on the node with ASCII escaping substitutions.
        /*@internal*/ TypeScriptClassWrapper = 1 << 25, // The node is an IIFE class wrapper created by the ts transform.
        /*@internal*/ NeverApplyImportHelper = 1 << 26, // Indicates the node should never be wrapped with an import star helper (because, for example, it imports tslib itself)
        /*@internal*/ IgnoreSourceNewlines = 1 << 27,   // Overrides `printerOptions.preserveSourceNewlines` to print this node (and all descendants) with default whitespace.
        /*@internal*/ Immutable = 1 << 28,      // Indicates a node is a singleton intended to be reused in multiple locations. Any attempt to make further changes to the node will result in an error.
        /*@internal*/ IndirectCall = 1 << 29,   // Emit CallExpression as an indirect call: `(0, f)()`
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

    enum class OuterExpressionKinds {
        Parentheses = 1 << 0,
        TypeAssertions = 1 << 1,
        NonNullAssertions = 1 << 2,
        PartiallyEmittedExpressions = 1 << 3,

        Assertions = TypeAssertions | NonNullAssertions,
        All = Parentheses | Assertions | PartiallyEmittedExpressions,

        ExcludeJSDocTypeAssertion = 1 << 4,
    };
}

namespace ts {
    using namespace std;

    using types::SyntaxKind;

    struct PseudoBigInt {
        bool negative;
        string base10Value;
    };

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

    struct NodeArray {
        vector<shared<Node>> list;
        int pos;
        int end;
        bool hasTrailingComma = false;
        bool isMissingList = false; //replaces `MissingList extends NodeArray {bool isMissingList;}`
        /* @internal */ int transformFlags = 0;   // Flags for transforms, possibly undefined

        NodeArray() {}

        NodeArray(const vector<shared<Node>> &list, bool hasTrailingComma = false): list(list), hasTrailingComma(hasTrailingComma) {}

        NodeArray(const shared<Node> &item) {
            list.push_back(item);
        }

        int length() {
            return list.size();
        }

        bool empty() {
            return list.empty();
        }

        void push(shared<Node> node) {
            list.push_back(node);
        }

        bool operator==(NodeArray &other) {
            if (pos != other.pos || end != other.end || hasTrailingComma != other.hasTrailingComma || isMissingList != other.isMissingList) return false;
            if (transformFlags != other.transformFlags) return false;
            if (list.size() != other.list.size()) return false;
            for (int i = 0; i < list.size(); i++) {
                if (list[i] != other.list[i]) return false;
            }
            return true;
        }

        vector<shared<Node>> slice(int start, int end = 0) {
            return slice < shared<Node>>
            (list, start, end);
        }
    };

    template<class T>
    bool some(sharedOpt<NodeArray> array, function<bool(shared<T>)> callback) {
        if (!array) return false;
        for (auto &&item: array->list) {
            if (callback(reinterpret_pointer_cast<T>(item))) return true;
        }
        return false;
    }

    inline bool some(sharedOpt<NodeArray> array) {
        return array && !array->empty();
    }

#define NodeTypeArray(x...) NodeArray

//    /**
//     * Union is like Node, it is the owner of the data
//     */
//    struct BaseUnion {
//        shared<Node> node;
//        BaseUnion();
//
//        SyntaxKind kind();
//
//        bool empty();
//    };

    struct SourceMapSource {
        string fileName;
        string text;
        /* @internal */ vector<int> lineMap;
        optional<function<int(int)>> skipTrivia;
    };

    struct SourceMapRange: types::TextRange {
        optional<SourceMapSource> source;
    };

    struct CommentRange: types::TextRange {
        bool hasTrailingNewLine = false;
        SyntaxKind kind; //SyntaxKind.SingleLineCommentTrivia | SyntaxKind.MultiLineCommentTrivia
    };

    struct SynthesizedComment: CommentRange {
        string text;
        bool hasLeadingNewline = false;
    };

    struct EmitNode {
//        optional<vector<shared<Node>>> annotatedNodes;                 // Tracks Parse-tree nodes with EmitNodes for eventual cleanup.
        /*EmitFlags*/ int flags = 0;                        // Flags that customize emit
        optional<vector<SynthesizedComment>> leadingComments;  // Synthesized leading comments
        optional<vector<SynthesizedComment>> trailingComments;  // Synthesized trailing comments
        sharedOpt<types::TextRange> commentRange;                // The text range to use when emitting leading or trailing comments
        sharedOpt<SourceMapRange> sourceMapRange;         // The text range to use when emitting leading or trailing source mappings
        //todo: if we enable that, we have to adjust mergeEmitNode()
//        tokenSourceMapRanges?: (SourceMapRange | undefined)[]; // The text range to use when emitting source mappings for tokens
//        constantValue?: string | number;         // The constant value of an expression
//        externalHelpersModuleName?: Identifier;  // The local name for an imported helpers module
//        externalHelpers?: boolean;
//        helpers?: EmitHelper[];                  // Emit helpers for the node
//        startsOnNewLine?: boolean;               // If the node should begin on a new line
//        snippetElement?: SnippetElement;         // Snippet element of the node
//        typeNode?: TypeNode;                         // VariableDeclaration type
    };

    /**
     * All BaseNode pointers are owned by SourceFile. If SourceFile destroys, all its Nodes are destroyed as well.
     *
     * There are a big variety of sub types: All have in common that they are the owner of their data (except *parent).
     */
    class Node: public SourceMapRange {
    protected:
        Node &parent = *this;                                 // Parent BaseNode (initialized by binding)
    public:
        SyntaxKind kind = SyntaxKind::Unknown;
        constexpr static auto KIND = SyntaxKind::Unknown;
        /* types::NodeFlags */ int flags;
        /* @internal */ /* types::ModifierFlags */ int modifierFlagsCache;
        sharedOpt<NodeTypeArray(Modifier)> modifiers;            // Array of modifiers
        sharedOpt<NodeTypeArray(Decorator)> decorators;           // Array of decorators (in document order)
        /* @internal */ /* types::TransformFlags */ int transformFlags; // Flags for transforms
//        /* @internal */ id?: NodeId;                          // Unique id (used to look up NodeLinks)
        /* @internal */ sharedOpt<Node> original;                      // The original BaseNode if this is an updated node.
//        /* @internal */ symbol: Symbol;                       // Symbol declared by BaseNode (initialized by binding)
//        /* @internal */ locals?: SymbolTable;                 // Locals associated with BaseNode (initialized by binding)
//        /* @internal */ nextContainer?: Node;                 // Next container in declaration order (initialized by binding)
//        /* @internal */ localSymbol?: Symbol;                 // Local symbol declared by BaseNode (initialized by binding only for exported nodes)
//        /* @internal */ flowNode?: FlowNode;                  // Associated FlowNode (initialized by binding)
        /* @internal */ sharedOpt<EmitNode> emitNode; //?: ;                  // Associated EmitNode (initialized by transforms)
//        /* @internal */ contextualType?: Type;                // Used to temporarily assign a contextual type during overload resolution
//        /* @internal */ inferenceContext?: InferenceContext;  // Inference context for contextual type

        virtual ~Node() {
        }

        bool hasParent() {
            return &parent != this;
        }

        Node &getParent() {
            if (!hasParent()) throw std::runtime_error("Node has no parent set");
            return parent;
        }

        //using dynamic_cast
        template<class T>
        T &cast() {
            return dynamic_cast<T &>(*this);
        }

        //using dynamic_cast
        template<class T>
        bool validCast() {
            return dynamic_cast<T *>(this) != nullptr;
        }

        template<class T>
        bool is() {
            if (T::KIND == SyntaxKind::Unknown) throw runtime_error("Passed Node type has unknown kind.");
            return this->kind == T::KIND;
        }

        template<typename T>
        T &to() {
            if (T::KIND == SyntaxKind::Unknown) throw runtime_error("Passed Node type has unknown kind.");
            if (kind != T::KIND) throw std::runtime_error(fmt::format("Can not convert Node, from kind {} to {}", kind, T::KIND));
            return *reinterpret_cast<T *>(this);
        }
    };

    template<class T>
    sharedOpt<T> to(sharedOpt<Node> p) {
        if (!p) return nullptr;
        if (T::KIND != SyntaxKind::Unknown && p->kind != T::KIND) return nullptr;
        return reinterpret_pointer_cast<T>(p);
    }

    inline sharedOpt<Node> operator||(sharedOpt<Node> a, sharedOpt<Node> b) {
        if (a) return a;
        return b;
    };

    inline bool operator==(sharedOpt<NodeArray> a, sharedOpt<NodeArray> b) {
        if (!a && !b) return true;
        if (!a || !b) return false;
        return *a == *b;
    };

    template<typename Default, typename ... T>
    struct BaseNodeUnion: Node {
    };

#define NodeUnion(x...) Node

    //just to have the type information in the code
    template<typename ... Types>
    using UnionNode = Node;

#define FIRST_ARG_(N, ...) N
#define FIRST_ARG(args) FIRST_ARG_ args
/**
 * Defines a shared union property and initializes its first type.
 */
#define UnionProperty(name, Types...) shared<Node> name = make_shared<FIRST_ARG((Types))>()
#define OptionalUnionProperty(name, Types...) sharedOpt<Node> name

//Parent properties can not have initializer as it would lead to circular ref. We expect from the user to set it where required.
#define ParentProperty(Types...) shared<UnionNode<Types>> parent
#define Property(name, Type) shared<Type> name = make_shared<Type>()
#define OptionalProperty(name, Type) sharedOpt<Type> name

    struct DeclarationName;

    struct Statement: Node {};

    struct NamedDeclaration {
        OptionalProperty(name, DeclarationName);

        bool brandNamedDeclaration = true;
    };

    struct Expression: Node {};

    struct UnaryExpression: Expression {};

    struct UpdateExpression: UnaryExpression {};

    struct LeftHandSideExpression: UpdateExpression {};

    struct MemberExpression: LeftHandSideExpression {};

    struct PrimaryExpression: MemberExpression {};

    struct PrivateIdentifier: BrandKind<SyntaxKind::PrivateIdentifier, PrimaryExpression> {
        string escapedText;
    };

    template<SyntaxKind T>
    struct Token: BrandKind<T, Node> {};

    struct DotToken: Token<SyntaxKind::DotToken> {};
    struct DotDotDotToken: Token<SyntaxKind::DotDotDotToken> {};
    struct QuestionToken: Token<SyntaxKind::QuestionToken> {};
    struct CommaToken: Token<SyntaxKind::CommaToken> {};
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

    struct BooleanLiteral: NodeUnion(TrueLiteral, FalseLiteral) {};

    struct ThisExpression: BrandKind<SyntaxKind::ThisKeyword, PrimaryExpression> {};

    struct SuperExpression: BrandKind<SyntaxKind::SuperKeyword, PrimaryExpression> {};

    struct ImportExpression: BrandKind<SyntaxKind::ImportKeyword, PrimaryExpression> {};

    using PostfixUnaryOperator = SyntaxKind; //SyntaxKind.PlusPlusToken | SyntaxKind.MinusMinusToken

    struct PrefixUnaryExpression: BrandKind<SyntaxKind::PrefixUnaryExpression, UpdateExpression> {
        Property(operand, Node); //LeftHandSideExpression
        PostfixUnaryOperator operatorKind = SyntaxKind::Unknown;
    };

    struct PartiallyEmittedExpression: BrandKind<SyntaxKind::PartiallyEmittedExpression, LeftHandSideExpression> {
        Property(expression, Expression);
    };

    struct PostfixUnaryExpression: BrandKind<SyntaxKind::PostfixUnaryExpression, UpdateExpression> {
        Property(operand, LeftHandSideExpression);
        PostfixUnaryOperator operatorKind = SyntaxKind::Unknown;
    };

    struct DeleteExpression: BrandKind<SyntaxKind::DeleteExpression, UnaryExpression> {
        Property(expression, UnaryExpression);
    };

    struct TypeOfExpression: BrandKind<SyntaxKind::TypeOfExpression, UnaryExpression> {
        Property(expression, UnaryExpression);
    };

    struct VoidExpression: BrandKind<SyntaxKind::VoidExpression, UnaryExpression> {
        Property(expression, UnaryExpression);
    };

    struct AwaitExpression: BrandKind<SyntaxKind::AwaitExpression, UnaryExpression> {
        Property(expression, UnaryExpression);
    };

    struct YieldExpression: BrandKind<SyntaxKind::YieldExpression, Expression> {
        OptionalProperty(asteriskToken, AsteriskToken);
        OptionalProperty(expression, Expression);
    };

    //this seems to be related to instantiated types
    struct Type {};

    struct ParameterDeclaration;
    struct NamedTupleMember;

    struct SyntheticExpression: BrandKind<SyntaxKind::SyntheticExpression, Expression> {
        bool isSpread;
        Property(type, Type);
        OptionalUnionProperty(tupleNameSource, ParameterDeclaration, NamedTupleMember);
    };

    struct TypeNode: Node {};

    /** @deprecated Use `AwaitKeyword` instead. */
    using AwaitKeywordToken = AwaitKeyword;

    /** @deprecated Use `AssertsKeyword` instead. */
    using AssertsToken = AssertsKeyword;

    /** @deprecated Use `ReadonlyKeyword` instead. */
    using ReadonlyToken = ReadonlyKeyword;

    struct Modifier: NodeUnion(
            AbstractKeyword, AsyncKeyword, ConstKeyword, DeclareKeyword, DefaultKeyword, ExportKeyword, InKeyword, PrivateKeyword, ProtectedKeyword, PublicKeyword, OutKeyword, OverrideKeyword, ReadonlyKeyword, StaticKeyword) {
    };

    struct ModifiersArray: NodeTypeArray(Modifier) {};

    struct LiteralLike: PrimaryExpression {
        std::string text;
        optional<bool> isUnterminated;
        optional<bool> hasExtendedUnicodeEscape;
    };

    struct LiteralExpression: LiteralLike {};

    struct Identifier;
    struct NoSubstitutionTemplateLiteral;
    struct NumericLiteral;

    struct StringLiteral: BrandKind<SyntaxKind::StringLiteral, LiteralExpression> {
        OptionalUnionProperty(textSourceNode, Identifier, StringLiteral, NoSubstitutionTemplateLiteral, NumericLiteral);
        optional<bool> singleQuote;
    };

    struct ImportSpecifier;

    enum class GeneratedIdentifierFlags {
        // Kinds
        None = 0,                           // Not automatically generated.
        /*@internal*/ Auto = 1,             // Automatically generated identifier.
        /*@internal*/ Loop = 2,             // Automatically generated identifier with a preference for '_i'.
        /*@internal*/ Unique = 3,           // Unique name based on the 'text' property.
        /*@internal*/ Node = 4,             // Unique name based on the node in the 'original' property.
        /*@internal*/ KindMask = 7,         // Mask to extract the kind of identifier from its flags.

        // Flags
        ReservedInNestedScopes = 1 << 3,    // Reserve the generated name in nested scopes
        Optimistic = 1 << 4,                // First instance won't use '_#' if there's no conflict
        FileLevel = 1 << 5,                 // Use only the file identifiers list and not generated names to search for conflicts
        AllowNameSubstitution = 1 << 6, // Used by `module.ts` to indicate generated nodes which can have substitutions performed upon them (as they were generated by an earlier transform phase)
    };

    struct Identifier: BrandKind<SyntaxKind::Identifier, PrimaryExpression> {
        /**
         * Prefer to use `id.unescapedText`. (Note: This is available only in services, not internally to the TypeScript compiler.)
         * Text of identifier, but if the identifier begins with two underscores, this will begin with three.
         */
        string escapedText;
        optional<SyntaxKind> originalKeywordKind;// Original syntaxKind which get set so that we can report an error later

        optional<bool> isInJSDocNamespace;
        /*@internal*/ optional</*GeneratedIdentifierFlags*/int> autoGenerateFlags; // Specifies whether to auto-generate the text for an identifier.
        /*@internal*/ optional<int> autoGenerateId;           // Ensures unique generated identifiers get unique names, but clones get the same name.
        /*@internal*/ sharedOpt<ImportSpecifier> generatedImportReference; // Reference to the generated import specifier this identifier refers to
        /*@internal*/ sharedOpt<NodeArray> typeArguments; // Only defined on synthesized nodes. Though not syntactically valid, used in emitting diagnostics, quickinfo, and signature help.
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
        shared<NodeTypeArray(Statement)> statements;
        /*@internal*/ bool multiLine;
    };

    struct TemplateLiteralLike: LiteralLike {
        optional<string> rawText = {};
        /* @internal */
        optional<int> templateFlags; //types::TokenFlags
    };

    struct RegularExpressionLiteral: BrandKind<SyntaxKind::RegularExpressionLiteral, LiteralExpression> {
    };

    struct NoSubstitutionTemplateLiteral: BrandKind<SyntaxKind::NoSubstitutionTemplateLiteral, LiteralExpression> {
        OptionalProperty(rawText, string);
        optional<int> templateFlags; //types::TokenFlags
    };

    struct NumericLiteral: BrandKind<SyntaxKind::NumericLiteral, LiteralExpression> {
        int numericLiteralFlags = types::TokenFlags::None;
    };

    struct BigIntLiteral: BrandKind<SyntaxKind::BigIntLiteral, LiteralExpression> {
    };

    struct ComputedPropertyName: BrandKind<SyntaxKind::ComputedPropertyName, Node> {
        Property(expression, Expression);
    };

    struct QualifiedName;

#define EntityName Identifier, QualifiedName

    struct QualifiedName: BrandKind<SyntaxKind::QualifiedName, Node> {
        shared<NodeUnion(EntityName)> left;
        shared<Identifier> right = make_shared<Identifier>();
        /*@internal*/ OptionalProperty(jsdocDotPos, int); // QualifiedName occurs in JSDoc-style generic: Id1.Id2.<T>
    };

    struct ElementAccessExpression: BrandKind<SyntaxKind::ElementAccessExpression, MemberExpression> {
        Property(expression, LeftHandSideExpression);
        OptionalProperty(questionDotToken, QuestionDotToken);
        Property(argumentExpression, Expression);
    };

    struct OmittedExpression: BrandKind<SyntaxKind::OmittedExpression, Expression> {};

    struct VariableDeclaration;
    struct ParameterDeclaration;
    struct ObjectBindingPattern;
    struct ArrayBindingPattern;
    struct ArrayBindingPattern;

#define BindingPattern ObjectBindingPattern, ArrayBindingPattern
#define BindingName Identifier, BindingPattern
#define PropertyName Identifier, StringLiteral, NumericLiteral, ComputedPropertyName, PrivateIdentifier

    struct BindingElement: BrandKind<SyntaxKind::BindingElement, NamedDeclaration, Node> {
        OptionalUnionProperty(propertyName, PropertyName);        // Binding property name (in object binding pattern)
        OptionalProperty(dotDotDotToken, DotDotDotToken);    // Present on rest element (in object binding pattern)
        shared<NodeUnion(BindingName)> name;                  // Declared binding element name
        OptionalProperty(initializer, Expression);           // Optional initializer
    };

#define ArrayBindingElement BindingElement, OmittedExpression

    struct ObjectBindingPattern: BrandKind<SyntaxKind::ObjectBindingPattern, Node> {
        shared<NodeTypeArray(BindingElement)> elements;
        ParentProperty(VariableDeclaration, ParameterDeclaration, BindingElement);
    };

    struct ArrayBindingPattern: BrandKind<SyntaxKind::ArrayBindingPattern, Node> {
        ParentProperty(VariableDeclaration, ParameterDeclaration, BindingElement);
        shared<NodeTypeArray(ArrayBindingElement)> elements;
    };

    struct ExpressionStatement: BrandKind<SyntaxKind::ExpressionStatement, Statement> {
        Property(expression, Expression);
    };

    struct PrologueDirective: ExpressionStatement {
        Property(expression, StringLiteral);
    };

    struct IfStatement: BrandKind<SyntaxKind::IfStatement, Statement> {
        Property(expression, Expression);
        Property(thenStatement, Statement);
        OptionalProperty(elseStatement, Statement);
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
        OptionalProperty(label, Identifier);
    };

    struct ContinueStatement: BrandKind<SyntaxKind::ContinueStatement, Statement> {
        OptionalProperty(label, Identifier);
    };

//    export type BreakOrContinueStatement =
//        | BreakStatement
//        | ContinueStatement
//        ;

    struct ReturnStatement: BrandKind<SyntaxKind::ReturnStatement, Statement> {
        OptionalProperty(expression, Expression);
    };

    struct WithStatement: BrandKind<SyntaxKind::WithStatement, Statement> {
        Property(expression, Expression);
        Property(statement, Statement);
    };

    struct SwitchStatement;
    struct CaseClause;
    struct DefaultClause;

    struct CaseBlock: BrandKind<SyntaxKind::CaseBlock, Node> {
        shared<SwitchStatement> parent;
        shared<NodeTypeArray(CaseClause, DefaultClause)> clauses;
    };

    struct SwitchStatement: BrandKind<SyntaxKind::SwitchStatement, Statement> {
        Property(expression, Expression);
        Property(caseBlock, CaseBlock);
        bool possiblyExhaustive; // initialized by binding
    };

    struct CaseClause: BrandKind<SyntaxKind::CaseClause, Node> {
        Property(parent, CaseBlock);
        Property(expression, Expression);
        shared<NodeTypeArray(Statement)> statements;
    };

    struct DefaultClause: BrandKind<SyntaxKind::DefaultClause, Node> {
        shared<CaseBlock> parent;
        shared<NodeTypeArray(Statement)> statements;
    };

//    export type CaseOrDefaultClause =
//        | CaseClause
//        | DefaultClause
//        ;

    struct LabeledStatement: BrandKind<SyntaxKind::LabeledStatement, Statement> {
        Property(label, Identifier);
        Property(statement, Statement);
    };

    struct ThrowStatement: BrandKind<SyntaxKind::ThrowStatement, Statement> {
        Property(expression, Expression);
    };

    struct IterationStatement: Statement {
        Property(statement, Statement);
    };

    struct DoStatement: BrandKind<SyntaxKind::DoStatement, IterationStatement> {
        Property(expression, Expression);
    };

    struct WhileStatement: BrandKind<SyntaxKind::WhileStatement, IterationStatement> {
        Property(expression, Expression);
    };

    struct VariableDeclarationList;

    using ForInitializer = NodeUnion(VariableDeclarationList, Expression);

    struct ForStatement: BrandKind<SyntaxKind::ForStatement, IterationStatement> {
        OptionalProperty(initializer, ForInitializer);
        OptionalProperty(condition, Expression);
        OptionalProperty(incrementor, Expression);
    };

    struct ForOfStatement: BrandKind<SyntaxKind::ForOfStatement, IterationStatement> {
        Property(awaitModifier, AwaitKeyword);
        Property(initializer, ForInitializer);
        Property(expression, Expression);
    };

    struct ForInStatement: BrandKind<SyntaxKind::ForInStatement, IterationStatement> {
        Property(initializer, ForInitializer);
        Property(expression, Expression);
    };

    struct VariableStatement;

    struct VariableDeclarationList: BrandKind<SyntaxKind::VariableDeclarationList, Node> {
        ParentProperty(VariableStatement, ForStatement, ForOfStatement, ForInStatement);
        shared<NodeTypeArray(VariableDeclaration)> declarations;
    };

    struct VariableStatement: BrandKind<SyntaxKind::VariableStatement, Statement> {
//        /* @internal*/ sharedOpt<NodeTypeArray(Decorator)> decorators; // Present for use with reporting a grammar error
        Property(declarationList, VariableDeclarationList);
    };

    struct CatchClause;

    struct TryStatement: BrandKind<SyntaxKind::TryStatement, Statement> {
        Property(tryBlock, Block);
        OptionalProperty(catchClause, CatchClause);
        OptionalProperty(finallyBlock, Block);
    };

    struct CatchClause: BrandKind<SyntaxKind::CatchClause, Node> {
        shared<TryStatement> parent;
        OptionalProperty(variableDeclaration, VariableDeclaration);
        Property(block, Block);
    };

    struct VariableDeclaration: BrandKind<SyntaxKind::VariableDeclaration, NamedDeclaration, Node> {
//        shared<NodeUnion(VariableDeclarationList, CatchClause)> parent;
        shared<NodeUnion(BindingName)> name;                    // Declared variable name
        OptionalProperty(exclamationToken, ExclamationToken);  // Optional definite assignment assertion
        OptionalProperty(type, TypeNode);                      // Optional type annotation
        OptionalProperty(initializer, Expression);             // Optional initializer
    };

    struct MemberName: NodeUnion(Identifier, PrivateIdentifier) {};

    struct PropertyAccessExpression: BrandKind<SyntaxKind::PropertyAccessExpression, MemberExpression, NamedDeclaration> {
        Property(expression, LeftHandSideExpression);
        OptionalProperty(questionDotToken, QuestionDotToken);
        UnionProperty(name, Identifier, PrivateIdentifier);
    };

    using PropertyAccessChain = PropertyAccessExpression;

    struct PropertyAccessEntityNameExpression;

    struct EntityNameExpression: NodeUnion(Identifier, PropertyAccessEntityNameExpression) {};

    //seems to be needed only for JSDoc
    struct PropertyAccessEntityNameExpression: PropertyAccessExpression {
        Property(expression, EntityNameExpression);
        Property(name, Identifier);
    };

    using StringLiteralLike = NodeUnion(StringLiteral, NoSubstitutionTemplateLiteral);
    struct DeclarationName: NodeUnion(Identifier, PrivateIdentifier, StringLiteralLike, NumericLiteral, ComputedPropertyName, ElementAccessExpression, BindingPattern, EntityNameExpression) {};

    struct MetaProperty: BrandKind<SyntaxKind::MetaProperty, PrimaryExpression> {
        SyntaxKind keywordToken = SyntaxKind::NewKeyword; //: SyntaxKind.NewKeyword | SyntaxKind.ImportKeyword;
        Property(name, Identifier);
    };

    struct ObjectLiteralElement: NamedDeclaration {
        sharedOpt<NodeUnion(PropertyName)> name;
    };

    struct ClassElement: NamedDeclaration {
        sharedOpt<NodeUnion(PropertyName)> name;
    };

    struct TypeElement: NamedDeclaration {
        sharedOpt<NodeUnion(PropertyName)> name;
        sharedOpt<QuestionToken> questionToken;
    };

    struct SpreadAssignment: BrandKind<SyntaxKind::SpreadAssignment, Node> {
        Property(expression, Expression);
    };

    struct TypeLiteralNode: BrandKind<SyntaxKind::TypeLiteral, TypeNode> {
        shared<NodeTypeArray(TypeElement)> members;
    };

#define ClassLikeDeclaration ClassDeclaration, ClassExpression

    struct ShorthandPropertyAssignment: BrandKind<SyntaxKind::ShorthandPropertyAssignment, ObjectLiteralElement, Node> {
        Property(name, Identifier);
        OptionalProperty(questionToken, QuestionToken);
        OptionalProperty(exclamationToken, ExclamationToken);

        // used when ObjectLiteralExpression is used in ObjectAssignmentPattern
        // it is a grammar error to appear in actual object initializer:
        OptionalProperty(equalsToken, EqualsToken);
        OptionalProperty(objectAssignmentInitializer, Expression);
    };

//    struct VariableDeclaration: BrandKind<SyntaxKind::VariableDeclaration, NamedDeclaration, Node> {
//        Property(name, BindingNameNode);                    // Declared variable name
////            readonly kind: SyntaxKind.VariableDeclaration;
////            readonly parent: VariableDeclarationList | CatchClause;
//        optional <NodeType<ExclamationToken>> exclamationToken;  // Optional definite assignment assertion
//        optional <TypeNode> type; // Optional type annotation
//        optional <NodeType<Expression>> initializer; // Optional initializer
//    };

    struct TypeParameterDeclaration: BrandKind<SyntaxKind::TypeParameter, NamedDeclaration, Statement> {
        inline static auto kind = SyntaxKind::TypeParameter;
//        BaseNode *parent; //: DeclarationWithTypeParameterChildren | InferTypeNode;
        shared<Identifier> name;
        /** Note: Consider calling `getEffectiveConstraintOfTypeParameter` */
        Property(constraint, TypeNode);
        Property(defaultType, TypeNode);

        // For error recovery purposes.
        OptionalProperty(expression, Expression);
    };

    struct ParameterDeclaration: BrandKind<SyntaxKind::Parameter, Node> {
        UnionProperty(name, BindingName);
        OptionalProperty(dotDotDotToken, DotDotDotToken);
        OptionalProperty(questionToken, QuestionToken);
        OptionalProperty(type, TypeNode);
        OptionalProperty(initializer, Expression);
    };

    struct PropertyDeclaration: BrandKind<SyntaxKind::PropertyDeclaration, ClassElement, Node> {
        OptionalProperty(dotDotDotToken, DotDotDotToken);
        shared<NodeUnion(BindingName)> name;
        OptionalProperty(questionToken, QuestionToken);
        OptionalProperty(exclamationToken, ExclamationToken);
        OptionalProperty(type, TypeNode);
        OptionalProperty(initializer, Expression);
    };

    struct SignatureDeclarationBase: NamedDeclaration {
        OptionalUnionProperty(name, PropertyName);
        sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters;
        shared<NodeTypeArray(ParameterDeclaration)> parameters;
        OptionalProperty(type, TypeNode);
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
    };

    struct PropertyAssignment: BrandKind<SyntaxKind::PropertyAssignment, ObjectLiteralElement, Node> {
        shared<NodeUnion(PropertyName)> name;
        OptionalProperty(questionToken, QuestionToken);
        OptionalProperty(exclamationToken, ExclamationToken);
        OptionalProperty(initializer, Expression);
    };

    struct FunctionLikeDeclarationBase: SignatureDeclarationBase {
        OptionalProperty(asteriskToken, AsteriskToken);
        OptionalProperty(questionToken, QuestionToken);
        OptionalProperty(exclamationToken, ExclamationToken);
        OptionalUnionProperty(body, Block, Expression);

//        /* @internal */ optional<NodeType<FlowNode>> endFlowNode;
//        /* @internal */ optional<NodeType<FlowNode>> returnFlowNode;
    };

#define FunctionBody Block
#define ConciseBody FunctionBody, Expression

    struct ArrowFunction: BrandKind<SyntaxKind::ArrowFunction, Expression, FunctionLikeDeclarationBase> {
        Property(equalsGreaterThanToken, EqualsGreaterThanToken);
        UnionProperty(body, ConciseBody);
    };

    struct HeritageClause;

    struct ClassLikeDeclarationBase {
        OptionalProperty(name, Identifier);
        sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters;
        sharedOpt<NodeTypeArray(HeritageClause)> heritageClauses;
        shared<NodeTypeArray(ClassElement)> members;
    };

    struct DeclarationStatement: Statement {
//        sharedOpt<NodeTypeArray(Decorator)> decorators;           // Array of decorators (in document order)
//        sharedOpt<NodeTypeArray(Modifier)> modifiers;            // Array of modifiers
        OptionalUnionProperty(name, Identifier, StringLiteral, NumericLiteral);
    };

    struct EmptyStatement: BrandKind<SyntaxKind::EmptyStatement, Statement> {};

    struct DebuggerStatement: BrandKind<SyntaxKind::DebuggerStatement, Statement> {};

    struct CommaListExpression: BrandKind<SyntaxKind::CommaListExpression, Expression> {
        shared<NodeTypeArray(Expression)> elements;
    };

    struct MissingDeclaration: BrandKind<SyntaxKind::MissingDeclaration, DeclarationStatement> {
        OptionalProperty(name, Identifier);
    };

    struct ClassDeclaration: BrandKind<SyntaxKind::ClassDeclaration, ClassLikeDeclarationBase, DeclarationStatement> {
//        sharedOpt<NodeTypeArray(Decorator)> decorators;           // Array of decorators (in document order)
//        sharedOpt<NodeTypeArray(Modifier)> modifiers;            // Array of modifiers
        OptionalProperty(name, Identifier);
    };

    struct ClassExpression: BrandKind<SyntaxKind::ClassExpression, ClassLikeDeclarationBase, PrimaryExpression> {
        sharedOpt<NodeTypeArray(Decorator)> decorators;           // Array of decorators (in document order)
        sharedOpt<NodeTypeArray(Modifier)> modifiers;            // Array of modifiers
    };

    struct HeritageClause;

    struct InterfaceDeclaration: BrandKind<SyntaxKind::InterfaceDeclaration, DeclarationStatement> {
        Property(name, Identifier);
        sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters;
        sharedOpt<NodeTypeArray(HeritageClause)> heritageClauses;
        shared<NodeTypeArray(TypeElement)> members;
    };

    struct NodeWithTypeArguments {
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
    };

    struct ExpressionWithTypeArguments: BrandKind<SyntaxKind::ExpressionWithTypeArguments, MemberExpression, NodeWithTypeArguments> {
        Property(expression, LeftHandSideExpression);
    };

    struct HeritageClause: BrandKind<SyntaxKind::HeritageClause, Node> {
        ParentProperty(InterfaceDeclaration, ClassLikeDeclaration);
        SyntaxKind token; //SyntaxKind.ExtendsKeyword | SyntaxKind.ImplementsKeyword
        shared<NodeTypeArray(ExpressionWithTypeArguments)> types;
    };

    struct TypeAliasDeclaration: BrandKind<SyntaxKind::TypeAliasDeclaration, DeclarationStatement> {
        Property(name, Identifier);
        sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters;
        Property(type, TypeNode);
    };

    struct EnumMember;

    struct EnumDeclaration: BrandKind<SyntaxKind::EnumDeclaration, DeclarationStatement> {
        Property(name, Identifier);
        shared<NodeTypeArray(EnumMember)> members;
    };

    struct EnumMember: BrandKind<SyntaxKind::EnumMember, NamedDeclaration, Node> {
        Property(parent, EnumDeclaration);
        // This does include ComputedPropertyName, but the parser will give an error
        // if it parses a ComputedPropertyName in an EnumMember
        UnionProperty(name, PropertyName);
        OptionalProperty(initializer, Expression);
    };

    struct ClassStaticBlockDeclaration: BrandKind<SyntaxKind::ClassStaticBlockDeclaration, ClassElement, Statement> {
        ParentProperty(ClassDeclaration, ClassExpression);
        Property(body, Block);
//        /* @internal */ endFlowNode?: FlowNode;
//        /* @internal */ returnFlowNode?: FlowNode;
    };

    struct PropertySignature: BrandKind<SyntaxKind::PropertySignature, TypeElement, Node> {
        UnionProperty(name, PropertyName);
        OptionalProperty(type, TypeNode);
        OptionalProperty(initializer, Expression);
    };

    struct TypeReferenceNode: BrandKind<SyntaxKind::TypeReference, NodeWithTypeArguments, TypeNode> {
        UnionProperty(typeName, EntityName);
    };

#define ModuleName Identifier, StringLiteral
#define NamespaceBody ModuleBlock, NamespaceDeclaration
#define ModuleBody NamespaceBody

    struct ModuleBlock;
    struct NamespaceDeclaration;

    struct ModuleDeclaration: BrandKind<SyntaxKind::ModuleDeclaration, DeclarationStatement> {
        ParentProperty(ModuleBlock, NamespaceDeclaration, SourceFile);
        UnionProperty(name, ModuleName);
        OptionalUnionProperty(body, ModuleBody);
    };

    struct ModuleBlock: BrandKind<SyntaxKind::ModuleBlock, Statement> {
        shared<ModuleDeclaration> parent;
        shared<NodeTypeArray(Statement)> statements;
    };

    struct NamespaceDeclaration: BrandKind<SyntaxKind::ModuleDeclaration, DeclarationStatement> {
        ParentProperty(ModuleBody, SourceFile);
        Property(name, Identifier);
        OptionalUnionProperty(body, ModuleBody);
    };

    struct ExternalModuleReference;

#define ModuleReference EntityName, ExternalModuleReference

    /**
     * One of:
     * - import x = require("mod");
     * - import x = M.x;
     */
    struct ImportEqualsDeclaration: BrandKind<SyntaxKind::ImportEqualsDeclaration, DeclarationStatement> {
        ParentProperty(SourceFile, ModuleBlock);
        Property(name, Identifier);
        bool isTypeOnly;

        // 'EntityName' for an internal module reference, 'ExternalModuleReference' for an external
        // module reference.
        sharedOpt<NodeUnion(ModuleReference)> moduleReference;
    };

    struct ExternalModuleReference: BrandKind<SyntaxKind::ImportEqualsDeclaration, Node> {
        Property(parent, ImportEqualsDeclaration);
        Property(expression, Expression);
    };

    struct ImportDeclaration;

    struct ImportClause;
    struct NamespaceImport: BrandKind<SyntaxKind::NamespaceImport, NamedDeclaration, Node> {
        shared<ImportClause> parent;
        Property(name, Identifier);
    };

    struct NamedImports;
    struct ImportSpecifier: BrandKind<SyntaxKind::ImportSpecifier, NamedDeclaration, Node> {
        shared<NamedImports> parent;
        OptionalProperty(propertyName, Identifier);  // Name preceding "as" keyword (or undefined when "as" is absent)
        Property(name, Identifier);           // Declared name
        bool isTypeOnly;
    };

    struct NamedImports: BrandKind<SyntaxKind::NamedImports, Node> {
        shared<ImportClause> parent;
        shared<NodeTypeArray(ImportSpecifier)> elements;
    };

#define NamedImportBindings NamespaceImport, NamedImports
#define NamedExportBindings NamespaceExport, NamedExports

#define AssertionKey Identifier, StringLiteral

    struct AssertClause;
    struct AssertEntry: BrandKind<SyntaxKind::AssertEntry, Node> {
        shared<AssertClause> parent;
        UnionProperty(name, AssertionKey);
        Property(value, Expression);
    };

    struct ExportDeclaration;
    struct AssertClause: BrandKind<SyntaxKind::AssertClause, Node> {
        ParentProperty(ImportDeclaration, ExportDeclaration);
        shared<NodeTypeArray(AssertEntry)> elements;
        bool multiLine;
    };

    // In case of:
    // import d from "mod" => name = d, namedBinding = undefined
    // import * as ns from "mod" => name = undefined, namedBinding: NamespaceImport = { name: ns }
    // import d, * as ns from "mod" => name = d, namedBinding: NamespaceImport = { name: ns }
    // import { a, b as x } from "mod" => name = undefined, namedBinding: NamedImports = { elements: [{ name: a }, { name: x, propertyName: b}]}
    // import d, { a, b as x } from "mod" => name = d, namedBinding: NamedImports = { elements: [{ name: a }, { name: x, propertyName: b}]}
    struct ImportClause: BrandKind<SyntaxKind::ImportClause, NamedDeclaration, Node> {
        ParentProperty(ImportDeclaration);
        bool isTypeOnly;
        OptionalProperty(name, Identifier); // Default binding
        OptionalUnionProperty(namedBindings, NamedImportBindings);
    };

    // In case of:
    // import "mod"  => importClause = undefined, moduleSpecifier = "mod"
    // In rest of the cases, module specifier is string literal corresponding to module
    // ImportClause information is shown at its declaration below.
    struct ImportDeclaration: BrandKind<SyntaxKind::ImportDeclaration, Statement> {
        ParentProperty(SourceFile, ModuleBlock);
        OptionalProperty(importClause, ImportClause);
        /** If this is not a StringLiteral it will be a grammar error. */
        Property(moduleSpecifier, Expression);
        OptionalProperty(assertClause, AssertClause);
    };

    struct NamespaceExport: BrandKind<SyntaxKind::NamespaceExport, NamedDeclaration, Node> {
        shared<ExportDeclaration> parent;
        Property(name, Identifier);
    };

    struct NamedExports;
    struct ExportSpecifier: BrandKind<SyntaxKind::ExportSpecifier, NamedDeclaration, Node> {
        shared<NamedExports> parent;
        bool isTypeOnly;
        OptionalProperty(propertyName, Identifier);  // Name preceding "as" keyword (or undefined when "as" is absent)
        Property(name, Identifier);           // Declared name
    };

    struct NamedExports: BrandKind<SyntaxKind::NamedExports, Node> {
        shared<ExportDeclaration> parent;
        shared<NodeTypeArray(ExportSpecifier)> elements;
    };

    struct NamespaceExportDeclaration: BrandKind<SyntaxKind::NamespaceExportDeclaration, DeclarationStatement> {
        Property(name, Identifier);
        /* @internal */ sharedOpt<NodeTypeArray(Decorator)> decorators; // Present for use with reporting a grammar error
        /* @internal */ OptionalProperty(modifiers, ModifiersArray); // Present for use with reporting a grammar error
    };

    struct ExportDeclaration: BrandKind<SyntaxKind::ExportDeclaration, DeclarationStatement> {
        ParentProperty(SourceFile, ModuleBlock);
        bool isTypeOnly;
        /** Will not be assigned in the case of `export * from "foo";` */
        OptionalUnionProperty(exportClause, NamespaceExport, NamedExports);
        /** If this is not a StringLiteral it will be a grammar error. */
        OptionalProperty(moduleSpecifier, Expression);
        OptionalProperty(assertClause, AssertClause);
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
        shared<SourceFile> parent;
        bool isExportEquals;
        Property(expression, Expression);
    };

    struct ImportTypeNode;

    struct ImportTypeAssertionContainer: BrandKind<SyntaxKind::ImportTypeAssertionContainer, Node> {
        shared<ImportTypeNode> parent;
        Property(assertClause, AssertClause);
        bool multiLine = false;
    };

    struct ImportTypeNode: BrandKind<SyntaxKind::ImportType, NodeWithTypeArguments, TypeNode> {
        bool isTypeOf;
        Property(argument, TypeNode);
        OptionalProperty(assertions, ImportTypeAssertionContainer);
        OptionalUnionProperty(qualifier, EntityName);
    };

    struct ThisTypeNode: BrandKind<SyntaxKind::ThisType, TypeNode> {};

    struct CallSignatureDeclaration: BrandKind<SyntaxKind::CallSignature, SignatureDeclarationBase, TypeElement, Node> {
        using TypeElement::name;
    };
    struct ConstructSignatureDeclaration: BrandKind<SyntaxKind::ConstructSignature, SignatureDeclarationBase, TypeElement, Node> {
        using TypeElement::name;
    };

#define ObjectTypeDeclaration ClassLikeDeclaration, InterfaceDeclaration, TypeLiteralNode

    struct MethodSignature: BrandKind<SyntaxKind::MethodSignature, SignatureDeclarationBase, TypeElement, Node> {
        shared<NodeUnion(ObjectTypeDeclaration)> parent;
        UnionProperty(name, PropertyName);
    };

    struct IndexSignatureDeclaration: BrandKind<SyntaxKind::IndexSignature, SignatureDeclarationBase, ClassElement, TypeElement, Node> {
        using TypeElement::name;
        shared<NodeUnion(ObjectTypeDeclaration)> parent;
//        sharedOpt<NodeTypeArray(Decorator)> decorators;           // Array of decorators (in document order)
//        sharedOpt<NodeTypeArray(Modifier)> modifiers;            // Array of modifiers
        Property(type, TypeNode);
    };

    struct FunctionOrConstructorTypeNodeBase: TypeNode, SignatureDeclarationBase {
        Property(type, TypeNode);
    };

    struct FunctionTypeNode: BrandKind<SyntaxKind::FunctionType, FunctionOrConstructorTypeNodeBase> {};

    struct ConstructorTypeNode: BrandKind<SyntaxKind::ConstructorType, FunctionOrConstructorTypeNodeBase> {};

    struct FunctionDeclaration: BrandKind<SyntaxKind::FunctionDeclaration, FunctionLikeDeclarationBase, DeclarationStatement> {
        OptionalProperty(name, Identifier);
        OptionalProperty(body, FunctionBody);
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
    struct MethodDeclaration: BrandKind<SyntaxKind::MethodDeclaration, FunctionLikeDeclarationBase, ClassElement, ObjectLiteralElement, Node> {
        ParentProperty(ClassLikeDeclaration, ObjectLiteralExpression);
        UnionProperty(name, PropertyName);
        UnionProperty(body, FunctionBody);
        /* @internal*/ OptionalProperty(exclamationToken, ExclamationToken); // Present for use with reporting a grammar error
    };

    struct ConstructorDeclaration: BrandKind<SyntaxKind::Constructor, FunctionLikeDeclarationBase, ClassElement, Node> {
        using ClassElement::name;
        shared<NodeUnion(ClassLikeDeclaration)> parent;
        OptionalProperty(body, FunctionBody);

        /* @internal */ sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters; // Present for use with reporting a grammar error
        /* @internal */ OptionalProperty(type, TypeNode); // Present for use with reporting a grammar error
    };

    struct SemicolonClassElement: BrandKind<SyntaxKind::SemicolonClassElement, ClassElement, Node> {};

    // See the comment on MethodDeclaration for the intuition behind GetAccessorDeclaration being a
    // ClassElement and an ObjectLiteralElement.
    struct GetAccessorDeclaration: BrandKind<SyntaxKind::GetAccessor, FunctionLikeDeclarationBase, ClassElement, TypeElement, ObjectLiteralElement, Node> {
        ParentProperty(ClassLikeDeclaration, ObjectLiteralExpression, TypeLiteralNode, InterfaceDeclaration);
        UnionProperty(name, PropertyName);
        OptionalProperty(body, FunctionBody);
        /* @internal */ sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters; // Present for use with reporting a grammar error
    };

    // See the comment on MethodDeclaration for the intuition behind SetAccessorDeclaration being a
    // ClassElement and an ObjectLiteralElement.
    struct SetAccessorDeclaration: BrandKind<SyntaxKind::SetAccessor, FunctionLikeDeclarationBase, ClassElement, TypeElement, ObjectLiteralElement, Node> {
        ParentProperty(ClassLikeDeclaration, ObjectLiteralExpression, TypeLiteralNode, InterfaceDeclaration);
        UnionProperty(name, PropertyName);
        OptionalUnionProperty(body, FunctionBody);
        /* @internal */ sharedOpt<NodeTypeArray(TypeParameterDeclaration)> typeParameters; // Present for use with reporting a grammar error
        /* @internal */ OptionalProperty(type, TypeNode); // Present for use with reporting a grammar error
    };

#define AccessorDeclaration GetAccessorDeclaration, SetAccessorDeclaration

    using ObjectLiteralElementLike = NodeUnion(PropertyAssignment, ShorthandPropertyAssignment, SpreadAssignment, MethodDeclaration, AccessorDeclaration);

    template<class T>
    struct ObjectLiteralExpressionBase: PrimaryExpression {
        shared<NodeTypeArray(T)> properties;
    };

    struct ObjectLiteralExpression: BrandKind<SyntaxKind::ObjectLiteralExpression, ObjectLiteralExpressionBase<ObjectLiteralElementLike>> {
        /* @internal */ bool multiLine;
    };

    struct BinaryExpression: BrandKind<SyntaxKind::BinaryExpression, Expression> {
        Property(left, Expression);
        UnionProperty(operatorToken, Node); //BinaryOperatorToken uses a lot of different NodeType<T>
        Property(right, Expression);
    };

//    using AssignmentOperatorToken = Token<AssignmentOperator>;

    struct AssignmentExpression: BinaryExpression {
        Property(left, LeftHandSideExpression);
    };

    struct ObjectDestructuringAssignment: BinaryExpression {
        Property(left, ObjectLiteralExpression);
        Property(operatorToken, EqualsToken);
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

    struct ConditionalExpression: BrandKind<SyntaxKind::ConditionalExpression, Expression> {
        Property(condition, Expression);
        Property(questionToken, QuestionToken);
        Property(whenTrue, Expression);
        Property(colonToken, ColonToken);
        Property(whenFalse, Expression);
    };

    struct FunctionExpression: BrandKind<SyntaxKind::FunctionExpression, PrimaryExpression, FunctionLikeDeclarationBase> {
        OptionalProperty(name, Identifier);
        OptionalProperty(body, FunctionBody); // Required, whereas the member inherited from FunctionDeclaration is optional
    };

//    struct SignatureDeclaration: NodeType<CallSignatureDeclaration, ConstructSignatureDeclaration, MethodSignature, IndexSignatureDeclaration, FunctionTypeNode, ConstructorTypeNode, JSDocFunctionType, FunctionDeclaration, MethodDeclaration, ConstructorDeclaration, AccessorDeclaration, FunctionExpression, ArrowFunction> {};

    struct TypePredicateNode: BrandKind<SyntaxKind::TypePredicate, TypeNode> {
        ParentProperty(CallSignatureDeclaration, ConstructSignatureDeclaration, MethodSignature, IndexSignatureDeclaration, FunctionTypeNode, ConstructorTypeNode, FunctionDeclaration, MethodDeclaration, ConstructorDeclaration, AccessorDeclaration, FunctionExpression, ArrowFunction);
        OptionalProperty(assertsModifier, AssertsKeyword);
        UnionProperty(parameterName, Identifier, ThisTypeNode);
        OptionalProperty(type, TypeNode);
    };

    struct ArrayLiteralExpression: BrandKind<SyntaxKind::ArrayLiteralExpression, PrimaryExpression> {
        shared<NodeTypeArray(Expression)> elements;
        /* @internal */
        bool multiLine; //optional
    };

    struct ArrayDestructuringAssignment: BinaryExpression {
        Property(left, ArrayLiteralExpression);
        Property(operatorToken, EqualsToken);
    };

    struct CallExpression: BrandKind<SyntaxKind::CallExpression, LeftHandSideExpression> {
        Property(expression, LeftHandSideExpression);
        OptionalProperty(questionDotToken, QuestionDotToken);
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
        shared<NodeTypeArray(Expression)> arguments;
    };

    using CallChain = CallExpression;
    using OuterExpression = Expression; // ParenthesizedExpression | TypeAssertion | AsExpression | NonNullExpression | PartiallyEmittedExpression;

/* @internal */
    struct CallChainRoot: CallChain {};

    struct NewExpression: BrandKind<SyntaxKind::NewExpression, PrimaryExpression> {
        Property(expression, LeftHandSideExpression);
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
        sharedOpt<NodeTypeArray(Expression)> arguments;
    };

    struct TypeAssertion: BrandKind<SyntaxKind::TypeAssertionExpression, UnaryExpression> {
        Property(type, TypeNode);
        Property(expression, UnaryExpression);
    };

    struct TemplateExpression;
    struct TemplateLiteralTypeNode;
    struct TemplateSpan;
    struct TemplateLiteralTypeSpan;

    struct TemplateHead: BrandKind<SyntaxKind::TemplateHead, TemplateLiteralLike> {
        ParentProperty(TemplateExpression, TemplateLiteralTypeNode);
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateMiddle: BrandKind<SyntaxKind::TemplateMiddle, TemplateLiteralLike> {
        shared<NodeUnion(TemplateSpan, TemplateLiteralTypeSpan)> parent;
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateTail: BrandKind<SyntaxKind::TemplateTail, TemplateLiteralLike> {
        ParentProperty(TemplateSpan, TemplateLiteralTypeSpan);
        /* @internal */
        optional<types::TokenFlags> templateFlags;
    };

    struct TemplateLiteralTypeSpan: BrandKind<SyntaxKind::TemplateLiteralTypeSpan, TypeNode> {
        shared<TemplateLiteralTypeNode> parent;
        Property(type, TypeNode);
        UnionProperty(literal, TemplateMiddle, TemplateTail);
    };

    struct TemplateLiteralTypeNode: BrandKind<SyntaxKind::TemplateLiteralType, TypeNode> {
        Property(head, TemplateHead);
        shared<NodeTypeArray(TemplateLiteralTypeSpan)> templateSpans;
    };

    struct TemplateExpression: BrandKind<SyntaxKind::TemplateExpression, PrimaryExpression> {
        Property(head, TemplateHead);
        shared<NodeTypeArray(TemplateSpan)> templateSpans;
    };

#define TemplateLiteral TemplateExpression, NoSubstitutionTemplateLiteral

    struct TaggedTemplateExpression: BrandKind<SyntaxKind::TaggedTemplateExpression, MemberExpression> {
        Property(tag, LeftHandSideExpression);
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
        UnionProperty(templateLiteral, TemplateLiteral);
        /*@internal*/ OptionalProperty(questionDotToken, QuestionDotToken); // NOTE: Invalid syntax, only used to report a grammar error.
    };

    struct TemplateSpan: BrandKind<SyntaxKind::TemplateSpan, Node> {
        Property(parent, TemplateExpression);
        Property(expression, Expression);
        UnionProperty(literal, TemplateMiddle, TemplateTail);
    };

    struct AsExpression: BrandKind<SyntaxKind::AsExpression, Expression> {
        Property(expression, Expression);
        Property(type, TypeNode);
    };

    struct NonNullExpression: BrandKind<SyntaxKind::NonNullExpression, LeftHandSideExpression> {
        Property(expression, Expression);
    };

    using NonNullChain = NonNullExpression;

    struct ParenthesizedExpression: BrandKind<SyntaxKind::ParenthesizedExpression, PrimaryExpression> {
        Property(expression, Expression);
    };

    struct SpreadElement: BrandKind<SyntaxKind::SpreadElement, Expression> {
        ParentProperty(ArrayLiteralExpression, CallExpression, NewExpression);
        Property(expression, Expression);
    };

    struct TypeQueryNode: BrandKind<SyntaxKind::TypeQuery, NodeWithTypeArguments, TypeNode> {
        UnionProperty(exprName, EntityName);
    };

    struct ArrayTypeNode: BrandKind<SyntaxKind::ArrayType, TypeNode> {
        Property(elementType, TypeNode);
    };

    struct NamedTupleMember: BrandKind<SyntaxKind::NamedTupleMember, TypeNode> {
        OptionalProperty(dotDotDotToken, DotDotDotToken);
        Property(name, Identifier);
        OptionalProperty(questionToken, QuestionToken);
        Property(type, TypeNode);
    };

    struct OptionalTypeNode: BrandKind<SyntaxKind::OptionalType, TypeNode> {
        Property(type, TypeNode);
    };

    struct RestTypeNode: BrandKind<SyntaxKind::RestType, TypeNode> {
        Property(type, TypeNode);
    };

    struct UnionTypeNode: BrandKind<SyntaxKind::UnionType, TypeNode> {
        shared<NodeTypeArray(TypeNode)> types;
    };

    struct IntersectionTypeNode: BrandKind<SyntaxKind::IntersectionType, TypeNode> {
        shared<NodeTypeArray(TypeNode)> types;
    };

#define UnionOrIntersectionTypeNode UnionTypeNode, IntersectionTypeNode

    struct ConditionalTypeNode: BrandKind<SyntaxKind::ConditionalType, TypeNode> {
        Property(checkType, TypeNode);
        Property(extendsType, TypeNode);
        Property(trueType, TypeNode);
        Property(falseType, TypeNode);
    };

    struct InferTypeNode: BrandKind<SyntaxKind::InferType, TypeNode> {
        Property(typeParameter, TypeParameterDeclaration);
    };

    struct ParenthesizedTypeNode: BrandKind<SyntaxKind::ParenthesizedType, TypeNode> {
        Property(type, TypeNode);
    };

    struct TypeOperatorNode: BrandKind<SyntaxKind::TypeOperator, TypeNode> {
        SyntaxKind operatorKind;
        Property(type, TypeNode);
    };

/* @internal */
    struct UniqueTypeOperatorNode: TypeOperatorNode {
        SyntaxKind operatorKind = SyntaxKind::UniqueKeyword;
    };

    struct IndexedAccessTypeNode: BrandKind<SyntaxKind::IndexedAccessType, TypeNode> {
        Property(objectType, TypeNode);
        Property(indexType, TypeNode);
    };

    struct MappedTypeNode: BrandKind<SyntaxKind::MappedType, TypeNode> {
        OptionalUnionProperty(readonlyToken, ReadonlyKeyword, PlusToken, MinusToken);
        Property(typeParameter, TypeParameterDeclaration);
        OptionalProperty(nameType, TypeNode);
        OptionalUnionProperty(questionToken, QuestionToken, PlusToken, MinusToken);
        OptionalProperty(type, TypeNode);
        /** Used only to produce grammar errors */
        sharedOpt<NodeTypeArray(TypeElement)> members;
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
        UnionProperty(expression, JsxTagNameExpression);
    };

    struct JsxAttribute;
    struct JsxSpreadAttribute;
    struct JsxFragment;

    struct JsxExpression: BrandKind<SyntaxKind::JsxExpression, Expression> {
        ParentProperty(JsxElement, JsxFragment, JsxAttributeLike);
        OptionalProperty(dotDotDotToken, DotDotDotToken);
        OptionalProperty(expression, Expression);
    };

    struct JsxAttribute: BrandKind<SyntaxKind::JsxAttribute, ObjectLiteralElement, Node> {
        shared<JsxAttributes> parent;
        Property(name, Identifier);
        /// JSX attribute initializers are optional; <X y /> is sugar for <X y={true} />
        OptionalUnionProperty(initializer, JsxAttributeValue);
    };

    struct JsxAttributes: BrandKind<SyntaxKind::JsxAttributes, PrimaryExpression> {
        OptionalUnionProperty(parent, JsxOpeningLikeElement);
        shared<NodeTypeArray(JsxAttributeLike)> properties;
    };

    // The opening element of a <Tag>...</Tag> JsxElement
    struct JsxOpeningElement: BrandKind<SyntaxKind::JsxOpeningElement, Expression> {
        shared<JsxElement> parent;
        UnionProperty(tagName, JsxTagNameExpression);
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
        Property(attributes, JsxAttributes);
    };

    struct JsxText: BrandKind<SyntaxKind::JsxText, LiteralLike> {
        shared<NodeUnion(JsxElement, JsxFragment)> parent;
        bool containsOnlyTriviaWhiteSpaces;
    };

    struct JsxClosingElement: BrandKind<SyntaxKind::JsxClosingElement, Node> {
        shared<JsxElement> parent;
        UnionProperty(tagName, JsxTagNameExpression);
    };

    /// A JSX expression of the form <TagName attrs>...</TagName>
    struct JsxElement: BrandKind<SyntaxKind::JsxElement, PrimaryExpression> {
        Property(openingElement, JsxOpeningElement);
        shared<NodeTypeArray(JsxChild)> children;
        Property(closingElement, JsxClosingElement);
    };

    struct JsxAttribute;
    struct JsxFragment;
    struct JsxSpreadAttribute;
    struct JsxClosingFragment;

    // A JSX expression of the form <TagName attrs />
    struct JsxSelfClosingElement: BrandKind<SyntaxKind::JsxSelfClosingElement, PrimaryExpression> {
        UnionProperty(tagName, JsxTagNameExpression);
        sharedOpt<NodeTypeArray(TypeNode)> typeArguments;
        Property(attributes, JsxAttributes);
    };

    struct JsxFragment;
    /// The opening element of a <>...</> JsxFragment
    struct JsxOpeningFragment: BrandKind<SyntaxKind::JsxOpeningFragment, Expression> {
        shared<JsxFragment> parent;
    };

    /// The closing element of a <>...</> JsxFragment
    struct JsxClosingFragment: BrandKind<SyntaxKind::JsxClosingFragment, Expression> {
        shared<JsxFragment> parent;
    };

    /// A JSX expression of the form <>...</>
    struct JsxFragment: BrandKind<SyntaxKind::JsxFragment, PrimaryExpression> {
        Property(openingFragment, JsxOpeningFragment);
        shared<NodeTypeArray(JsxChild)> children;
        Property(closingFragment, JsxClosingFragment);
    };

    struct JsxSpreadAttribute: BrandKind<SyntaxKind::JsxSpreadAttribute, ObjectLiteralElement, Node> {
        Property(parent, JsxAttributes);
        Property(expression, Expression);
    };

    struct LiteralTypeNode: BrandKind<SyntaxKind::LiteralType, TypeNode> {
        UnionProperty(literal, NullLiteral, BooleanLiteral, LiteralExpression, PrefixUnaryExpression);
    };

    struct TupleTypeNode: BrandKind<SyntaxKind::TupleType, TypeNode> {
        shared<NodeTypeArray(TypeNode, NamedTupleMember)> elements;
    };

//    using AccessibilityModifier = NodeType<PublicKeyword, PrivateKeyword, ProtectedKeyword>;
//    using ParameterPropertyModifier = NodeType<AccessibilityModifier, ReadonlyKeyword>;
//    using ClassMemberModifier = NodeType<AccessibilityModifier, ReadonlyKeyword, StaticKeyword>;

    struct Decorator: BrandKind<SyntaxKind::Decorator, Node> {
        Property(parent, NamedDeclaration);
        Property(expression, Expression);
    };

    struct EndOfFileToken: Token<SyntaxKind::EndOfFileToken> {};

    struct SourceFile: BrandKind<SyntaxKind::SourceFile, Node> {
        string fileName;
        string text;

        shared<NodeTypeArray(Statement)> statements;
        Property(endOfFileToken, EndOfFileToken);

        /* @internal */ string path;
        /** Resolved path can be different from path property,
         * when file is included through project reference is mapped to its output instead of source
         * in that case resolvedPath = path to output file
         * path = input file's path
         */
        /* @internal */ string resolvedPath;

        /** Original file name that can be different from fileName,
         * when file is included through project reference is mapped to its output instead of source
         * in that case originalFileName = name of input file
         * fileName = output file's name
         */
        /* @internal */ string originalFileName;

//        amdDependencies: readonly AmdDependency[];
        optional<string> moduleName;
//        referencedFiles: readonly FileReference[];
//        typeReferenceDirectives: readonly FileReference[];
//        libReferenceDirectives: readonly FileReference[];
        types::LanguageVariant languageVariant;
        bool isDeclarationFile;
//
//        // this map is used by transpiler to supply alternative names for dependencies (i.e. in case of bundling)
//        /* @internal */
//        renamedDependencies?: ReadonlyESMap<string, string>;
//
        /**
         * lib.d.ts should have a reference comment like
         *
         *  /// <reference no-default-lib="true"/>
         *
         * If any other file has this comment, it signals not to include lib.d.ts
         * because this containing file is intended to act as a default library.
         */
        bool hasNoDefaultLib;

        types::ScriptTarget languageVersion;

        /**
         * When `module` is `Node16` or `NodeNext`, this field controls whether the
         * source file in question is an ESNext-output-format file, or a CommonJS-output-format
         * module. This is derived by the module resolver as it looks up the file, since
         * it is derived from either the file extension of the module, or the containing
         * `package.json` context, and affects both checking and emit.
         *
         * It is _public_ so that (pre)transformers can set this field,
         * since it switches the builtin `node` module transform. Generally speaking, if unset,
         * the field is treated as though it is `ModuleKind.CommonJS`.
         */
        optional<types::ModuleKind> impliedNodeFormat; //?: ModuleKind.ESNext | ModuleKind.CommonJS;

        /* @internal */ types::ScriptKind scriptKind;

        /**
         * The first "most obvious" node that makes a file an external module.
         * This is intended to be the first top-level import/export,
         * but could be arbitrarily nested (e.g. `import.meta`).
         */
        /* @internal */ sharedOpt<Node> externalModuleIndicator; //?: Node | true;

        /**
         * The callback used to set the external module indicator - this is saved to
         * be later reused during incremental reparsing, which otherwise lacks the information
         * to set this field
         */
        /* @internal */ optional<function<void(shared<SourceFile>)>> setExternalModuleIndicator; //?: (file: SourceFile) => void;
//        // The first node that causes this file to be a CommonJS module
//        /* @internal */ commonJsModuleIndicator?: Node;
//        // JS identifier-declarations that are intended to merge with globals
//        /* @internal */ jsGlobalAugmentations?: SymbolTable;
//
//        /* @internal */ identifiers: ESMap<string, string>; // Map from a string to an interned string
//        /* @internal */ nodeCount: number;
//        /* @internal */ identifierCount: number;
//        /* @internal */ symbolCount: number;
//
//        // File-level diagnostics reported by the parser (includes diagnostics about /// references
//        // as well as code diagnostics).
//        /* @internal */ parseDiagnostics: DiagnosticWithLocation[];
//
//        // File-level diagnostics reported by the binder.
//        /* @internal */ bindDiagnostics: DiagnosticWithLocation[];
//        /* @internal */ bindSuggestionDiagnostics?: DiagnosticWithLocation[];
//
//        // File-level JSDoc diagnostics reported by the JSDoc parser
//        /* @internal */ jsDocDiagnostics?: DiagnosticWithLocation[];
//
//        // Stores additional file-level diagnostics reported by the program
//        /* @internal */ additionalSyntacticDiagnostics?: readonly DiagnosticWithLocation[];
//
//        // Stores a line map for the file.
//        // This field should never be used directly to obtain line map, use getLineMap function instead.
//        /* @internal */ lineMap: readonly number[];
//        /* @internal */ classifiableNames?: ReadonlySet<__String>;
//        // Comments containing @ts-* directives, in order.
//        /* @internal */ commentDirectives?: CommentDirective[];
//        // Stores a mapping 'external module reference text' -> 'resolved file name' | undefined
//        // It is used to resolve module names in the checker.
//        // Content of this field should never be used directly - use getResolvedModuleFileName/setResolvedModuleFileName functions instead
//        /* @internal */ resolvedModules?: ModeAwareCache<ResolvedModuleFull | undefined>;
//        /* @internal */ resolvedTypeReferenceDirectiveNames: ModeAwareCache<ResolvedTypeReferenceDirective | undefined>;
//        /* @internal */ imports: readonly StringLiteralLike[];
//        // Identifier only if `declare global`
//        /* @internal */ moduleAugmentations: readonly (StringLiteral | Identifier)[];
//        /* @internal */ patternAmbientModules?: PatternAmbientModule[];
//        /* @internal */ ambientModuleNames: readonly string[];
//        /* @internal */ checkJsDirective?: CheckJsDirective;
//        /* @internal */ version: string;
//        /* @internal */ pragmas: ReadonlyPragmaMap;
//        /* @internal */ localJsxNamespace?: __String;
//        /* @internal */ localJsxFragmentNamespace?: __String;
//        /* @internal */ localJsxFactory?: EntityName;
//        /* @internal */ localJsxFragmentFactory?: EntityName;
//
//        /* @internal */ exportedModulesFromDeclarationEmit?: ExportedModulesFromDeclarationEmit;
//        /* @internal */ endFlowNode?: FlowNode;

    };
}

template<>
struct fmt::formatter<ts::types::SyntaxKind> {
//    // Presentation format: 'f' - fixed, 'e' - exponential.
//    char presentation = 'f';

    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context &ctx)->decltype(ctx.begin()) {
        return ctx.begin();
    }

    // Formats the point p using the parsed format specification (presentation)
    // stored in this formatter.
    template<typename FormatContext>
    auto format(const ts::types::SyntaxKind &p, FormatContext &ctx)->decltype(ctx.out()) {
        fmt::format_to(ctx.out(), magic_enum::enum_name<ts::types::SyntaxKind>(p));
        // ctx.out() is an output iterator to write to.
//        return magic_enum::enum_name<ts::types::SyntaxKind>(p);
    }
};
