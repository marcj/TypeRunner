# TypeRunner

High-performance TypeScript compiler.

## Goal

- Parser
- Type checking (as CLI and as library)
- Language Server
- Interactive type debugging
- Type information in other languages
- (optional) transpiling to JavaScript
- (optional) RTTI in JavaScript

The goal is to make TypeScript type checking as fast as possible and provide alongside with it a native TS library for other languages, so they can use TypeScript type information
without the need for a JavaScript engine for all sorts of use cases like JSON-Schema replacement, ORM DSL, encoding information (like Protocol Buffers schema) and more.

The goal is not to be a drop-in replacement for the entire official TypeScript compiler (tsc). TypeScript just supports so much that is not always necessary. 
We focus on the more strict TS part which means TypeRunner won't support JSDoc and a lot of compiler options.  

## Status

The source code in the initial version is really only a proof of concept. It consists of roughly 30k LoC and shows very promising results.
The approach is a TS-to-bytecode compiler and then run the bytecode in a custom virtual machine.
The data show that this approach can lead to a hundred- to several-thousand-fold improvement in speed.

-- todo show data as graphs, maybe even make auto-generated --

![TypeRunner Debugger](./docs/typerunner-debugger.png)

Once the project gets funding through the community, the development will continue.

## Development

TypeRunner is written in modern C++ with cmake, doctest, imgui, tracy, fmt. To work on this project first clone the repository:

```sh
$ git clone git@github.com:marcj/TypeRunner.git
$ cd TypeRunner
```

then make sure cmake and a C++ compiler is installed. We use LLVM toolchain per default. To build the project run the usual cmake command:

```sh
$ mkdir build
$ cd build
$ cmake ..
```

Now you find in the build folder some binaries you can execute.