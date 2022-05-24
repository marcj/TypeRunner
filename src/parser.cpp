#include <functional>
#include "parser.h"
#include "diagnostic_messages.h"

using namespace ts;

ts::SourceFile Parser::parse() {
    // Prime the scanner.
    scanner.scan();

//    auto statements = parseList(ParsingContext::SourceElements, parseStatement);
}

bool Parser::isListTerminator(ParsingContext kind) {
    if (token() == SyntaxKind::EndOfFileToken) {
        // Being at the end of the file ends all lists.
        return true;
    }

    switch (kind) {
        case ParsingContext::BlockStatements:
        case ParsingContext::SwitchClauses:
        case ParsingContext::TypeMembers:
        case ParsingContext::ClassMembers:
        case ParsingContext::EnumMembers:
        case ParsingContext::ObjectLiteralMembers:
        case ParsingContext::ObjectBindingElements:
        case ParsingContext::ImportOrExportSpecifiers:
        case ParsingContext::AssertEntries:
            return token() == SyntaxKind::CloseBraceToken;
        case ParsingContext::SwitchClauseStatements:
            return token() == SyntaxKind::CloseBraceToken || token() == SyntaxKind::CaseKeyword || token() == SyntaxKind::DefaultKeyword;
        case ParsingContext::HeritageClauseElement:
            return token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::ExtendsKeyword || token() == SyntaxKind::ImplementsKeyword;
        case ParsingContext::VariableDeclarations:
            return isVariableDeclaratorListTerminator();
        case ParsingContext::TypeParameters:
            // Tokens other than '>' are here for better error recovery
            return token() == SyntaxKind::GreaterThanToken || token() == SyntaxKind::OpenParenToken || token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::ExtendsKeyword || token() == SyntaxKind::ImplementsKeyword;
        case ParsingContext::ArgumentExpressions:
            // Tokens other than ')' are here for better error recovery
            return token() == SyntaxKind::CloseParenToken || token() == SyntaxKind::SemicolonToken;
        case ParsingContext::ArrayLiteralMembers:
        case ParsingContext::TupleElementTypes:
        case ParsingContext::ArrayBindingElements:
            return token() == SyntaxKind::CloseBracketToken;
        case ParsingContext::JSDocParameters:
        case ParsingContext::Parameters:
        case ParsingContext::RestProperties:
            // Tokens other than ')' and ']' (the latter for index signatures) are here for better error recovery
            return token() == SyntaxKind::CloseParenToken || token() == SyntaxKind::CloseBracketToken /*|| token == SyntaxKind::OpenBraceToken*/;
        case ParsingContext::TypeArguments:
            // All other tokens should cause the type-argument to terminate except comma token
            return token() != SyntaxKind::CommaToken;
        case ParsingContext::HeritageClauses:
            return token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::CloseBraceToken;
        case ParsingContext::JsxAttributes:
            return token() == SyntaxKind::GreaterThanToken || token() == SyntaxKind::SlashToken;
        case ParsingContext::JsxChildren:
            return token() == SyntaxKind::LessThanToken && lookAhead([this] { return this->nextTokenIsSlash(); });
        default:
            return false;
    }
}

SyntaxKind Parser::token() {
    return currentToken;
}

SyntaxKind Parser::nextToken() {
    // if the keyword had an escape
    if (isKeyword(currentToken) && (scanner.hasUnicodeEscape() || scanner.hasExtendedUnicodeEscape())) {
        // issue a parse error for the escape
        parseErrorAt(scanner.getTokenPos(), scanner.getTextPos(), Diagnostics::Keywords_cannot_contain_escape_characters);
    }
    return nextTokenWithoutCheck();
}

bool Parser::nextTokenIsSlash() {
    return nextToken() == SyntaxKind::SlashToken;
}

SyntaxKind Parser::nextTokenWithoutCheck() {
    return currentToken = scanner.scan();
}

bool Parser::lookAhead(function<any()> callback) {
    return speculationHelper(callback, SpeculationKind::Lookahead);
}

bool Parser::tryParse(function<any()> callback) {
    return speculationHelper(callback, SpeculationKind::TryParse);
}

