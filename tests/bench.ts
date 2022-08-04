import {CompilerOptions, createCompilerHost, createProgram, getPreEmitDiagnostics} from "typescript";

const code = `
    class MyDate {
        static now(): number {
            return 0;
        }
    }
    const now: number = MyDate.now();
    const now2: string = MyDate.now();
`;

const options: CompilerOptions = {
    strict: true,
    skipLibCheck: true,
    types: [],
    typeRoots: [],
    lib: [],
};
const host = createCompilerHost(options);
host.readFile = (fileName) => {
    return code;
}
host.writeFile = () => {
}

const program = createProgram(['app.ts'], options, host);
console.log(getPreEmitDiagnostics(program));

const iterations = 10;
const start = Date.now();
for (let i = 0; i < iterations; i++) {
    const program = createProgram(['app.ts'], options, host);
    const diagnostics = getPreEmitDiagnostics(program);
}
const took = Date.now() - start;
console.log(iterations, 'iterations took', took, 'ms.', took/iterations, 'ms/op');
