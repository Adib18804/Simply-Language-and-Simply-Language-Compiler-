const { execFileSync, spawnSync } = require('child_process');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { v4: uuidv4 } = require('uuid');

const COMPILER_PATH = path.join(__dirname, '..', '..', 'compiler', 'simply-compiler.exe');
const TEMP_ROOT = path.join(__dirname, '..', 'temp');
const EXECUTION_TIMEOUT = 5000;

if (!fs.existsSync(TEMP_ROOT)) {
    fs.mkdirSync(TEMP_ROOT, { recursive: true });
}

function readFileSafe(filePath) {
    try { return fs.readFileSync(filePath, 'utf8'); }
    catch { return ''; }
}

function parseTokens(text) {
    const tokens = [];
    const lines = text.trim().split('\n').filter(l => l.length > 0);
    for (const line of lines) {
        const parts = line.split('\t');
        if (parts.length >= 5) {
            tokens.push({
                type: parts[0],
                subType: parts[1] === '-' ? '' : parts[1],
                value: parts[2],
                line: parseInt(parts[3], 10),
                col: parseInt(parts[4], 10)
            });
        }
    }
    return tokens;
}

function parseTAC(text) {
    const instrs = [];
    const lines = text.trim().split('\n').filter(l => l.length > 0);
    for (const line of lines) {
        const parts = line.split('\t');
        if (parts.length >= 6) {
            instrs.push({
                index: parseInt(parts[0], 10),
                op: parts[1],
                result: parts[2],
                arg1: parts[3],
                arg2: parts[4],
                line: parseInt(parts[5], 10)
            });
        }
    }
    return instrs;
}

function formatTacForDisplay(tacArr) {
    return tacArr.map(t => {
        switch (t.op) {
            case 'LABEL': return `${t.result}:`;
            case 'JUMP': return `   goto ${t.result}`;
            case 'JFALSE': return `   if (!${t.arg1}) goto ${t.result}`;
            case 'ASSIGN': return `   ${t.result} = ${t.arg1}`;
            case 'OUTPUT': return `   show ${t.arg1}`;
            case 'INPUT': return `   ${t.result} = ask ${t.arg1}`;
            case 'NOT': return `   ${t.result} = !${t.arg1}`;
            case 'NEG': return `   ${t.result} = -${t.arg1}`;
            case 'CALL': return `   call ${t.arg1}`;
            case 'RETURN': return `   return ${t.arg1}`;
            default: return `   ${t.result} = ${t.arg1} ${opToSymbol(t.op)} ${t.arg2}`;
        }
    }).join('\n');
}

function opToSymbol(op) {
    const m = { 'ADD': '+', 'SUB': '-', 'MUL': '*', 'DIV': '/', 'MOD': '%',
        'EQ': '==', 'NEQ': '!=', 'LT': '<', 'GT': '>', 'LTE': '<=', 'GTE': '>=',
        'AND': '&&', 'OR': '||' };
    return m[op] || op;
}

function parseErrors(text) {
    const errs = [];
    const lines = text.split('\n');
    for (const line of lines) {
        if (line.startsWith('ERR\t')) {
            const parts = line.split('\t');
            if (parts.length >= 4) {
                errs.push({
                    type: 'Parse Error',
                    line: parseInt(parts[1], 10),
                    col: parseInt(parts[2], 10),
                    message: parts.slice(3).join('\t')
                });
            }
        }
    }
    return errs;
}

function parseSemantic(text) {
    const result = { errors: [], symbols: [] };
    const lines = text.split('\n');
    let symMode = false;
    for (const line of lines) {
        if (line.startsWith('ERR\t')) {
            const parts = line.split('\t');
            if (parts.length >= 4) {
                result.errors.push({
                    type: parts[1],
                    line: parseInt(parts[2], 10),
                    message: parts.slice(3).join('\t')
                });
            }
        }
        if (line.startsWith('---SYMBOLS---')) { symMode = true; continue; }
        if (symMode && line.startsWith('SYM\t')) {
            const parts = line.split('\t');
            if (parts.length >= 5) {
                result.symbols.push({
                    name: parts[1],
                    type: parts[2],
                    scope: parseInt(parts[3], 10),
                    line: parseInt(parts[4], 10)
                });
            }
        }
    }
    return result;
}

