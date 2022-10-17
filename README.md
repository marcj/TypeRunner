# TypeRunner

<p align="center">
<img src="./docs/logo.png"/>
</p>

<p align="center">
High-performance TypeScript compiler.
</p>

## Goal

- Parser
- Type checking (as CLI and as library)
- Language Server
- Interactive type debugging
- Type information in other languages
- (optional) transpiling to JavaScript
- (optional) RTTI in JavaScript
- (optional) type profiler

The goal is to make TypeScript type checking as fast as possible and provide alongside with it a native library for other languages, so they can use TypeScript type information
without the need for a JavaScript engine for all sorts of use cases like JSON-Schema replacement, ORM DSL, encoding information (like Protocol Buffers schema) and more.

The goal is not to be a drop-in replacement for the entire official TypeScript compiler (tsc). TypeScript supports so much that is not always necessary. 
We focus on the more strict TS part which means TypeRunner won't support JSDoc and a lot of compiler options.  

## Status

The source code in the initial version is really only a proof of concept. It consists of roughly 30k LoC and shows very promising results.
The approach is a TS-to-bytecode compiler and then run the bytecode in a custom virtual machine.
The data show that this approach can lead to a hundred- to several-thousand-fold improvement in speed.

![TypeRunner Debugger](./docs/typerunner-debugger.png)

Once the project gets funding through the community, the development will continue.

## Performance

`TypeRunner cold` means the file was seen for the first time and has to be compiled to bytecode first.
`TypeRunner warm` means the bytecode could be directly executed because it was cached. Generally it is assumed
that only a few files change, for example if you have a project with 100 projects and edit one and then rerun 
type checking, then only the changed one has the slower `cold` timing. Note that compilation has not yet been optimised 
(it still uses a slow memory allocator which can be improvement by a tenfold). 

Note that `tsc` numbers are after 10 iterations (the JavaScript engine V8 JIT optimises it early already), which somewhat
leads to a wrong conclusion. Only 1 iteration is 10x slower and a cold `tsc` start even slower because of the initial bootstrap
delay of several hundred milliseconds. So you can generally assume that `tsc` is slower than the numbers shown below.

### Basic variables

```typescript
const v1: string = "abc";
const v2: number = 123;
```

```
TypeScript tsc:  1.1ms
TypeRunner cold: 0.008374167ms (131x faster)
TypeRunner warm: 0.000104375ms (10,576x faster)
```

### Generic function

```typescript
function doIt<T extends number>(v: T) {
}
const a = doIt<number>;
a(23);
```

```
TypeScript tsc:  1.4ms
TypeRunner cold: 0.014966250ms (93x faster)
TypeRunner warm: 0.000181875ms (7,697x faster)
```

### Object literal type

```typescript
type Person = {name: string, age: number}

const a: Person = {name: 'Peter', age: 52};
const b: Person = {name: 'Peter', age: '52'};
```

```
TypeScript tsc:  1.9ms
TypeRunner cold: 0.021316125ms (89x faster)
TypeRunner warm: 0.001111333ms (1,709x faster)
```

### Complex type

```typescript
type StringToNum<T extends string, A extends 0[] = []> = `${A['length']}` extends T ? A['length'] : StringToNum<T, [...A, 0]>;
const var1: StringToNum<'999'> = 999;
```

```
TypeScript tsc:  350.2ms
TypeRunner cold:   0.862534792ms (406x faster)
TypeRunner warm:   0.839308334ms (417x faster)
```

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

Now you find in the build folder some binaries you can execute.**