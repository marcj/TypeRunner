#include <string>
#include <span>

#include "./app.h"
#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/vm.h"
#include "../checker/debug.h"
#include "TextEditor.h"

using std::string;
using std::span;

using namespace ts;
using namespace ts::gui;

typedef std::chrono::duration<double, std::milli> took;

struct ExecutionData {
    took parseTime;
    took compileTime;
    took binaryTime;
    took checkTime;
};

int main() {
    guiApp.title = "Typhoon";
    guiAppInit();

    string fileName = "app.ts";

    TextEditor editor;
    auto lang = TextEditor::LanguageDefinition::CPlusPlus();

    lang.mKeywords.insert({"type", "extends", "string", "number", "boolean", "bigint"});

    editor.SetLanguageDefinition(lang);

    TextEditor::ErrorMarkers markers;
//    markers.insert(std::make_pair<int, std::string>(1, "Example error here:\nInclude file not found: \"TextEditor.h\""));
//    markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
    editor.SetErrorMarkers(markers);
    editor.SetShowWhitespaces(false);

    editor.SetText(R"(
// Here you can see in real-time what branch the conditional type takes
type isNumber<T> = T extends number ? df : "no";
const v2: isNumber<number> = "yes";

// Here you can see that distributive conditional types
// are executed for each union member
type NoNumber<T> = T extends number ? never : T;
type Primitive = string | number | boolean;
const v3: NoNumber<Primitive> = 34;
)");

    auto fontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNS.ttf", 32.0);
    auto fontMono = ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNSMono.ttf", 30.0);
    auto fontMonoSmall = ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNSMono.ttf", 26.0);

    checker::DebugBinResult debugBinResult;
    auto module = make_shared<vm::Module>();

    ExecutionData lastExecution;

    auto extractErrors = [&] {
        editor.inlineErrors.clear();
        for (auto &&e: module->errors) {
            auto map = e.module->findNormalizedMap(e.ip);
            auto lineChar = e.module->mapToLineCharacter(map);
            editor.inlineErrors.push_back({.data = &e, .line = (int) lineChar.line, .charPos = (int) lineChar.pos, .charEnd = (int) lineChar.end});
        }
    };

    {
        checker::Compiler compiler;
        Parser parser;
        auto result = parser.parseSourceFile(fileName, editor.GetText(), ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
        auto program = compiler.compileSourceFile(result);
        auto bin = program.build();
        module = make_shared<vm::Module>(std::move(bin), fileName, editor.GetText());
        debugBinResult = checker::parseBin(module->bin);
        extractErrors();
    }

    auto runProgram = [&] {
        checker::Compiler compiler;
        Parser parser;
        auto iterations = 1000;

        auto start = std::chrono::high_resolution_clock::now();
        shared<SourceFile> result;
        for (auto i = 0; i < iterations; i++) {
            result = parser.parseSourceFile(fileName, editor.GetText(), ts::types::ScriptTarget::Latest, false, ScriptKind::TS, {});
        }
        lastExecution.parseTime = (std::chrono::high_resolution_clock::now() - start) / iterations;

        start = std::chrono::high_resolution_clock::now();

        checker::Program program;
        for (auto i = 0; i < iterations; i++) {
            program = compiler.compileSourceFile(result);
        }
        lastExecution.compileTime = (std::chrono::high_resolution_clock::now() - start) / iterations;

        start = std::chrono::high_resolution_clock::now();
        string bin;
        for (auto i = 0; i < iterations; i++) {
            bin = program.build();
        }
        lastExecution.binaryTime = (std::chrono::high_resolution_clock::now() - start) / iterations;

        module = make_shared<vm::Module>(std::move(bin), fileName, editor.GetText());
        debugBinResult = checker::parseBin(module->bin);

        start = std::chrono::high_resolution_clock::now();

        for (auto i = 0; i < iterations; i++) {
            module->clear();
            vm::VM vm; //we need to keep Modules alive
            vm.run(module);
        }
        lastExecution.checkTime = (std::chrono::high_resolution_clock::now() - start) / iterations;

//        vm.printErrors();
        editor.highlights.clear();
        extractErrors();
    };

    editor.inlineErrorHover = [](ImVec2 &start, ImVec2 &end, TextEditor::InlineError &inlineError) {
        ImGui::BeginTooltip();
        vm::DiagnosticMessage *message = (vm::DiagnosticMessage *) inlineError.data;
        ImGui::TextUnformatted(message->message.c_str());
        ImGui::EndTooltip();
    };

    auto debugActive = false;
    auto debugEnded = false;
    vm::VM debugVM;

    runProgram();
    guiAppRender([&] {
        ImGui::PushFont(fontDefault);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(guiApp.displayWidth, guiApp.displayHeight));
        ImGui::Begin("TypeScript Debugger", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                                     ImGuiWindowFlags_NoMove);

        ImGui::SetWindowFontScale(1.2);
//        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);

        if (editor.IsTextChanged()) {
            runProgram();
        }

        ImGui::PushFont(fontMono);
        editor.Render("TextEditor");
        ImGui::PopFont();

        ImVec4 grey{0.5, 0.5, 0.5, 1};
        ImVec4 green{0.5, 0.9, 0.5, 1};
        ImVec4 yellow{1, 1, 0, 1};

        {
            ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Run", nullptr)) {
                if (ImGui::Button("Execute")) {
                    runProgram();
                    debugActive = false;
                    debugEnded = false;
                }

                auto totalCompiler = lastExecution.parseTime.count() + lastExecution.compileTime.count() + lastExecution.binaryTime.count();
                auto total = totalCompiler + lastExecution.checkTime.count();
                ImGui::PushFont(fontMonoSmall);

                ImGui::TextColored(green, fmt::format("Warm: {:.6f}ms", lastExecution.checkTime.count()).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, "Bytecode was cached on disk/memory");

                ImGui::TextColored(green, fmt::format("Cold: {:.6f}ms", total).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, "No bytecode cache");

                ImGui::TextColored(grey, "Details:");

                ImGui::BeginGroup();
                ImGui::Text(fmt::format("Compile bytecode\n{:.6f}ms", totalCompiler).c_str());

                ImGui::BeginGroup();
                ImGui::TextColored(grey, fmt::format("Parse\n{:.6f}ms", lastExecution.parseTime.count()).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, fmt::format("Compile\n{:.6f}ms", lastExecution.compileTime.count()).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, fmt::format("Packaging\n{:.6f}ms", lastExecution.binaryTime.count()).c_str());
                ImGui::EndGroup();

                ImGui::EndGroup();
                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::Text(fmt::format("Checking\n{:.6f}ms", lastExecution.checkTime.count()).c_str());
                ImGui::EndGroup();

                ImGui::PopFont();
//
//                ImGui::SameLine();
//                ImGui::BeginGroup();
//                ImGui::Text("Checking:");
//                ImGui::SameLine();
//                ImGui::PushFont(fontMono);
//                ImGui::Text("%fms", lastExecution.checkTime.count());
//                ImGui::PopFont();
//                ImGui::EndGroup();
            }
            ImGui::End();
        }

        {
            ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Diagnostics", nullptr)) {
                ImGui::BeginTable("diagnostics", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders);

                ImGui::TableSetupColumn("file", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 60);
                ImGui::TableSetupColumn("bytepos", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 60);
                ImGui::TableSetupColumn("pos", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 60);
                ImGui::TableSetupColumn("message", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
                ImGui::TableHeadersRow();

                for (auto &&e: module->errors) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text(e.module->fileName.c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", e.ip);

                    ImGui::TableNextColumn();
                    auto map = e.module->findMap(e.ip);
                    ImGui::Text("%d:%d", map.pos, map.end);

                    ImGui::TableNextColumn();
                    ImGui::TextWrapped(e.message.c_str());
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }

        {
            ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Virtual Machine", nullptr)) {
                if ((!debugActive || debugEnded) && ImGui::Button("Debug")) {
                    if (!debugActive || debugEnded) {
                        debugVM = vm::VM();
                        module->clear();
                        debugActive = true;
                        debugEnded = false;
                        editor.SetReadOnly(true);
                        debugVM.stepper = true;
                        debugVM.prepare(module);
                    }
                }

                static vm::Frame *selectedFrame = nullptr;

                if (debugActive) {
                    if (debugEnded) {
                        ImGui::Text("Program exited");
                    } else {
                        if (ImGui::Button("Next")) {
                            static vm::FoundSourceMap lastMap;
                            selectedFrame = nullptr;
                            while (true) {
                                debugVM.process();
                                if (!debugVM.subroutine) {
                                    debugEnded = true;
                                    editor.SetReadOnly(false);
                                    editor.highlights.clear();
                                    break;
                                } else {
                                    auto map = module->findNormalizedMap(debugVM.subroutine->ip);
                                    if (!map.found()) continue; //another step please
                                    if (map.pos == lastMap.pos && map.end == lastMap.end) continue; //another step please
                                    lastMap = map;
                                    auto lineChar = module->mapToLineCharacter(map);
                                    editor.highlights.clear();
                                    editor.highlights.push_back({.line = (int) lineChar.line, .charPos = (int) lineChar.pos, .charEnd = (int) lineChar.end});
                                    break;
                                }
                            }
                        }
                        if (!debugEnded) {
                            ImGui::SameLine();
                            if (ImGui::Button("Stop")) {
                                editor.SetReadOnly(false);
                                debugActive = false;
                                debugEnded = true;
                                debugVM = vm::VM();
                                runProgram();
                            }

                            ImGui::SameLine();
                            ImGui::TextColored(green, "*Active*");
                        }
                    }

                    if (debugVM.subroutine) {

                        auto current = debugVM.frame;
                        vector<vm::Frame *> frames;
                        while (current) {
                            frames.push_back(current);
                            current = current->previous;
                        }
                        ImGui::Text("Stack (%d/%d)", frames.size(), debugVM.stack.size());

                        static auto showNonVariables = false;

                        if (!selectedFrame) selectedFrame = frames.front();

                        ImGui::Checkbox("Show all stack entries", &showNonVariables);

                        ImGui::PushItemWidth(120);
                        if (ImGui::BeginListBox("###listbox")) {
                            auto i = 0;
                            string_view lastName = "main";
                            for (auto it = frames.rbegin(); it != frames.rend(); it++) {
                                auto frame = (*it);
                                ImGui::PushID(i);

                                if (!frame->subroutine->name.empty()) {
                                    lastName = frame->subroutine->name;
                                }
                                if (ImGui::Selectable((string(lastName)).c_str(), selectedFrame == frame)) {
                                    selectedFrame = frame;
                                }

                                ImGui::SameLine();
                                ImGui::TextColored(grey, to_string(showNonVariables ? frame->size() : frame->variables).c_str());

                                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                if (selectedFrame == frame) ImGui::SetItemDefaultFocus();
                                i++;
                                ImGui::PopID();
                            }
                            ImGui::EndListBox();
                        }

                        ImGui::PopItemWidth();

                        if (selectedFrame) {
                            ImGui::SameLine();
                            ImGui::BeginGroup();
                            auto start = selectedFrame->initialSp; // + frame->variables;
                            span<shared<vm::Type>> frameStack{debugVM.stack.data() + start, selectedFrame->sp - start};

                            for (unsigned int i = 0; i < frameStack.size(); i++) {
                                auto type = frameStack[i];
                                if (i >= selectedFrame->variables && !showNonVariables) continue;

                                ImGui::Text("    ");
                                ImGui::SameLine();
                                if (i < selectedFrame->variables) {
                                    auto ip = selectedFrame->variableIPs[i];
                                    auto identifier = module->findIdentifier(ip);
                                    ImGui::Text(identifier.c_str());
                                    ImGui::SameLine();
                                }
                                auto stype = vm::stringify(type);
                                if (stype.size() > 20) stype = stype.substr(0, 20) + "...";
                                ImGui::TextColored(grey, stype.c_str());
                            }
                            ImGui::EndGroup();
                        }
                    }
                }
            }

            ImGui::End();
        }

        {
            ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Bytecode", nullptr)) {
                //show storage
                ImGui::Text("Size: ");
                ImGui::SameLine();
                ImGui::PushFont(fontMono);
                ImGui::TextWrapped((to_string(module->bin.size()) + "bytes").c_str());
                ImGui::PopFont();

                ImGui::Text("Storage: ");
                ImGui::SameLine();
                string storage = "";
                for (auto &&s: debugBinResult.storages) {
                    storage += "\"" + s + "\" ";
                }
                ImGui::PushFont(fontMono);
                ImGui::TextWrapped(storage.c_str());
                ImGui::PopFont();
//                ImGui::Spacing();

                //show subroutines + ops
                ImGui::Text("Subroutines");
                ImGui::BeginTable("subroutines2", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders);

                ImGui::TableSetupColumn("idx", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 30);
                ImGui::TableSetupColumn("bytepos", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 40);
                ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 70);
                ImGui::TableSetupColumn("OPs", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
                ImGui::TableHeadersRow();

                ImGui::PushFont(fontMonoSmall);
                auto i = 0;
                for (auto &&s: debugBinResult.subroutines) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text(to_string(i).c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text(to_string(s.address).c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text(s.name.c_str());

                    ImGui::TableNextColumn();
                    for (auto &&op: s.operations) {
                        ImGui::TextColored(grey, to_string(op.address).c_str());
                        ImGui::SameLine();
                        if (debugVM.subroutine && debugVM.subroutine->ip == op.address) {
                            ImGui::TextColored(yellow, op.text.c_str());
                        } else {
                            ImGui::Text(op.text.c_str());
                        }
                    }
                    i++;
                }
                ImGui::PopFont();
                ImGui::EndTable();


                //show subroutines + ops
                ImGui::Text("Sourcemap");
                ImGui::BeginTable("sourcemap", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders);

                ImGui::TableSetupColumn("bytepos", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 30);
                ImGui::TableSetupColumn("OP", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 120);
                ImGui::TableSetupColumn("source", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 60);
                ImGui::TableHeadersRow();

                ImGui::PushFont(fontMonoSmall);
                for (auto &&m: debugBinResult.sourceMap) {
                    ImGui::TableNextRow();

                    ImGui::TableNextColumn();
                    ImGui::Text("%d", m.bytecodePos);

                    ImGui::TableNextColumn();
                    ImGui::Text(string(magic_enum::enum_name<ts::instructions::OP>(m.op)).c_str());

                    ImGui::TableNextColumn();
                    ImGui::Text((to_string(m.sourcePos) + ":" + to_string(m.sourceEnd)).c_str());
                }
                ImGui::PopFont();
                ImGui::EndTable();
            }
            ImGui::End();
        }

        ImGui::End();
        ImGui::PopFont();
    });
}