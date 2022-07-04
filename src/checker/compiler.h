#pragma once

#include <string>
#include <functional>
#include <utility>

#include "./instructions.h"
#include "./utils.h"
#include "../node_test.h"

namespace ts::checker {

    using std::string;
    using std::function;
    using instructions::OP;
    using instructions::ErrorCode;

    enum class SymbolType {
        Variable, //const x = true;
        Function, //function x() {}
        Class, //class X {}
        Inline, //parts of conditional type, mapped type, ..
        Type, //type alias, e.g. `foo` in `type foo = string;`
        TypeVariable //template variable, e.g. T in function <T>foo(bar: T);
    };

    struct SourceMapEntry {
        unsigned int bytecodePos;
        unsigned int sourcePos;
        unsigned int sourceEnd;
    };

    struct SourceMap {
        vector<SourceMapEntry> map;

        void push(unsigned int bytecodePos, unsigned int sourcePos, unsigned int sourceEnd) {
            map.push_back({bytecodePos, sourcePos, sourceEnd});
        }
    };

    struct ArgumentUsage {
        unsigned int lastIp{};
        unsigned int lastSubroutineIndex{};
        unsigned int argumentIndex{};

        explicit ArgumentUsage(unsigned int argumentIndex): argumentIndex(argumentIndex) {}
    };

    //A subroutine is a sub program that can be executed by knowing its address.
    //They are used for example for type alias, mapped type, conditional type (for false and true side)
    struct Subroutine {
        vector<unsigned char> ops; //OPs, and its parameters
        SourceMap sourceMap;
        string_view identifier{};
        unsigned int index{};
        unsigned int nameAddress{};
        vector<ArgumentUsage> argumentUsages;
        SymbolType type = SymbolType::Type;

        void pushSourceMap(unsigned int sourcePos, unsigned int sourceEnd) {
            sourceMap.push(ops.size(), sourcePos, sourceEnd);
        }

        void registerArgumentUsage() {
            argumentUsages.emplace_back(argumentUsages.size());
        }

        ArgumentUsage &getArgumentUsage(unsigned int argumentIndex) {
            if (argumentUsages.size()<=argumentIndex) {
                throw std::runtime_error("Unknown argument");
            }
            return argumentUsages[argumentIndex];
        }

        unsigned int getFlags() {
            unsigned int flags = 0;
            if (type == SymbolType::Inline) {
                flags |= instructions::SubroutineFlag::Inline;
            }
            return flags;
        }

        explicit Subroutine() {
            identifier = "";
        }

        explicit Subroutine(string_view &identifier): identifier(identifier) {}
    };

    struct Frame;

    struct Symbol {
        string name;
        SymbolType type = SymbolType::Type;
        unsigned int index{}; //symbol index of the current frame
        unsigned int pos{};
        unsigned int end{};
        unsigned int declarations = 1;
        unsigned int lastUsedIP = 0;
        sharedOpt<Subroutine> routine = nullptr;
        shared<Frame> frame = nullptr;
    };

    struct Frame {
        const bool conditional = false;
        shared<Frame> previous;
        unsigned int id = 0; //in a tree the unique id, needed to resolve symbols during runtime.
        vector<Symbol> symbols{};

        Frame() = default;

        Frame(shared<Frame> previous): previous(std::move(previous)) {}
    };

    struct StorageItem {
        string_view value;
        unsigned int address{};

        explicit StorageItem(const string_view &value): value(value) {}
    };

    struct FrameOffset {
        uint32_t frame; //how many frames up
        uint32_t symbol; //the index of the symbol in referenced frame, refers directly to x stack entry of that stack frame.
    };

    struct Visit {
        bool active = true;
        unsigned int index; //subroutine index
        unsigned int ip; //subroutine ip
        unsigned int frameDepth;
        OP op;
    };

    inline void visitOps2(vector<shared<Subroutine>> &subroutines, Visit &visit, const function<void(Visit &)> &callback) {
        const auto ops = subroutines[visit.index]->ops;
        for (unsigned int i = 0; visit.active && i<ops.size(); i++) {
            visit.op = (OP) ops[i];
            switch (visit.op) {
                case OP::Frame: {
                    visit.frameDepth++;
                    break;
                }
                case OP::Tuple:
                case OP::Union:
                case OP::Intersection:
                case OP::Class:
                case OP::ObjectLiteral:
                case OP::Return: {
                    visit.frameDepth--;
                    break;
                }
                    //todo: Go deeper for inline functions for distributive and mapped types, too.
                case OP::JumpCondition: {
                    //go deeper
                    const auto leftProgram = vm::readUint16(ops, i + 1);
                    const auto rightProgram = vm::readUint16(ops, i + 3);
                    visit.frameDepth++;
                    visit.index = leftProgram;
                    visitOps2(subroutines, visit, callback);
                    visit.index = rightProgram;
                    visitOps2(subroutines, visit, callback);
                    visit.frameDepth--;
                    break;
                }
                default: {
                    visit.ip = i;
                    callback(visit);
                }
            }
            vm::eatParams(visit.op, &i);
        }
    }

    inline void visitOps(vector<shared<Subroutine>> &subroutines, unsigned int index, const function<void(Visit &)> &callback) {
        auto current = subroutines[index];
        Visit visit;
        visit.index = index;
        visitOps2(subroutines, visit, callback);
    }

