import {CompilerOptions, createCompilerHost, createProgram, getPreEmitDiagnostics} from "typescript";
import {readFileSync} from "fs";
import {execSync} from "child_process";

// const code = `
//     function doIt(): string {
//         return 1;
//     }
//     doIt();
// `;
const file = process.argv[2];
const code = readFileSync(file).toString('utf8');

console.log('file', file);
console.log('code:');
console.log(code);
console.log();

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

const iterations = 10;
const start = Date.now();
for (let i = 0; i < iterations; i++) {
    const program = createProgram(['app.ts'], options, host);
    const diagnostics = getPreEmitDiagnostics(program);
}
const took = Date.now() - start;
console.log('tsc: ', iterations, 'iterations took', took, 'ms.', took / iterations, 'ms/op');

execSync(__dirname + '/../cmake-build-release/bench ' + file, {stdio: 'inherit'});
