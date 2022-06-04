#pragma once

#include "./type_objects.h"
#include "../core.h"

#include <memory>

namespace ts {
    using namespace ts::checker;
    using std::unique_ptr;

    class VM {
    public:
        unique_ptr<Type> process() {

        }
    };
}