    shared<Subroutine> findOuterTypeFunction(vector<shared<Subroutine>> &subroutines, shared<Subroutine> &subroutine) {
        shared<Subroutine> typeFunction = subroutine;

        while (typeFunction->type == SymbolType::Inline) {
            if (typeFunction->index == 0) {
                debug("No type function found for subroutine");
                return subroutines[0];
            }
            //the actual function is always before the Inline
            typeFunction = subroutines[typeFunction->index - 1];
        }

        return typeFunction;
    }

    inline void optimiseRestReuse(vector<shared<Subroutine>> &subroutines, shared<Subroutine> &subroutine) {
        for (auto &&variable: subroutine->argumentUsages) {
            if (!variable.lastIp) continue;

            //check if it was used in ...T, and mark it as RestReuse
            auto &lastUseSubroutine = subroutines[variable.lastSubroutineIndex];
            auto variableUserOp = (OP)lastUseSubroutine->ops[variable.lastIp + 1 + 2 + 2];
            if (variableUserOp == OP::Rest) {
                lastUseSubroutine->ops[variable.lastIp + 1 + 2 + 2] = OP::RestReuse;
            }
        }

//        std::set<Subroutine *> visited;
//
//        LastRest lastRest = subroutine->lastRest;
//
//        //we support for the moment only
//        bool startPositionFound = false;
//        bool startIndex = typeFunction->index;
//        bool usedAfterRest = false;
//
//        //note: if we have a second optimisation that needs a OP forward-pass, we should generalise it and do it only once.
//        visitOps(subroutines, typeFunction->index, [&lastRest, &startPositionFound, &subroutines, &usedAfterRest, &startIndex](Visit visit) {
//            if (!startPositionFound && visit.index == startIndex && visit.op == lastRest.ip) {
//                startPositionFound = true;
//            }
//
//            if (startPositionFound) {
//                //detect now usage of the lastRest.typeArgument
//                if (visit.op == OP::Loads) {
//                    //check if it actually references the right typeArgument
//                    unsigned int frameOffset = vm::readUint16(subroutines[visit.index]->ops, visit.op + 1);
//                    unsigned int varIndex = vm::readUint16(subroutines[visit.index]->ops, visit.op + 3);
//                    if (varIndex == lastRest.typeArgument && frameOffset == visit.frameDepth) {
//                        usedAfterRest = true;
//                        visit.active = false;
//                    }
//                }
//            }
//        });
//
//        if (!usedAfterRest) {
//            //it's safe to mark it as RestReuse
//            debug("safe to RestReuse!");
//        }
    }

//    struct Optimiser {
//        vector<shared<Subroutine>> *subroutines;
//
//        explicit Optimiser(vector<shared<Subroutine>> *subroutines): subroutines(subroutines) {}
//
//        void optimise(unsigned int index) {
//            auto current = (*subroutines)[index];
//            if (current->optimised) return;
//
//            current->optimised = true;
//
//            for (unsigned int i = 0; i<current->ops.size(); i++) {
//                auto op = (OP) current->ops[i];
////                debug("[{}] optimise OP {}", current->index, op);
//
//                switch (op) {
//                    case OP::Rest: {
//                        debug("optimise Rest");
//                        //todo: check if previous is ::Loads
//                        // determine the identity of the Loads variable
//                        // and
//                        break;
//                    }
//                    case OP::Call: {
//                        //go deeper
//                        auto address = vm::readUint32(current->ops, i + 1);
//                        optimise(address);
//                        break;
//                    }
//                    case OP::JumpCondition: {
//                        //go deeper
//                        const auto leftProgram = vm::readUint16(current->ops, i + 1);
//                        const auto rightProgram = vm::readUint16(current->ops, i + 3);
//                        optimise(leftProgram);
//                        optimise(rightProgram);
//                        break;
//                    }
//                }
//                vm::eatParams(op, &i);
//            }
//        }
//    };

    class Program {
    public:
        vector<unsigned char> ops; //OPs of "main"
        SourceMap sourceMap; //SourceMap of "main"

        vector<string_view> storage; //all kind of literals, as strings
        unordered_map<uint64_t, reference_wrapper<StorageItem>> storageMap; //used to deduplicated storage entries

        unsigned int storageIndex{};
        shared<Frame> frame = make_shared<Frame>();

        //tracks which subroutine is active (end() is), so that pushOp calls are correctly assigned.
        vector<shared<Subroutine>> activeSubroutines;
        vector<shared<Subroutine>> subroutines;

//        Optimiser optimiser{&subroutines};

        //implicit is when a OP itself triggers in the VM a new frame, without having explicitly a OP::Frame
        shared<Frame> pushFrame(bool implicit = false) {
            if (!implicit) this->pushOp(OP::Frame);
            auto id = frame->id;
            frame = make_shared<Frame>(frame);
            frame->id = id + 1;
            return frame;
        }

        /**
         * Creates a new nameless subroutine, used for example in mapped-type, conditional type
         * @return
         */
        unsigned int pushSubroutineNameLess(const shared<Node> &node) {
            auto routine = make_shared<Subroutine>();
            routine->type = SymbolType::Inline;
            routine->index = subroutines.size();

            pushFrame(true); //subroutines have implicit stack frames due to call convention
            subroutines.push_back(routine);
            activeSubroutines.push_back(subroutines.back());
            return routine->index;
        }

        shared<Subroutine> getOuterTypeFunction() {
            return findOuterTypeFunction(subroutines, activeSubroutines.empty() ? subroutines[0] : activeSubroutines.back());
        }

        /**
         * Push the subroutine from the symbol as active. This means it will now be populated with OPs.
         */
        unsigned int pushSubroutine(string_view name) {
            //find subroutine
            for (auto &&s: frame->symbols) {
                if (s.name == name) {
                    pushFrame(true); //subroutines have implicit stack frames due to call convention
                    activeSubroutines.push_back(s.routine);
                    return s.routine->index;
                }
            }
            throw runtime_error(fmt::format("no symbol found for {}", name));
        }

