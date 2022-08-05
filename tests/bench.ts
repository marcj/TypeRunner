import {CompilerOptions, createCompilerHost, createProgram, getPreEmitDiagnostics} from "typescript";

const code = `
    function doIt(): string {
        return 1;
    }
    doIt();
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
