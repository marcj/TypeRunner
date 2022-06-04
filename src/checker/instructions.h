#pragma once

namespace ts::instructions {
    enum OP {
        //type related
        Never,
        Any,
        Unknown,
        Void, //void is reserved word
        Object,

        String,
        Number,
        Boolean,
        BigInt,

        Symbol,
        Null,
        Undefined,


        StringLiteral,
        NumberLiteral,
        BigIntLiteral,
        True,
        False,

        Function,

        Method,
        MethodSignature,

        Parameter,

        Property,
        PropertySignature,

        Class,

        Union,
        Intersection,

        /**
         * Stack parameter. For each JS variable, JS function, as well as type variables (mapped-type variable for example).
         *
         * Parameters:
         *  1. address on initial stack frame, which should contain its name as a string.
         *  3. modifier: const
         *  2. position in source code. necessary to determine if a reference is made to a const symbol before it was defined.
         */
        Var,

        /**
         * Reserves a type variable on the stack, which contains a type object. Unknown as default.
         *
         * The address of it is known in the program and referenced directly.
         */
        TypeVar,
        Loads, //pushes to the stack a referenced type in the stack. has 2 parameters: <frame> <index>, frame is a negative offset to the frame, and index the index of the stack entry withing the referenced frame
        Assign,

        Frame, //creates a new stack frame
        Return,

        Jump,
        Call //call a subroutine and push the result on the stack
    };
}

template <> struct fmt::formatter<ts::instructions::OP> : formatter<std::string_view> {
    template <typename FormatContext> auto format(ts::instructions::OP p, FormatContext &ctx) {
        return formatter<string_view>::format(magic_enum::enum_name<ts::instructions::OP>(p), ctx);
    }
};