        shared<Subroutine> popSubroutine() {
            if (activeSubroutines.empty()) throw runtime_error("No active subroutine found");
            popFrameImplicit();
            auto subroutine = activeSubroutines.back();
            if (subroutine->ops.empty()) {
                throw runtime_error("Routine is empty");
            }
            subroutine->ops.push_back(OP::Return);

            if (subroutine->type == SymbolType::Type) {
                //for type functions, we optimise ...T re-usage
                optimiseRestReuse(subroutines, subroutine);
            }

            activeSubroutines.pop_back();
            return subroutine;
        }

        Symbol *findSymbol(const string_view &identifier) {
            Frame *current = frame.get();

            while (true) {
                for (auto &&s: current->symbols) {
                    if (s.name == identifier) {
                        return &s;
                    }
                }
                if (!current->previous) break;
                current = current->previous.get();
            };

            return nullptr;
        }

        /**
         * Remove stack without doing it as OP in the VM. Some other command calls popFrame() already, which makes popFrameImplicit() an implicit popFrame.
         * e.g. union, class, etc. all call VM::popFrame(). the current CompilerProgram needs to be aware of that, which this function is for.
         */
        void popFrameImplicit() {
            if (frame->previous) frame = frame->previous;
        }

        /**
         * The address is always written using 4 bytes.
         *
         * It sometimes is defined in Program as index to the storage or subroutine and thus is a immediate representation of the address.
         * In this case it will be replaced in build() with the real address in the binary (hence why we need 4 bytes, so space stays constant).
         */
        void pushAddress(unsigned int address) {
            auto &ops = getOPs();
            vm::writeUint32(ops, ops.size(), address);
        }

        void pushUint32(unsigned int v) {
            auto &ops = getOPs();
            vm::writeUint32(ops, ops.size(), v);
        }

        void pushUint16(unsigned int v) {
            auto &ops = getOPs();
            vm::writeUint16(ops, ops.size(), v);
        }

        void pushError(ErrorCode code, const shared<Node> &node) {
            //errors need to be part of main
            sourceMap.push(0, node->pos, node->end);
            ops.push_back(OP::Error);
            vm::writeUint16(ops, ops.size(), (unsigned int) code);
        }

        void pushSymbolAddress(Symbol &symbol) {
            auto &ops = getOPs();
            unsigned int frameOffset = 0;
            auto current = frame;
            while (current) {
                if (current == symbol.frame) break;
                frameOffset++;
                current = current->previous;
            }
            vm::writeUint16(ops, ops.size(), frameOffset);
            vm::writeUint16(ops, ops.size(), symbol.index);
        }

        vector<unsigned char> &getOPs() {
            if (activeSubroutines.size()) return activeSubroutines.back()->ops;
            return ops;
        }

        void pushSourceMap(const shared<Node> &node) {
            if (activeSubroutines.size()) {
                activeSubroutines.back()->pushSourceMap(node->pos, node->end);
            } else {
                sourceMap.push(ops.size(), node->pos, node->end);
            }
        }

        void pushOp(OP op) {
            auto &ops = getOPs();
            ops.push_back(op);
        }

        unsigned int subroutineIndex() {
            return activeSubroutines.size() ? activeSubroutines.back()->index : 0;
        }

        shared<Subroutine> subroutine() {
            return activeSubroutines.size() ? activeSubroutines.back() : subroutines[0];
        }

        unsigned int ip() {
            return getOPs().size();
        }

        void pushOp(OP op, const sharedOpt<Node> &node) {
            auto &ops = getOPs();
            if (node) pushSourceMap(node);
            ops.push_back(op);
        }

        //needed for variables
//        void pushOpAtFrameInHead(shared<Frame> frame, OP op, std::vector<unsigned int> params = {}) {
//            auto &ops = getOPs();
//
//            //an earlier known frame could be referenced, in which case we have to put ops between others.
//            ops.insert(ops.begin() + frame->headOffset, op);
//            if (params.size()) {
//                ops.insert(ops.begin() + frame->headOffset + 1, params.begin(), params.end());
//            }
//        }

        /**
         * A symbol could be type alias, function expression, var type declaration.
         * Each represents a type expression and gets its own subroutine. The subroutine
         * is directly created and an index assign. Later when pushSubroutine() is called,
         * this subroutine is returned and with OPs populated.
         *
         * Symbols will be created first before a body is extracted. This makes sure all
         * symbols are known before their reference is used.
         */
        Symbol &pushSymbol(string_view name, SymbolType type, const shared<Node> &node, sharedOpt<Frame> frameToUse = nullptr) {
            if (!frameToUse) frameToUse = frame;

            for (auto &&v: frameToUse->symbols) {
                if (v.name == name) {
                    v.declarations++;
                    return v;
                }
            }

//            Symbol symbol{
//                    .name = name,
//                    .type = type,
//                    .index = (unsigned int) frameToUse->symbols.size(),
//                    .pos = pos,
//                    .routine = nullptr,
//                    .frame = frameToUse,
//            };
            Symbol symbol;
            symbol.name = string(name);
            symbol.type = type;
            symbol.index = (unsigned int) frameToUse->symbols.size();
            symbol.pos = node->pos;
            symbol.end = node->end;
            symbol.frame = frameToUse;
            frameToUse->symbols.push_back(std::move(symbol));
            return frameToUse->symbols.back();
        }

