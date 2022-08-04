#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <memory>

#include "../core.h"
#include "../hash.h"
#include "../checker/compiler.h"
#include "../checker/vm2.h"
#include "./utils.h"

using namespace ts;
using namespace ts::vm2;

using std::string;
using std::string_view;

TEST_CASE("bigUnion") {
    ts::checker::Program program;

    auto foos = 300;

    for (auto i = 0; i<foos; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage((new string("foo"))->append(to_string(i)));
        program.pushOp(OP::StringLiteral);
        program.pushStorage("a");
        program.pushOp(OP::PropertySignature);
        program.pushOp(OP::ObjectLiteral);
        program.pushUint16(1);
        program.pushOp(OP::TupleMember);
    }
    program.pushOp(OP::Tuple);
    program.pushUint16(foos);

    for (auto i = 0; i<foos; i++) {
        program.pushOp(OP::StringLiteral);
        program.pushStorage((new string("foo"))->append(to_string(i)));
    }
    program.pushOp(OP::Union);
    program.pushUint16(foos);

    program.pushOp(OP::StringLiteral);
    program.pushStorage("a");
    program.pushOp(OP::PropertySignature);
    program.pushOp(OP::ObjectLiteral);
    program.pushUint16(1);
    program.pushOp(OP::Array);

    program.pushOp(OP::Assign);
    program.pushOp(OP::Halt);

    auto module = std::make_shared<vm2::Module>(program.build(), "app.ts", "");
    run(module);
    run(module);
    module->printErrors();
    REQUIRE(module->errors.size() == 0);

    debug("pool.active = {}", ts::vm2::pool.active);
    debug("poolRef.active = {}", ts::vm2::poolRef.active);
    ts::vm2::clear(module);
    ts::vm2::gcStackAndFlush();
    REQUIRE(ts::vm2::pool.active == 0);
    REQUIRE(ts::vm2::poolRef.active == 0);

    ts::bench("first", 1000, [&] {
        module->clear();
        run(module);
    });
}