bool Parser::speculationHelper(function<any()> callback, SpeculationKind speculationKind) {
    // Keep track of the state we'll need to rollback to if lookahead fails (or if the
    // caller asked us to always reset our state).
    auto saveToken = currentToken;
    auto saveParseDiagnosticsLength = parseDiagnostics.size();
    auto saveParseErrorBeforeNextFinishedNode = parseErrorBeforeNextFinishedNode;

    // Note: it is not actually necessary to save/restore the context flags here.  That's
    // because the saving/restoring of these flags happens naturally through the recursive
    // descent nature of our parser.  However, we still store this here just so we can
    // assert that invariant holds.
    auto saveContextFlags = contextFlags;

    // If we're only looking ahead, then tell the scanner to only lookahead as well.
    // Otherwise, if we're actually speculatively parsing, then tell the scanner to do the
    // same.
    auto result = speculationKind != SpeculationKind::TryParse
                  ? scanner.lookAhead(callback)
                  : scanner.tryScan(callback);

    assert(saveContextFlags == contextFlags);

    // If our callback returned something 'falsy' or we're just looking ahead,
    // then unconditionally restore us to where we were.
    if (!result || speculationKind != SpeculationKind::TryParse) {
        currentToken = saveToken;
        if (speculationKind != SpeculationKind::Reparse) {
            parseDiagnostics.resize(saveParseDiagnosticsLength);
        }
        parseErrorBeforeNextFinishedNode = saveParseErrorBeforeNextFinishedNode;
    }

    return result;
}

bool isInOrOfKeyword(SyntaxKind t) {
    return t == SyntaxKind::InKeyword || t == SyntaxKind::OfKeyword;
}

bool Parser::isVariableDeclaratorListTerminator() {
    // If we can consume a semicolon (either explicitly, or with ASI), then consider us done
    // with parsing the list of variable declarators.
    if (canParseSemicolon()) {
        return true;
    }

    // in the case where we're parsing the variable declarator of a 'for-in' statement, we
    // are done if we see an 'in' keyword in front of us. Same with for-of
    if (isInOrOfKeyword(token())) {
        return true;
    }

    // ERROR RECOVERY TWEAK:
    // For better error recovery, if we see an '=>' then we just stop immediately.  We've got an
    // arrow function here and it's going to be very unlikely that we'll resynchronize and get
    // another variable declaration.
    if (token() == SyntaxKind::EqualsGreaterThanToken) {
        return true;
    }

    // Keep trying to parse out variable declarators.
    return false;
}

bool Parser::canParseSemicolon() {
    // If there's a real semicolon, then we can always parse it out.
    if (token() == SyntaxKind::SemicolonToken) {
        return true;
    }

    // We can parse out an optional semicolon in ASI cases in the following cases.
    return token() == SyntaxKind::CloseBraceToken || token() == SyntaxKind::EndOfFileToken || scanner.hasPrecedingLineBreak();
}

Statement Parser::parseStatement() {
    return Statement{};
}

//NodeArray Parser::createNodeArray(vector<Node> nodes, int pos, int end, bool hasTrailingComma) {
//    auto array = NodeArray{.list = nodes, pos, .end = end == -1 ? scanner.getStartPos() : end, hasTrailingComma};
//
//    return array;
//}

//optional<Node> Parser::currentNode(int parsingContext) {
//    // If we don't have a cursor or the parsing context isn't reusable, there's nothing to reuse.
//    //
//    // If there is an outstanding parse error that we've encountered, but not attached to
//    // some node, then we cannot get a node from the old source tree.  This is because we
//    // want to mark the next node we encounter as being unusable.
//    //
//    // Note: This may be too conservative.  Perhaps we could reuse the node and set the bit
//    // on it (or its leftmost child) as having the error.  For now though, being conservative
//    // is nice and likely won't ever affect perf.
//    if (!syntaxCursor || !isReusableParsingContext(parsingContext) || parseErrorBeforeNextFinishedNode) {
//        return nullopt;
//    }
//
//    const node = syntaxCursor.currentNode(scanner.getStartPos());
//
//    // Can't reuse a missing node.
//    // Can't reuse a node that intersected the change range.
//    // Can't reuse a node that contains a parse error.  This is necessary so that we
//    // produce the same set of errors again.
//    if (nodeIsMissing(node) || node.intersectsChange || containsParseError(node)) {
//        return nullopt;
//    }
//
//    // We can only reuse a node if it was parsed under the same strict mode that we're
//    // currently in.  i.e. if we originally parsed a node in non-strict mode, but then
//    // the user added 'using strict' at the top of the file, then we can't use that node
//    // again as the presence of strict mode may cause us to parse the tokens in the file
//    // differently.
//    //
//    // Note: we *can* reuse tokens when the strict mode changes.  That's because tokens
//    // are unaffected by strict mode.  It's just the parser will decide what to do with it
//    // differently depending on what mode it is in.
//    //
//    // This also applies to all our other context flags as well.
//    auto nodeContextFlags = node.flags & NodeFlags::ContextFlags;
//    if (nodeContextFlags != contextFlags) {
//        return nullopt;
//    }
//
//    // Ok, we have a node that looks like it could be reused.  Now verify that it is valid
//    // in the current list parsing context that we're currently at.
//    if (!canReuseNode(node, parsingContext)) {
//        return nullopt;
//    }
//
////    if ((node as JSDocContainer).jsDocCache) {
////        // jsDocCache may include tags from parent nodes, which might have been modified.
////        (node as JSDocContainer).jsDocCache = undefined;
////    }
//
//    return node;
//}