        Symbol &pushSymbolForRoutine(string_view name, SymbolType type, const shared<Node> &node, shared<Frame> frameToUse = nullptr) {
            auto &symbol = pushSymbol(name, type, node, frameToUse);
            if (symbol.routine) return symbol;

            auto routine = make_shared<Subroutine>(name);
            routine->type = type;
            routine->nameAddress = registerStorage(routine->identifier);
            routine->index = subroutines.size();
            subroutines.push_back(routine);
            symbol.routine = routine;

            return symbol;
        }

        //note: make sure the same name is not added twice. needs hashmap
        unsigned int registerStorage(const string_view &s) {
            if (!storageIndex) storageIndex = 1 + 4; //jump+address

            const auto address = storageIndex;
            storage.push_back(s);
            storageIndex += 8 + 2 + s.size(); //hash + size + data
            return address;
        }

        /**
         * Pushes a Uint32 and stores the text into the storage.
         * @param s
         */
        void pushStorage(string_view s) {
            pushAddress(registerStorage(s));
        }

        void pushStringLiteral(string_view s, const shared<Node> &node) {
            pushOp(OP::StringLiteral, node);
            pushStorage(s);
        }

        string build() {
            vector<unsigned char> bin;
            unsigned int address = 0;

            address = 5; //we add JUMP + index when building the program to jump over all subroutines&storages
            bin.push_back(OP::Jump);
            vm::writeUint32(bin, bin.size(), 0); //set after storage handling

            for (auto &&item: storage) {
                address += 8 + 2 + item.size(); //hash+size+data
            }

            //set initial jump position to right after the storage data
            vm::writeUint32(bin, 1, address);
            //push all storage data to the binary
            for (auto &&item: storage) {
                vm::writeUint64(bin, bin.size(), hash::runtime_hash(item));
                vm::writeUint16(bin, bin.size(), item.size());
                bin.insert(bin.end(), item.begin(), item.end());
            }

            //collect sourcemap data
            unsigned int sourceMapSize = 0;
            for (auto &&routine: subroutines) {
                sourceMapSize += routine->sourceMap.map.size() * (4 * 3);
            }
            sourceMapSize += sourceMap.map.size() * (4 * 3);

            //write sourcemap
            bin.push_back(OP::SourceMap);
            vm::writeUint32(bin, bin.size(), sourceMapSize);
            address += 1 + 4 + sourceMapSize; //OP::SourceMap + uint32 size

            unsigned int bytecodePosOffset = address;
            bytecodePosOffset += subroutines.size() * (1 + 4 + 4 + 1); //OP::Subroutine + uint32 name address + uint32 routine address + flags
            bytecodePosOffset += 1 + 4; //OP::Main + uint32 address

            for (auto &&routine: subroutines) {
                for (auto &&map: routine->sourceMap.map) {
                    vm::writeUint32(bin, bin.size(), bytecodePosOffset + map.bytecodePos);
                    vm::writeUint32(bin, bin.size(), map.sourcePos);
                    vm::writeUint32(bin, bin.size(), map.sourceEnd);
                }
                bytecodePosOffset += routine->ops.size();
            }

            for (auto &&map: sourceMap.map) {
                vm::writeUint32(bin, bin.size(), bytecodePosOffset + map.bytecodePos);
                vm::writeUint32(bin, bin.size(), map.sourcePos);
                vm::writeUint32(bin, bin.size(), map.sourceEnd);
            }

            address += 1 + 4; //OP::Main + uint32 address
            address += subroutines.size() * (1 + 4 + 4 + 1); //OP::Subroutine + uint32 name address + uint32 routine address + flags

            //after the storage data follows the subroutine meta-data.
            for (auto &&routine: subroutines) {
                bin.push_back(OP::Subroutine);
                vm::writeUint32(bin, bin.size(), routine->nameAddress);
                vm::writeUint32(bin, bin.size(), address);
                bin.push_back(routine->getFlags());
                address += routine->ops.size();
            }

            //after subroutine meta-data follows the actual subroutine code, which we jump over.
            //this marks the end of the header.
            bin.push_back(OP::Main);
            vm::writeUint32(bin, bin.size(), address);

            for (auto &&routine: subroutines) {
                bin.insert(bin.end(), routine->ops.begin(), routine->ops.end());
            }

            //now the main code is added
            bin.insert(bin.end(), ops.begin(), ops.end());
            bin.push_back(OP::Halt);

            return string(bin.begin(), bin.end());
        }
    };

    class Compiler {
    public:
        Program compileSourceFile(const shared<SourceFile> &file) {
            Program program;

            handle(file, program);

            return std::move(program);
        }

