#include <string>

#include "./app.h"
#include "../parser2.h"
#include "../checker/compiler.h"
#include "../checker/vm.h"
#include "../checker/debug.h"
#include "TextEditor.h"

using std::string;

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
    guiAppInit();

    string fileName = "app.ts";

    TextEditor editor;
    auto lang = TextEditor::LanguageDefinition::CPlusPlus();
    lang.mKeywords.insert("type");
    editor.SetLanguageDefinition(lang);

    TextEditor::ErrorMarkers markers;
//    markers.insert(std::make_pair<int, std::string>(1, "Example error here:\nInclude file not found: \"TextEditor.h\""));
//    markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
    editor.SetErrorMarkers(markers);

    editor.SetText("type a<T> = T;const v2: a<number> = \"as\";\n\n");

    auto fontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNS.ttf", 32.0);
    auto fontMono = ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNSMono.ttf", 32.0);
    auto fontMonoSmall = ImGui::GetIO().Fonts->AddFontFromFileTTF("/System/Library/Fonts/SFNSMono.ttf", 26.0);

    checker::DebugBinResult debugBinResult;
    auto module = make_shared<vm::Module>();

    ExecutionData lastExecution;

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

        editor.inlineErrors.clear();
        for (auto &&e: module->errors) {
            auto map = e.module->findNormalizedMap(e.ip);
            auto lineChar = e.module->mapToLineCharacter(map);
            editor.inlineErrors.push_back({.data = &e, .line = (int) lineChar.line, .charPos = (int) lineChar.pos, .charEnd = (int) lineChar.end});
        }
    };

    editor.inlineErrorHover = [](ImVec2 &start, ImVec2 &end, TextEditor::InlineError &inlineError) {
        ImGui::BeginTooltip();
        vm::DiagnosticMessage *message = (vm::DiagnosticMessage *) inlineError.data;
        ImGui::TextUnformatted(message->message.c_str());
        ImGui::EndTooltip();
    };

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

        {
            ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Run", nullptr)) {
                if (ImGui::Button("Execute")) {
                    runProgram();
                }

                auto totalCompiler = lastExecution.parseTime.count() + lastExecution.compileTime.count() + lastExecution.binaryTime.count();
                auto total = totalCompiler + lastExecution.checkTime.count();
                ImGui::PushFont(fontMonoSmall);

                ImGui::TextColored(green, fmt::format("Warm: {}ms", lastExecution.checkTime.count()).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, "Bytecode was cached on disk/memory");

                ImGui::TextColored(green, fmt::format("Cold: {}ms", total).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, "No bytecode cache");


                ImGui::TextColored(grey, "Details:");

                ImGui::BeginGroup();
                ImGui::Text(fmt::format("Compile\n{}ms", totalCompiler).c_str());

                ImGui::BeginGroup();
                ImGui::TextColored(grey, fmt::format("Parse\n{}ms", lastExecution.parseTime.count()).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, fmt::format("Compile\n{}ms", lastExecution.compileTime.count()).c_str());
                ImGui::SameLine();
                ImGui::TextColored(grey, fmt::format("Packaging\n{}ms", lastExecution.binaryTime.count()).c_str());
                ImGui::EndGroup();

                ImGui::EndGroup();
                ImGui::SameLine();

                ImGui::BeginGroup();
                ImGui::Text(fmt::format("Checking\n{}ms", lastExecution.checkTime.count()).c_str());
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
            if (ImGui::Begin("Bytecode", nullptr)) {
                //show storage
                string size = "Size: " + to_string(module->bin.size()) + "bytes";
                ImGui::TextWrapped(size.c_str());
                string storage = "Storage: ";
                for (auto &&s: debugBinResult.storages) {
                    storage += s + " ";
                }
                ImGui::TextWrapped(storage.c_str());
                ImGui::Spacing();

                //show subroutines + ops
                ImGui::Text("Subroutines");
                ImGui::BeginTable("subroutines2", 4, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_Resizable | ImGuiTableFlags_Borders);

                ImGui::TableSetupColumn("idx", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 30);
                ImGui::TableSetupColumn("bytepos", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 40);
                ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 70);
                ImGui::TableSetupColumn("OPs", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoSort);
                ImGui::TableHeadersRow();

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
                    string ops = "";
                    for (auto &&op: s.operations) {
                        ops += op + " ";
                    }
                    ImGui::PushFont(fontMonoSmall);
                    ImGui::TextWrapped(ops.c_str());
                    ImGui::PopFont();
                    i++;
                }
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