bool Parser::isListElement(int parsingContext, bool inErrorRecovery) {
    //see signature of currentNode
//    auto node = currentNode(parsingContext);
//    if (node) {
//        return true;
//    }

//    switch (parsingContext) {
//        case ParsingContext::SourceElements:
//        case ParsingContext::BlockStatements:
//        case ParsingContext::SwitchClauseStatements:
//            // If we're in error recovery, then we don't want to treat ';' as an empty statement.
//            // The problem is that ';' can show up in far too many contexts, and if we see one
//            // and assume it's a statement, then we may bail out inappropriately from whatever
//            // we're parsing.  For example, if we have a semicolon in the middle of a class, then
//            // we really don't want to assume the class is over and we're on a statement in the
//            // outer module.  We just want to consume and move on.
//            return !(token() == SyntaxKind::SemicolonToken && inErrorRecovery) && isStartOfStatement();
//        case ParsingContext::SwitchClauses:
//            return token() == SyntaxKind::CaseKeyword || token() == SyntaxKind::DefaultKeyword;
//        case ParsingContext::TypeMembers:
//            return lookAhead(isTypeMemberStart);
//        case ParsingContext::ClassMembers:
//            // We allow semicolons as class elements (as specified by ES6) as long as we're
//            // not in error recovery.  If we're in error recovery, we don't want an errant
//            // semicolon to be treated as a class member (since they're almost always used
//            // for statements.
//            return lookAhead(isClassMemberStart) || (token() == SyntaxKind::SemicolonToken && !inErrorRecovery);
//        case ParsingContext::EnumMembers:
//            // Include open bracket computed properties. This technically also lets in indexers,
//            // which would be a candidate for improved error reporting.
//            return token() == SyntaxKind::OpenBracketToken || isLiteralPropertyName();
//        case ParsingContext::ObjectLiteralMembers:
//            switch (token()) {
//                case SyntaxKind::OpenBracketToken:
//                case SyntaxKind::AsteriskToken:
//                case SyntaxKind::DotDotDotToken:
//                case SyntaxKind::DotToken: // Not an object literal member, but don't want to close the object (see `tests/cases/fourslash/completionsDotInObjectLiteral.ts`)
//                    return true;
//                default:
//                    return isLiteralPropertyName();
//            }
//        case ParsingContext::RestProperties:
//            return isLiteralPropertyName();
//        case ParsingContext::ObjectBindingElements:
//            return token() == SyntaxKind::OpenBracketToken || token() == SyntaxKind::DotDotDotToken || isLiteralPropertyName();
//        case ParsingContext::AssertEntries:
//            return isAssertionKey();
//        case ParsingContext::HeritageClauseElement:
//            // If we see `{ ... }` then only consume it as an expression if it is followed by `,` or `{`
//            // That way we won't consume the body of a class in its heritage clause.
//            if (token() == SyntaxKind::OpenBraceToken) {
//                return lookAhead(isValidHeritageClauseObjectLiteral);
//            }
//
//            if (!inErrorRecovery) {
//                return isStartOfLeftHandSideExpression() && !isHeritageClauseExtendsOrImplementsKeyword();
//            } else {
//                // If we're in error recovery we tighten up what we're willing to match.
//                // That way we don't treat something like "this" as a valid heritage clause
//                // element during recovery.
//                return isIdentifier() && !isHeritageClauseExtendsOrImplementsKeyword();
//            }
//        case ParsingContext::VariableDeclarations:
//            return isBindingIdentifierOrPrivateIdentifierOrPattern();
//        case ParsingContext::ArrayBindingElements:
//            return token() == SyntaxKind::CommaToken || token() == SyntaxKind::DotDotDotToken || isBindingIdentifierOrPrivateIdentifierOrPattern();
//        case ParsingContext::TypeParameters:
//            return token() == SyntaxKind::InKeyword || isIdentifier();
//        case ParsingContext::ArrayLiteralMembers:
//            switch (token()) {
//                case SyntaxKind::CommaToken:
//                case SyntaxKind::DotToken: // Not an array literal member, but don't want to close the array (see `tests/cases/fourslash/completionsDotInArrayLiteralInObjectLiteral.ts`)
//                    return true;
//            }
//            // falls through
//        case ParsingContext::ArgumentExpressions:
//            return token() == SyntaxKind::DotDotDotToken || isStartOfExpression();
//        case ParsingContext::Parameters:
//            return isStartOfParameter(/*isJSDocParameter*/ false);
//        case ParsingContext::JSDocParameters:
//            return isStartOfParameter(/*isJSDocParameter*/ true);
//        case ParsingContext::TypeArguments:
//        case ParsingContext::TupleElementTypes:
//            return token() == SyntaxKind::CommaToken || isStartOfType();
//        case ParsingContext::HeritageClauses:
//            return isHeritageClause();
//        case ParsingContext::ImportOrExportSpecifiers:
//            return tokenIsIdentifierOrKeyword(token());
//        case ParsingContext::JsxAttributes:
//            return tokenIsIdentifierOrKeyword(token()) || token() == SyntaxKind::OpenBraceToken;
//        case ParsingContext::JsxChildren:
//            return true;
//    }
//
//    return Debug.fail("Non-exhaustive case in 'isListElement'.");
}
bool Parser::isIdentifier() {
    if (token() == SyntaxKind::Identifier) {
        return true;
    }

//    // If we have a 'yield' keyword, and we're in the [yield] context, then 'yield' is
//    // considered a keyword and is not an identifier.
//    if (token() == SyntaxKind::YieldKeyword && inYieldContext()) {
//        return false;
//    }
//
//    // If we have a 'await' keyword, and we're in the [Await] context, then 'await' is
//    // considered a keyword and is not an identifier.
//    if (token() == SyntaxKind::AwaitKeyword && inAwaitContext()) {
//        return false;
//    }

    return token() > SyntaxKind::LastReservedWord;
}

