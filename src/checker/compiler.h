#pragma

#include <string>
#include <functional>
#include <utility>

#include "../types.h"
#include "./instructions.h"
#include "./utils.h"

namespace ts::checker {

    using std::string;
    using std::function;
    using instructions::OP;

    enum class SymbolType {
        Variable,
        Function,
        Class,
        Type
    };

    struct Symbol {
        const string_view name;
        const SymbolType type;
        const unsigned int pos;

        const unsigned int subroutineIndex = 0;
    };

    //A subroutine is a sub program that can be executed by knowing its address.
    //They are used for example for type alias, mapped type, conditional type (for false and true side)
    struct Subroutine {
        vector<unsigned char> ops; //OPs, and its parameters
        unordered_map<unsigned int, unsigned int> opSourceMap;
        string_view identifier;
        unsigned int address; //final address in the binary
        unsigned int pos{};

        explicit Subroutine(string_view &identifier): identifier(identifier) {}
    };

    struct Frame {
        const bool conditional = false;
        shared<Frame> previous;
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

    class Program {
    public:
        vector<unsigned char> ops; //OPs of "main"
        unordered_map<unsigned int, unsigned int> opSourceMap;

        vector<string_view> storage; //all kind of literals, as strings
        unordered_map<string_view, reference_wrapper<StorageItem>> storageMap; //used to deduplicated storage entries
        unsigned int storageIndex{};
        shared<Frame> frame = make_shared<Frame>();

        //tracks which subroutine is active (end() is), so that pushOp calls are correctly assigned.
        vector<Subroutine *> activeSubroutines;
        vector<Subroutine> subroutines;

        //implicit is when a OP itself triggers in the VM a new frame, without having explicitly a OP::Frame
        shared<Frame> pushFrame(bool implicit = false) {
            if (!implicit) this->pushOp(OP::Frame);
            frame = make_shared<Frame>(frame);
            return frame;
        }

        unsigned int pushSubroutine(string_view name) {
            //find subroutine
            for (auto &&s: frame->symbols) {
                if (s.name == name) {
                    pushFrame(true); //subroutines have implicit stack frames due to call convention
                    activeSubroutines.push_back(&subroutines[s.subroutineIndex]);
                    return s.subroutineIndex;
                }
            }
            throw runtime_error(fmt::format("no symbol found for {}", name));
        }

        /**
         * Returns the index of the sub routine. Will be replaced in build() with the real address.
         */
        void popSubroutine() {
            if (activeSubroutines.empty()) throw runtime_error("No active subroutine found");
            popFrameImplicit();
            auto subroutine = activeSubroutines.back();
            subroutine->ops.push_back(OP::Return);
            activeSubroutines.pop_back();
        }

        /**
         * The returning index will be replaced for all Call OP with the real subroutine address.
         */
        unsigned int findSubroutineIndex(const string_view &identifier) {
            //todo: this should go through frame->symbols, because there could be multiple subroutines with the same name
            Frame *current = frame.get();

            while (true) {
                for (auto &&s: current->symbols) {
                    if (s.name == identifier) {
                        return s.subroutineIndex;
                    }
                }
                if (!current->previous) break;
                current = current->previous.get();
            };

            throw runtime_error(fmt::format("No subroutine for {} found", identifier));
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
            writeUint32(ops, ops.size(), address);
        }

        vector<unsigned char> &getOPs() {
            if (activeSubroutines.size()) return activeSubroutines.back()->ops;
            return ops;
        }