        void handle(const shared<Node> &node, Program &program) {
            switch (node->kind) {
                case types::SyntaxKind::SourceFile: {
                    for (auto &&statement: to<SourceFile>(node)->statements->list) {
                        handle(statement, program);
                    }
                    break;
                }
                case types::SyntaxKind::AnyKeyword:
                    program.pushOp(OP::Any, node);
                    break;
                case types::SyntaxKind::NullKeyword:
                    program.pushOp(OP::Null, node);
                    break;
                case types::SyntaxKind::UndefinedKeyword:
                    program.pushOp(OP::Undefined, node);
                    break;
                case types::SyntaxKind::NeverKeyword:
                    program.pushOp(OP::Never, node);
                    break;
                case types::SyntaxKind::BooleanKeyword:
                    program.pushOp(OP::Boolean, node);
                    break;
                case types::SyntaxKind::StringKeyword:
                    program.pushOp(OP::String, node);
                    break;
                case types::SyntaxKind::NumberKeyword:
                    program.pushOp(OP::Number, node);
                    break;
                case types::SyntaxKind::BigIntLiteral:
                    program.pushOp(OP::BigIntLiteral, node);
                    program.pushStorage(to<BigIntLiteral>(node)->text);
                    break;
                case types::SyntaxKind::NumericLiteral:
                    program.pushOp(OP::NumberLiteral, node);
                    program.pushStorage(to<NumericLiteral>(node)->text);
                    break;
                case types::SyntaxKind::StringLiteral:
                    program.pushOp(OP::StringLiteral, node);
                    program.pushStorage(to<StringLiteral>(node)->text);
                    break;
                case types::SyntaxKind::TrueKeyword:
                    program.pushOp(OP::True, node);
                    break;
                case types::SyntaxKind::FalseKeyword:
                    program.pushOp(OP::False, node);
                    break;
                case types::SyntaxKind::IndexedAccessType: {
                    const auto n = to<IndexedAccessTypeNode>(node);

                    handle(n->objectType, program);
                    handle(n->indexType, program);
                    program.pushOp(OP::IndexAccess, node);
                    break;
                }
                case types::SyntaxKind::LiteralType: {
                    const auto n = to<LiteralTypeNode>(node);
                    handle(n->literal, program);
                    break;
                }
                case types::SyntaxKind::TemplateLiteralType: {
                    auto t = to<TemplateLiteralTypeNode>(node);

                    program.pushFrame();
                    if (t->head->rawText && *t->head->rawText != "") {
                        program.pushOp(OP::StringLiteral, t->head);
                        program.pushStorage(*t->head->rawText);
                    }

                    for (auto &&sub: t->templateSpans->list) {
                        auto span = to<TemplateLiteralTypeSpan>(sub);
                        handle(to<TemplateLiteralTypeSpan>(span)->type, program);

                        if (auto a = to<TemplateMiddle>(span->literal)) {
                            if (a->rawText && *a->rawText != "") {
                                program.pushOp(OP::StringLiteral, sub);
                                program.pushStorage(a->rawText ? *a->rawText : "");
                            }
                        } else if (auto a = to<TemplateTail>(span->literal)) {
                            if (a->rawText && *a->rawText != "") {
                                program.pushOp(OP::StringLiteral, a);
                                program.pushStorage(a->rawText ? *a->rawText : "");
                            }
                        }
                    }

                    program.pushOp(OP::TemplateLiteral, node);
                    program.popFrameImplicit();

                    break;
                }
                case types::SyntaxKind::UnionType: {
                    const auto n = to<UnionTypeNode>(node);
                    program.pushFrame();

                    for (auto &&s: n->types->list) {
                        handle(s, program);
                    }

                    program.pushOp(OP::Union, node);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::TypeReference: {
                    //todo: search in symbol table and get address
//                    debug("type reference {}", to<TypeReferenceNode>(node)->typeName->to<Identifier>().escapedText);
//                    program.pushOp(OP::Number);
                    const auto n = to<TypeReferenceNode>(node);
                    const auto name = to<Identifier>(n->typeName)->escapedText;
                    auto symbol = program.findSymbol(name);
                    if (!symbol) {
                        program.pushOp(OP::Never, n->typeName);
                        program.pushError(ErrorCode::CannotFind, n->typeName);
                    } else {
                        if (symbol->type == SymbolType::TypeVariable) {
                            auto &variableUsage = program.getOuterTypeFunction()->getArgumentUsage(symbol->index);
                            variableUsage.lastIp = program.ip();
                            variableUsage.lastSubroutineIndex = program.subroutineIndex();

                            program.pushOp(OP::Loads, n->typeName);
                            program.pushSymbolAddress(*symbol);
                        } else {
                            if (n->typeArguments) {
                                for (auto &&p: n->typeArguments->list) {
                                    handle(p, program);
                                }
                            }
                            program.pushOp(OP::Call, n->typeName);
                            if (!symbol->routine) {
                                throw runtime_error("Reference is not a reference to a existing routine.");
                            }
                            program.pushAddress(symbol->routine->index);
                            if (n->typeArguments) {
                                program.pushUint16(n->typeArguments->length());
                            } else {
                                program.pushUint16(0);
                            }
                        }
                    }
                    break;
                }
                case types::SyntaxKind::TypeAliasDeclaration: {
                    const auto n = to<TypeAliasDeclaration>(node);

                    auto &symbol = program.pushSymbolForRoutine(n->name->escapedText, SymbolType::Type, n); //move this to earlier symbol-scan round
                    if (symbol.declarations>1) {
                        //todo: for functions/variable embed an error that symbol was declared twice in the same scope
                    } else {
                        //populate routine
                        program.pushSubroutine(n->name->escapedText);

                        if (n->typeParameters) {
                            for (auto &&p: n->typeParameters->list) {
                                handle(p, program);
                            }
                        }

                        handle(n->type, program);
                        program.popSubroutine();
                    }
                    break;
                }
                case types::SyntaxKind::Parameter: {
                    const auto n = to<ParameterDeclaration>(node);
                    if (n->type) {
                        handle(n->type, program);
                    } else {
                        program.pushOp(OP::Unknown, node);
                    }
                    program.pushOp(OP::Parameter, node);
                    if (auto id = to<Identifier>(n->name)) {
                        program.pushStorage(id->escapedText);
                    } else {
                        program.pushStorage("");
                    }
                    if (n->questionToken) program.pushOp(OP::Optional, n->questionToken);
                    if (n->initializer) {
                        handle(n->initializer, program);
                        program.pushOp(OP::Initializer, n->initializer);
                    }
                    break;
                }
                case types::SyntaxKind::TypeParameter: {
                    const auto n = to<TypeParameterDeclaration>(node);
                    auto &symbol = program.pushSymbol(n->name->escapedText, SymbolType::TypeVariable, n);
                    if (n->defaultType) {
                        program.pushSubroutineNameLess(n->defaultType);
                        handle(n->defaultType, program);
                        auto routine = program.popSubroutine();
                        program.pushOp(instructions::TypeArgumentDefault, n->name);
                        program.pushAddress(routine->index);
                    } else {
                        program.pushOp(instructions::TypeArgument, n->name);
                    }
                    program.subroutine()->registerArgumentUsage();
                    //todo constraints
                    break;
                }
                case types::SyntaxKind::FunctionDeclaration: {
                    const auto n = to<FunctionDeclaration>(node);
                    if (const auto id = to<Identifier>(n->name)) {
                        auto &symbol = program.pushSymbolForRoutine(id->escapedText, SymbolType::Function, id); //move this to earlier symbol-scan round
                        if (symbol.declarations>1) {
                            //todo: embed error since function is declared twice
                        } else {
                            if (n->typeParameters) {
                                program.pushSubroutine(id->escapedText);
                                //when there are type parameters, FunctionDeclaration returns a FunctionRef
                                //which indicates the VM that the function needs to be instantiated first.

                                auto subroutineIndex = program.pushSubroutineNameLess(n);

                                for (auto &&param: n->typeParameters->list) {
                                    handle(param, program);
                                }

                                //<T>(v: T)
                                //<T extends string>(v: T)

                                //<T>(k: {v: T}, v: T), ({v: ''}, v: 2) => T=string
                                //<T extends string>(k: {v: T}, v: T), ({v: ''}, v: 2) => T=''
                                //<T, K extends {v: T}>(k: K, v: T), ({v: ''}, v: 2) => T=number

                                //try to infer type parameters from passed function parameters
                                for (auto &&param: n->typeParameters->list) {
                                }

                                //todo
                                //after types are inferred, apply default if still empty

                                //after types are set, apply constraint check

                                for (auto &&param: n->parameters->list) {
                                    handle(param, program);
                                }
                                if (n->type) {
                                    handle(n->type, program);
                                } else {
                                    //todo: Infer from body
                                    program.pushOp(OP::Unknown);
                                    if (n->body) {
                                    } else {
                                    }
                                }
                                program.pushOp(OP::Function, node);
                                program.popSubroutine();

                                program.pushOp(OP::FunctionRef, node);
                                program.pushAddress(subroutineIndex);
                                program.popSubroutine();
                            } else {
                                program.pushSubroutine(id->escapedText);
                                for (auto &&param: n->parameters->list) {
                                    handle(param, program);
                                }
                                if (n->type) {
                                    handle(n->type, program);
                                } else {
                                    //todo: Infer from body
                                    program.pushOp(OP::Unknown);
                                    if (n->body) {
                                    } else {
                                    }
                                }
                                program.pushOp(OP::Function, node);
                                program.popSubroutine();
                            }
                        }
                    } else {
                        debug("No identifier in name");
                    }

                    break;
                }
                case types::SyntaxKind::Identifier: {
                    const auto n = to<Identifier>(node);
                    auto symbol = program.findSymbol(n->escapedText);
                    if (!symbol) {
                        program.pushOp(OP::Never, n);
                        program.pushError(ErrorCode::CannotFind, n);
                    } else {
                        if (symbol->type == SymbolType::TypeVariable) {
                            program.pushOp(OP::Loads, node);
                            program.pushSymbolAddress(*symbol);
                        } else {
                            if (n->typeArguments) {
                                for (auto &&p: n->typeArguments->list) {
                                    handle(p, program);
                                }
                            }
                            program.pushOp(OP::Call, node);
                            if (!symbol->routine) {
                                throw runtime_error("Reference is not a reference to a existing routine.");
                            }
                            program.pushAddress(symbol->routine->index);
                            if (n->typeArguments) {
                                program.pushUint16(n->typeArguments->length());
                            } else {
                                program.pushUint16(0);
                            }
                        }
                    }
                    break;
                }
                case types::SyntaxKind::PropertyAssignment: {
                    const auto n = to<PropertyAssignment>(node);
                    if (n->initializer) {
                        handle(n->initializer, program);
                    } else {
                        program.pushOp(OP::Any, n);
                    }
                    if (n->name->kind == SyntaxKind::Identifier) {
                        program.pushStringLiteral(to<Identifier>(n->name)->escapedText, n->name);
                    } else {
                        //computed type name like `[a]: string`
                        handle(n->name, program);
                    }
                    program.pushOp(OP::PropertySignature, n->name);
                    if (n->questionToken) program.pushOp(OP::Optional);
                    if (hasModifier(n, SyntaxKind::ReadonlyKeyword)) program.pushOp(OP::Readonly);
                    break;
                }
                case types::SyntaxKind::PropertySignature: {
                    const auto n = to<PropertySignature>(node);
                    if (n->type) {
                        handle(n->type, program);
                    } else {
                        program.pushOp(OP::Any);
                    }
                    if (n->name->kind == SyntaxKind::Identifier) {
                        program.pushStringLiteral(to<Identifier>(n->name)->escapedText, n->name);
                    } else {
                        //computed type name like `[a]: string`
                        handle(n->name, program);
                    }
                    program.pushOp(OP::PropertySignature, node);
                    if (n->questionToken) program.pushOp(OP::Optional);
                    if (hasModifier(n, SyntaxKind::ReadonlyKeyword)) program.pushOp(OP::Readonly);
                    break;
                }
                case types::SyntaxKind::InterfaceDeclaration: {
                    const auto n = to<InterfaceDeclaration>(node);
                    program.pushFrame();

                    //first all extend expressions
                    if (n->heritageClauses) {
                        for (auto &&node: n->heritageClauses->list) {
                            auto heritage = to<HeritageClause>(node);
                            if (heritage->token == SyntaxKind::ExtendsKeyword) {
                                for (auto &&extendType: heritage->types->list) {
                                    handle(extendType, program);
                                }
                            }
                        }
                    }

                    for (auto &&member: n->members->list) {
                        handle(member, program);
                    }

                    program.pushOp(OP::ObjectLiteral, n->name);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::TypeLiteral: {
                    const auto n = to<TypeLiteralNode>(node);
                    program.pushFrame();

                    for (auto &&member: n->members->list) {
                        handle(member, program);
                    }

                    program.pushOp(OP::ObjectLiteral, node);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::ParenthesizedExpression: {
                    const auto n = to<ParenthesizedExpression>(node);
                    handle(n->expression, program);
                    break;
                }
                case types::SyntaxKind::ExpressionWithTypeArguments: {
                    const auto n = to<ExpressionWithTypeArguments>(node);
                    auto typeArgumentsCount = n->typeArguments ? n->typeArguments->length() : 0;
                    if (n->typeArguments) {
                        for (auto &&sub: n->typeArguments->list) handle(sub, program);
                    }

                    handle(n->expression, program);

                    if (n->typeArguments) {
                        program.pushOp(OP::Instantiate, node);
                        program.pushUint16(typeArgumentsCount);
                    }
                    break;
                }
                case types::SyntaxKind::ObjectLiteralExpression: {
                    const auto n = to<ObjectLiteralExpression>(node);
                    program.pushFrame();
                    for (auto &&sub: n->properties->list) handle(sub, program);
                    program.pushOp(OP::ObjectLiteral, node);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::CallExpression: {
                    const auto n = to<CallExpression>(node);
                    auto typeArgumentsCount = n->typeArguments ? n->typeArguments->length() : 0;
                    if (n->typeArguments) {
                        for (auto &&sub: n->typeArguments->list) handle(sub, program);
                    }

                    handle(n->expression, program);

                    if (n->typeArguments) {
                        program.pushOp(OP::Instantiate, node);
                        program.pushUint16(typeArgumentsCount);
                    }

                    auto argumentsCount = n->arguments->length();
                    for (auto &&sub: n->arguments->list) handle(sub, program);

                    program.pushOp(OP::CallExpression, node);
                    program.pushUint16(argumentsCount);

                    break;
                }
                case types::SyntaxKind::ExpressionStatement: {
                    const auto n = to<ExpressionStatement>(node);
                    handle(n->expression, program);
                    break;
                }
                case types::SyntaxKind::ConditionalExpression: {
                    const auto n = to<ConditionalExpression>(node);
                    //it seems TS does not care about the condition. the result is always a union of false/true branch.
                    //we could improve that though to make sure that const-expressions are handled
                    program.pushFrame();
                    handle(n->whenFalse, program);
                    handle(n->whenTrue, program);
                    program.pushOp(OP::Union, node);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::ConditionalType: {
                    const auto n = to<ConditionalTypeNode>(node);
                    //Depending on whether this a distributive conditional type or not, the whole conditional type has to be moved to its own function
                    //so it can be executed for each union member.
                    // - the `checkType` is a simple identifier (just `T`, no `[T]`, no `T | x`, no `{a: T}`, etc)
//                    let distributiveOverIdentifier: Identifier | undefined = isTypeReferenceNode(narrowed.checkType) && isIdentifier(narrowed.checkType.typeName) ? narrowed.checkType.typeName : undefined;
                    sharedOpt<Identifier> distributiveOverIdentifier = isTypeReferenceNode(n->checkType) && isIdentifier(to<TypeReferenceNode>(n->checkType)->typeName) ? to<Identifier>(to<TypeReferenceNode>(n->checkType)->typeName) : nullptr;

                    if (distributiveOverIdentifier) {
//                        program.pushSymbol(distributiveOverIdentifier->escapedText, SymbolType::TypeVariable, distributiveOverIdentifier->pos);
//                        handle(n->checkType, program);
//                        program.pushFrame();
//                        //first we add to the stack the origin type we distribute over.
//                        handle(narrowed.checkType, program);
//
//                        //since the distributive conditional type is a loop that changes only the found `T`, it is necessary to add that as variable,
//                        //so call convention can take over.
//                        program.pushVariable(getIdentifierName(distributiveOverIdentifier));
//                        program.pushCoRoutine();
                        program.pushSubroutineNameLess(node);

                        //in the subroutine of the conditional type we place a new type variable, which acts as input.
                        //the `Distribute` OP makes then sure that the current stack entry is used as input
                        program.pushSymbol(distributiveOverIdentifier->escapedText, SymbolType::TypeVariable, distributiveOverIdentifier);
                        program.pushOp(instructions::TypeArgument, distributiveOverIdentifier);
                    }

                    handle(n->checkType, program);
                    handle(n->extendsType, program);
                    program.pushOp(instructions::Extends, n);

                    auto trueProgram = program.pushSubroutineNameLess(n->trueType);
                    handle(n->trueType, program);
                    program.popSubroutine();

                    auto falseProgram = program.pushSubroutineNameLess(n->falseType);
                    handle(n->falseType, program);
                    program.popSubroutine();

                    program.pushOp(OP::JumpCondition);
                    //todo increase to 32bit each
                    program.pushUint16(trueProgram);
                    program.pushUint16(falseProgram);

                    if (distributiveOverIdentifier) {
                        auto routine = program.popSubroutine();
                        handle(n->checkType, program); //LOADS the input type onto the stack. Distribute pops it then.
                        program.pushOp(OP::Distribute);
                        program.pushAddress(routine->index);
                    }

//                    debug("ConditionalType {}", !!distributiveOverIdentifier);
                    break;
                }
                case types::SyntaxKind::ParenthesizedType: {
                    handle(to<ParenthesizedTypeNode>(node)->type, program);
                    break;
                }
                case types::SyntaxKind::RestType: {
                    handle(to<RestTypeNode>(node)->type, program);
                    program.pushOp(OP::Rest, node);
                    break;
                }
//                case types::SyntaxKind::OptionalType: {
//
//                    break;
//                }
                    //value inference
                case types::SyntaxKind::ArrayLiteralExpression: {
                    program.pushFrame();
                    for (auto &&v: to<ArrayLiteralExpression>(node)->elements->list) {
                        handle(v, program);
                        program.pushOp(OP::TupleMember, v);
                    }
                    program.pushOp(OP::Tuple, node);
                    program.popFrameImplicit();
                    //todo: handle `as const`, widen if not const
                    break;
                }
                case types::SyntaxKind::ArrayType: {
                    auto n = to<ArrayTypeNode>(node);
                    handle(n->elementType, program);
                    program.pushOp(OP::Array, node);
                    break;
                }
                case types::SyntaxKind::TupleType: {
                    program.pushFrame();
                    auto n = to<TupleTypeNode>(node);
                    for (auto &&e: n->elements->list) {
                        if (auto tm = to<NamedTupleMember>(e)) {
                            handle(tm->type, program);
                            if (tm->dotDotDotToken) program.pushOp(OP::Rest);
                            program.pushOp(OP::TupleMember, tm);
                            if (tm->questionToken) program.pushOp(OP::Optional);
                        } else if (auto ot = to<OptionalTypeNode>(e)) {
                            handle(ot->type, program);
                            program.pushOp(OP::TupleMember, ot);
                            program.pushOp(OP::Optional);
                        } else {
                            handle(e, program);
                            program.pushOp(OP::TupleMember, e);
                        }
                    }
                    program.pushOp(OP::Tuple, node);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::BinaryExpression: {
                    //e.g. `var = ''`, `foo.bar = 1`
                    auto n = to<BinaryExpression>(node);
                    switch (n->operatorToken->kind) {
                        case types::SyntaxKind::EqualsToken: {

                            if (n->left->kind == types::SyntaxKind::Identifier) {
                                const auto name = to<Identifier>(n->left)->escapedText;
                                auto symbol = program.findSymbol(name);
                                if (!symbol) {
                                    program.pushOp(OP::Never, n->left);
                                    program.pushError(ErrorCode::CannotFind, n->left);
                                } else {
                                    if (!symbol->routine) throw runtime_error("Symbol has no routine");

                                    handle(n->right, program);
                                    program.pushOp(OP::Set, n->operatorToken);
                                    program.pushAddress(symbol->routine->index);
                                }
                            } else {
                                throw runtime_error("BinaryExpression left only Identifier implemented");
                            }

                            break;
                        }
                        default:
                            throw runtime_error(fmt::format("BinaryExpression Operator token {} not handled", n->operatorToken->kind));
                    }
                    break;
                }
                case types::SyntaxKind::VariableStatement: {
                    for (auto &&s: to<VariableStatement>(node)->declarationList->declarations->list) {
                        handle(s, program);
                    }
                    break;
                }
                case types::SyntaxKind::VariableDeclaration: {
                    const auto n = to<VariableDeclaration>(node);
                    if (const auto id = to<Identifier>(n->name)) {
                        auto &symbol = program.pushSymbolForRoutine(id->escapedText, SymbolType::Variable, id); //move this to earlier symbol-scan round
                        if (symbol.declarations>1) {
                            //todo: embed error since variable is declared twice
                        } else {
                            if (n->type) {
                                const auto subroutineIndex = program.pushSubroutine(id->escapedText);
//                                program.pushSourceMap(id);
                                handle(n->type, program);
                                program.popSubroutine();
                                if (n->initializer) {
                                    handle(n->initializer, program);
                                    program.pushOp(OP::Call);
                                    program.pushAddress(subroutineIndex);
                                    program.pushUint16(0);
                                    program.pushOp(OP::Assign, n->name);
                                }
                            } else {
                                auto subroutineIndex = program.pushSubroutine(id->escapedText);

                                if (n->initializer) {
                                    handle(n->initializer, program);
                                    //let var1 = true; //boolean
                                    //const var1 = true; //true
                                    if (!n->isConst()) {
                                        program.pushOp(OP::Widen);
                                    }
                                    program.popSubroutine();

                                    if (!n->isConst()) {
                                        //set current narrowed to initializer
                                        handle(n->initializer, program);
                                        program.pushOp(OP::Set);
                                        program.pushAddress(subroutineIndex);
                                    }
                                } else {
                                    program.pushOp(OP::Any);
                                    program.popSubroutine();
                                }
                            }
                        }
                    } else {
                        debug("No identifier in name");
                    }
                    break;
                }
                default: {
                    debug("Node {} not handled", node->kind);
                }
            }
        }
    };
}