bool tokenIsIdentifierOrKeyword(SyntaxKind token) {
    return token >= SyntaxKind::Identifier;
}

bool Parser::nextTokenIsIdentifierOnSameLine() {
    nextToken();
    return !scanner.hasPrecedingLineBreak() && isIdentifier();
}

bool Parser::nextTokenIsIdentifierOrStringLiteralOnSameLine() {
    nextToken();
    return !scanner.hasPrecedingLineBreak() && (isIdentifier() || token() == SyntaxKind::StringLiteral);
}

bool Parser::isDeclaration() {
    while (true) {
        switch (token()) {
            case SyntaxKind::VarKeyword:
            case SyntaxKind::LetKeyword:
            case SyntaxKind::ConstKeyword:
            case SyntaxKind::FunctionKeyword:
            case SyntaxKind::ClassKeyword:
            case SyntaxKind::EnumKeyword:
                return true;

                // 'declare', 'module', 'namespace', 'interface'* and 'type' are all legal JavaScript identifiers;
                // however, an identifier cannot be followed by another identifier on the same line. This is what we
                // count on to parse out the respective declarations. For instance, we exploit this to say that
                //
                //    namespace n
                //
                // can be none other than the beginning of a namespace declaration, but need to respect that JavaScript sees
                //
                //    namespace
                //    n
                //
                // as the identifier 'namespace' on one line followed by the identifier 'n' on another.
                // We need to look one token ahead to see if it permissible to try parsing a declaration.
                //
                // *Note*: 'interface' is actually a strict mode reserved word. So while
                //
                //   "use strict"
                //   interface
                //   I {}
                //
                // could be legal, it would add complexity for very little gain.
            case SyntaxKind::InterfaceKeyword:
            case SyntaxKind::TypeKeyword:
                return nextTokenIsIdentifierOnSameLine();
            case SyntaxKind::ModuleKeyword:
            case SyntaxKind::NamespaceKeyword:
                return nextTokenIsIdentifierOrStringLiteralOnSameLine();
            case SyntaxKind::AbstractKeyword:
            case SyntaxKind::AsyncKeyword:
            case SyntaxKind::DeclareKeyword:
            case SyntaxKind::PrivateKeyword:
            case SyntaxKind::ProtectedKeyword:
            case SyntaxKind::PublicKeyword:
            case SyntaxKind::ReadonlyKeyword:
                nextToken();
                // ASI takes effect for this modifier.
                if (scanner.hasPrecedingLineBreak()) {
                    return false;
                }
                continue;

            case SyntaxKind::GlobalKeyword:
                nextToken();
                return token() == SyntaxKind::OpenBraceToken || token() == SyntaxKind::Identifier || token() == SyntaxKind::ExportKeyword;

            case SyntaxKind::ImportKeyword:
                nextToken();
                return token() == SyntaxKind::StringLiteral || token() == SyntaxKind::AsteriskToken ||
                       token() == SyntaxKind::OpenBraceToken || tokenIsIdentifierOrKeyword(token());
//            case SyntaxKind::ExportKeyword:
//                auto currentToken = nextToken();
//                if (currentToken == SyntaxKind::TypeKeyword) {
//                    currentToken = lookAhead([this] { return this->nextToken(); });
//                }
//                if (currentToken == SyntaxKind::EqualsToken || currentToken == SyntaxKind::AsteriskToken ||
//                    currentToken == SyntaxKind::OpenBraceToken || currentToken == SyntaxKind::DefaultKeyword ||
//                    currentToken == SyntaxKind::AsKeyword) {
//                    return true;
//                }
//                continue;

            case SyntaxKind::StaticKeyword:
                nextToken();
                continue;
            default:
                return false;
        }
    }
}
bool Parser::isStartOfDeclaration() {
    return lookAhead([this] { return this->isDeclaration(); });
}
bool Parser::isStartOfStatement() {
    switch (token()) {
        case SyntaxKind::AtToken:
        case SyntaxKind::SemicolonToken:
        case SyntaxKind::OpenBraceToken:
        case SyntaxKind::VarKeyword:
        case SyntaxKind::LetKeyword:
        case SyntaxKind::FunctionKeyword:
        case SyntaxKind::ClassKeyword:
        case SyntaxKind::EnumKeyword:
        case SyntaxKind::IfKeyword:
        case SyntaxKind::DoKeyword:
        case SyntaxKind::WhileKeyword:
        case SyntaxKind::ForKeyword:
        case SyntaxKind::ContinueKeyword:
        case SyntaxKind::BreakKeyword:
        case SyntaxKind::ReturnKeyword:
        case SyntaxKind::WithKeyword:
        case SyntaxKind::SwitchKeyword:
        case SyntaxKind::ThrowKeyword:
        case SyntaxKind::TryKeyword:
        case SyntaxKind::DebuggerKeyword:
            // 'catch' and 'finally' do not actually indicate that the code is part of a statement,
            // however, we say they are here so that we may gracefully parse them and error later.
            // falls through
        case SyntaxKind::CatchKeyword:
        case SyntaxKind::FinallyKeyword:
            return true;

//        case SyntaxKind::ImportKeyword:
//            return isStartOfDeclaration() || lookAhead(nextTokenIsOpenParenOrLessThanOrDot);

        case SyntaxKind::ConstKeyword:
        case SyntaxKind::ExportKeyword:
            return isStartOfDeclaration();

        case SyntaxKind::AsyncKeyword:
        case SyntaxKind::DeclareKeyword:
        case SyntaxKind::InterfaceKeyword:
        case SyntaxKind::ModuleKeyword:
        case SyntaxKind::NamespaceKeyword:
        case SyntaxKind::TypeKeyword:
        case SyntaxKind::GlobalKeyword:
            // When these don't start a declaration, they're an identifier in an expression statement
            return true;

//        case SyntaxKind::PublicKeyword:
//        case SyntaxKind::PrivateKeyword:
//        case SyntaxKind::ProtectedKeyword:
//        case SyntaxKind::StaticKeyword:
//        case SyntaxKind::ReadonlyKeyword:
//            // When these don't start a declaration, they may be the start of a class member if an identifier
//            // immediately follows. Otherwise they're an identifier in an expression statement.
//            return isStartOfDeclaration() || !lookAhead(nextTokenIsIdentifierOrKeywordOnSameLine);
//
//        default:
//            return isStartOfExpression();
    }
}

//NodeArray Parser::parseList(ParsingContext kind, function<bool()> parseElement) {
//    auto saveParsingContext = parsingContext;
//    parsingContext |= 1 << kind;
//    vector<Node> list;
//    auto listPos = getNodePos();
//
//    while (!isListTerminator(kind)) {
//        if (isListElement(kind, /*inErrorRecovery*/ false)) {
//            list.push(parseListElement(kind, parseElement));
//
//            continue;
//        }
//
//        if (abortParsingListOrMoveToNextToken(kind)) {
//            break;
//        }
//    }
//
//    parsingContext = saveParsingContext;
//    return createNodeArray(list, listPos);
//}