        void pushOp(OP op, std::vector<unsigned int> params = {}) {
            auto &ops = getOPs();
            ops.push_back(op);
            ops.insert(ops.end(), params.begin(), params.end());
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
        void pushSymbol(string_view name, SymbolType type, unsigned int pos, shared<Frame> frameToUse = nullptr) {
            if (!frameToUse) frameToUse = frame;

            Subroutine routine{name};
            routine.pos = pos;
            frameToUse->symbols.push_back(Symbol{.name = name, .subroutineIndex = (unsigned int)subroutines.size()});

            subroutines.push_back(std::move(routine));

//            const unsigned int index = frameToUse->symbols.size();
//            frameToUse->symbols.push_back(Symbol{.name = name, .index = index});
//
//            const auto nameAddress = registerStorage(name);
//
//            ops.insert(ops.begin() + frameToUse->headerOffset, OP::Var);
//            ops.insert(ops.begin() + frameToUse->headerOffset + 1, {0, 0, 0, 0}); //is there a faster way?
//            writeUint(ops, frameToUse->headerOffset + 1, nameAddress);
//
//            unsigned int frameOffset = 0;
//            shared<Frame>& current = frame;
//
//            while (current) {
//                current->headerOffset += 5;  //for each frame in between active `frame` and including `frameToUse` we have to increase headerOffset
//
//                if (current == frameToUse) break;
//                frameOffset++;
//                current = current->previous;
//            }
//
////            opSourceMap[address] = pos; //todo sourcemap
//
//            return FrameOffset{.frame = frameOffset, .symbol = index};
        }

        //note: make sure the same name is not added twice. needs hashmap
        unsigned int registerStorage(const string_view &s) {
            if (!storageIndex) storageIndex = 5; //jump+address

            const auto address = storageIndex;
            storage.push_back(s);
            storageIndex += s.size();
            return address;
        }

        void pushStorage(const string_view &s) {
            pushAddress(registerStorage(s));
        }

        string_view findStorage(unsigned int index) {
            unsigned int i = 2;
            for (auto &&s: storage) {
                if (i == index) return s;
                i += s.size();
            }
            return "!unknown";
        }

        vector<unsigned char> build() {
            vector<unsigned char> bin;
            unsigned int address = 0;

            if (storage.size() || subroutines.size()) {
                address = 5; //we add JUMP + index when building the program to jump over all subroutines&storages
                bin.push_back(OP::Jump);
                writeUint32(bin, bin.size(), 0); //set after routine handling
            }

            for (auto &&item: storage) {
                writeUint16(bin, address, item.size());
                bin.insert(bin.end(), item.begin(), item.end());
                address += 2 + item.size();
            }

            //detect final binary address of all subroutines
            unsigned int routineAddress = address;
            for (auto &&routine: subroutines) {
                routine.address = routineAddress;
                routineAddress += routine.ops.size();
            }

            //go through all OPs and adjust CALL parameter to the final binary address
            setFinalBinaryAddress(ops);
            for (auto &&routine: subroutines) {
                setFinalBinaryAddress(routine.ops);
            }

            for (auto &&routine: subroutines) {
                bin.insert(bin.end(), routine.ops.begin(), routine.ops.end());
                address += routine.ops.size();
            }

            writeUint32(bin, 1, address);

            bin.insert(bin.end(), ops.begin(), ops.end());

            return bin;
        }

        void setFinalBinaryAddress(vector<unsigned char> &ops) {
            const auto end = ops.size();
            for (unsigned int i = 0; i < end; i++) {
                auto op = (OP) ops[i];
                switch (op) {
                    case OP::Call: {
                        //adjust binary address
                        auto index = readUint32(ops, i);
                        auto &routine = subroutines[index];
                        writeUint32(ops, i, routine.address);
                        i += 4;
                        break;
                    }
                    case OP::Assign:
                    case OP::NumberLiteral:
                    case OP::BigIntLiteral:
                    case OP::StringLiteral: {
                        i += 4;
                        break;
                    }
                }
            }
        }

        void printOps(vector<unsigned char> ops) {
            const auto end = ops.size();
            for (unsigned int i = 0; i < end; i++) {
                auto op = (OP) ops[i];
                std::string params = "";
                switch (op) {
                    case OP::Assign: {
                        params += fmt::format(" &{}", readUint32(ops, i + 1));
                        i += 4;
                        break;
                    }
                    case OP::NumberLiteral:
                    case OP::BigIntLiteral:
                    case OP::StringLiteral: {
                        params += fmt::format(" \"{}\"", findStorage(readUint32(ops, i + 1)));
                        i += 4;
                        break;
                    }
                }

                if (params.empty()) {
                    fmt::print("{} ", op);
                } else {
                    fmt::print("({}{}) ", op, params);
                }
            }
            fmt::print("\n");
        }

        void print() {
            int i = 0;
            for (auto &&subroutine: subroutines) {
                fmt::print("Subroutine {} &{}, {} bytes: ", subroutine.identifier, i++, subroutine.ops.size());
                printOps(subroutine.ops);
            }

            debug("Main {} bytes: {}", ops.size(), ops);
            printOps(ops);
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
                    for (auto &&statement: node->to<SourceFile>().statements->list) {
                        handle(statement, program);
                    }
                    break;
                }
                case types::SyntaxKind::BooleanKeyword: program.pushOp(OP::Boolean);
                    break;
                case types::SyntaxKind::StringKeyword: program.pushOp(OP::String);
                    break;
                case types::SyntaxKind::NumberKeyword: program.pushOp(OP::Number);
                    break;
                case types::SyntaxKind::BigIntLiteral: program.pushOp(OP::BigIntLiteral);
                    program.pushStorage(to<BigIntLiteral>(node)->text);
                    break;
                case types::SyntaxKind::NumericLiteral: program.pushOp(OP::NumberLiteral);
                    program.pushStorage(to<NumericLiteral>(node)->text);
                    break;
                case types::SyntaxKind::StringLiteral: program.pushOp(OP::StringLiteral);
                    program.pushStorage(to<StringLiteral>(node)->text);
                    break;
                case types::SyntaxKind::TrueKeyword: program.pushOp(OP::True);
                    break;
                case types::SyntaxKind::FalseKeyword: program.pushOp(OP::False);
                    break;
                case types::SyntaxKind::UnionType: {
                    const auto n = to<UnionTypeNode>(node);
                    program.pushFrame();

                    for (auto &&s: n->types->list) {
                        handle(s, program);
                    }

                    program.pushOp(OP::Union);
                    program.popFrameImplicit();
                    break;
                }
                case types::SyntaxKind::TypeReference: {
                    //todo: search in symbol table and get address
//                    debug("type reference {}", to<TypeReferenceNode>(node)->typeName->to<Identifier>().escapedText);
//                    program.pushOp(OP::Number);

                    program.pushOp(OP::Call);
                    const auto name = to<TypeReferenceNode>(node)->typeName->to<Identifier>().escapedText;
                    program.pushAddress(program.findSubroutineIndex(name));
                    break;
                }
                case types::SyntaxKind::TypeAliasDeclaration: {
                    const auto n = to<TypeAliasDeclaration>(node);

                    //1. create new subroutine
                    program.pushSubroutine(n->name->escapedText);

                    //2. extract type parameters

                    //3. extract type
                    handle(n->type, program);

                    program.popSubroutine();
                    break;
                }
                case types::SyntaxKind::FunctionDeclaration: {
                    const auto n = to<FunctionDeclaration>(node);
                    if (const auto id = to<Identifier>(n->name)) {
                        program.pushSymbol(id->escapedText, SymbolType::Function, id->pos); //move this to earlier symbol-scan round
                        program.pushSubroutine(id->escapedText);

                        for (auto &&param: n->parameters->list) {
                            handle(n, program);
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

                        program.pushOp(OP::Function);
                        program.popSubroutine();
                    } else {
                        debug("No identifier in name");
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
                        program.pushSymbol(id->escapedText, SymbolType::Variable, id->pos); //move this to earlier symbol-scan round
                        const auto subroutineIndex = program.pushSubroutine(id->escapedText);

                        if (n->type) {
                            handle(n->type, program);
                        } else {
                            program.pushOp(OP::Unknown);
                        }
                        program.popSubroutine();

                        if (n->initializer) {
                            //varName = initializer
                            handle(n->initializer, program);
                            program.pushOp(OP::Assign);
                            program.pushAddress(subroutineIndex);
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