async function compileAndRun(code, runProgram = true) {
    const sessionId = uuidv4();
    const sessionDir = path.join(TEMP_ROOT, sessionId);
    fs.mkdirSync(sessionDir, { recursive: true });

    const result = {
        success: false,
        tokens: [],
        ast: '',
        semanticErrors: [],
        symbols: [],
        tac: '',
        tacArray: [],
        optimizedTac: '',
        optimizedTacArray: [],
        generatedC: '',
        output: '',
        errors: [],
        compileExitCode: null
    };

    try {
        const simplyFile = path.join(sessionDir, 'input.simply');
        fs.writeFileSync(simplyFile, code, 'utf8');

        let compilerOutput = '';
        try {
            const proc = spawnSync(COMPILER_PATH, [simplyFile, sessionDir], {
                timeout: EXECUTION_TIMEOUT,
                encoding: 'utf8'
            });
            result.compileExitCode = proc.status;
            compilerOutput = proc.stdout || '';
            if (proc.error) {
                result.errors.push({ type: 'Compiler Invocation Error', message: proc.error.message });
                return result;
            }
        } catch (e) {
            result.errors.push({ type: 'Compiler Execution Failed', message: e.message });
            return result;
        }

        const statusText = readFileSafe(path.join(sessionDir, 'status.txt')).trim();

        const tokensText = readFileSafe(path.join(sessionDir, 'tokens.txt'));
        result.tokens = parseTokens(tokensText);

        result.ast = readFileSafe(path.join(sessionDir, 'ast.txt'));

        const parseErrText = readFileSafe(path.join(sessionDir, 'parse_errors.txt'));
        const parseErrorList = parseErrors(parseErrText);
        result.errors.push(...parseErrorList);

        const semText = readFileSafe(path.join(sessionDir, 'semantic.txt'));
        const sem = parseSemantic(semText);
        result.semanticErrors = sem.errors.map(e => ({
            type: `Semantic: ${e.type}`,
            line: e.line,
            message: e.message
        }));
        result.symbols = sem.symbols;
        result.errors.push(...result.semanticErrors);

        const tacRaw = readFileSafe(path.join(sessionDir, 'tac.txt'));
        result.tacArray = parseTAC(tacRaw);
        result.tac = formatTacForDisplay(result.tacArray);

        const optRaw = readFileSafe(path.join(sessionDir, 'optimized_tac.txt'));
        result.optimizedTacArray = parseTAC(optRaw);
        result.optimizedTac = formatTacForDisplay(result.optimizedTacArray);

        result.generatedC = readFileSafe(path.join(sessionDir, 'output.c'));

        if (statusText === 'LEX_ERROR') {
            for (const tok of result.tokens) {
                if (tok.type === 'ERROR') {
                    result.errors.push({ type: 'Lexical Error', line: tok.line, col: tok.col, message: `Invalid character/token: '${tok.value}'` });
                }
            }
        }

        if (statusText !== 'SUCCESS') {
            return result;
        }

        if (!result.generatedC) {
            result.errors.push({ type: 'Code Generation Error', message: 'No C code was generated.' });
            return result;
        }

        const cFile = path.join(sessionDir, 'output.c');
        const exeFile = path.join(sessionDir, 'output.exe');

        try {
            const gccProc = spawnSync('gcc', ['-O2', '-o', exeFile, cFile], {
                timeout: 10000,
                encoding: 'utf8'
            });
            if (gccProc.status !== 0) {
                result.errors.push({
                    type: 'GCC Compilation Error',
                    message: (gccProc.stderr || gccProc.stdout || 'GCC failed').trim()
                });
                return result;
            }
        } catch (e) {
            result.errors.push({ type: 'GCC Invocation Error', message: e.message });
            return result;
        }

        if (runProgram) {
            try {
                const runProc = spawnSync(exeFile, [], {
                    timeout: EXECUTION_TIMEOUT,
                    encoding: 'utf8',
                    input: ''
                });
                result.output = (runProc.stdout || '');
                if (runProc.stderr) {
                    result.errors.push({ type: 'Runtime Output', message: runProc.stderr });
                }
                if (runProc.error) {
                    result.errors.push({ type: 'Runtime Error', message: runProc.error.message });
                } else if (runProc.status !== null && runProc.status !== 0) {
                    result.errors.push({ type: 'Runtime', message: `Program exited with code ${runProc.status}` });
                }
            } catch (e) {
                result.errors.push({ type: 'Runtime Execution Failed', message: e.message });
            }
        }

        result.success = result.errors.filter(e => e.type && (e.type.includes('Error') || e.type.includes('Failed'))).length === 0
                      && result.semanticErrors.length === 0;
        return result;
    } finally {
        setTimeout(() => {
            try { fs.rmSync(sessionDir, { recursive: true, force: true }); } catch {}
        }, 60000);
    }
}

module.exports = { compileAndRun };
