import type { Config } from '@netlify/functions'
import { spawnSync } from 'node:child_process'
import { existsSync, mkdtempSync, readFileSync, rmSync, writeFileSync } from 'node:fs'
import { tmpdir } from 'node:os'
import { join } from 'node:path'
import { fileURLToPath } from 'node:url'

const EXECUTION_TIMEOUT = 5000

// The bundler's layout for `included_files` isn't guaranteed to mirror the
// repo's directory structure, so try every plausible location for the
// pre-built compiler binary and use whichever one actually exists.
function findCompilerPath(): string | null {
  const candidates = [
    fileURLToPath(new URL('../../compiler/simply-compiler.exe', import.meta.url)),
    join(process.cwd(), 'compiler', 'simply-compiler.exe'),
    fileURLToPath(new URL('./compiler/simply-compiler.exe', import.meta.url)),
    '/var/task/compiler/simply-compiler.exe',
  ]
  return candidates.find((candidate) => existsSync(candidate)) ?? null
}

function readFileSafe(filePath: string): string {
  try {
    return readFileSync(filePath, 'utf8')
  } catch {
    return ''
  }
}

type Token = { type: string; subType: string; value: string; line: number; col: number }
type TacInstr = { index: number; op: string; result: string; arg1: string; arg2: string; line: number }
type CompileError = { type?: string; line?: number; col?: number; message: string }

function parseTokens(text: string): Token[] {
  const tokens: Token[] = []
  const lines = text.trim().split('\n').filter((l) => l.length > 0)
  for (const line of lines) {
    const parts = line.split('\t')
    if (parts.length >= 5) {
      tokens.push({
        type: parts[0],
        subType: parts[1] === '-' ? '' : parts[1],
        value: parts[2],
        line: parseInt(parts[3], 10),
        col: parseInt(parts[4], 10),
      })
    }
  }
  return tokens
}

function parseTAC(text: string): TacInstr[] {
  const instrs: TacInstr[] = []
  const lines = text.trim().split('\n').filter((l) => l.length > 0)
  for (const line of lines) {
    const parts = line.split('\t')
    if (parts.length >= 6) {
      instrs.push({
        index: parseInt(parts[0], 10),
        op: parts[1],
        result: parts[2],
        arg1: parts[3],
        arg2: parts[4],
        line: parseInt(parts[5], 10),
      })
    }
  }
  return instrs
}

function opToSymbol(op: string): string {
  const m: Record<string, string> = {
    ADD: '+', SUB: '-', MUL: '*', DIV: '/', MOD: '%',
    EQ: '==', NEQ: '!=', LT: '<', GT: '>', LTE: '<=', GTE: '>=',
    AND: '&&', OR: '||',
  }
  return m[op] || op
}

function formatTacForDisplay(tacArr: TacInstr[]): string {
  return tacArr
    .map((t) => {
      switch (t.op) {
        case 'LABEL': return `${t.result}:`
        case 'JUMP': return `   goto ${t.result}`
        case 'JFALSE': return `   if (!${t.arg1}) goto ${t.result}`
        case 'ASSIGN': return `   ${t.result} = ${t.arg1}`
        case 'OUTPUT': return `   show ${t.arg1}`
        case 'INPUT': return `   ${t.result} = ask ${t.arg1}`
        case 'NOT': return `   ${t.result} = !${t.arg1}`
        case 'NEG': return `   ${t.result} = -${t.arg1}`
        case 'CALL': return `   call ${t.arg1}`
        case 'RETURN': return `   return ${t.arg1}`
        default: return `   ${t.result} = ${t.arg1} ${opToSymbol(t.op)} ${t.arg2}`
      }
    })
    .join('\n')
}

function parseErrors(text: string): CompileError[] {
  const errs: CompileError[] = []
  for (const line of text.split('\n')) {
    if (line.startsWith('ERR\t')) {
      const parts = line.split('\t')
      if (parts.length >= 4) {
        errs.push({ type: 'Parse Error', line: parseInt(parts[1], 10), col: parseInt(parts[2], 10), message: parts.slice(3).join('\t') })
      }
    }
  }
  return errs
}

function parseSemantic(text: string): { errors: CompileError[]; symbols: any[] } {
  const result: { errors: CompileError[]; symbols: any[] } = { errors: [], symbols: [] }
  let symMode = false
  for (const line of text.split('\n')) {
    if (line.startsWith('ERR\t')) {
      const parts = line.split('\t')
      if (parts.length >= 4) {
        result.errors.push({ type: parts[1], line: parseInt(parts[2], 10), message: parts.slice(3).join('\t') })
      }
    }
    if (line.startsWith('---SYMBOLS---')) { symMode = true; continue }
    if (symMode && line.startsWith('SYM\t')) {
      const parts = line.split('\t')
      if (parts.length >= 5) {
        result.symbols.push({ name: parts[1], type: parts[2], scope: parseInt(parts[3], 10), line: parseInt(parts[4], 10) })
      }
    }
  }
  return result
}

async function compileAndRun(code: string, runProgram: boolean) {
  const result: Record<string, any> = {
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
    errors: [] as CompileError[],
    compileExitCode: null,
  }

  const compilerPath = findCompilerPath()
  if (!compilerPath) {
    result.errors.push({ type: 'Server Error', message: 'Compiler binary is not available in this deployment.' })
    return result
  }

  const sessionDir = mkdtempSync(join(tmpdir(), 'simply-'))

  try {
    const simplyFile = join(sessionDir, 'input.simply')
    writeFileSync(simplyFile, code, 'utf8')

    const proc = spawnSync(compilerPath, [simplyFile, sessionDir], { timeout: EXECUTION_TIMEOUT, encoding: 'utf8' })
    result.compileExitCode = proc.status
    if (proc.error) {
      result.errors.push({ type: 'Compiler Invocation Error', message: proc.error.message })
      return result
    }

    const statusText = readFileSafe(join(sessionDir, 'status.txt')).trim()

    result.tokens = parseTokens(readFileSafe(join(sessionDir, 'tokens.txt')))
    result.ast = readFileSafe(join(sessionDir, 'ast.txt'))

    result.errors.push(...parseErrors(readFileSafe(join(sessionDir, 'parse_errors.txt'))))

    const sem = parseSemantic(readFileSafe(join(sessionDir, 'semantic.txt')))
    result.semanticErrors = sem.errors.map((e) => ({ type: `Semantic: ${e.type}`, line: e.line, message: e.message }))
    result.symbols = sem.symbols
    result.errors.push(...result.semanticErrors)

    result.tacArray = parseTAC(readFileSafe(join(sessionDir, 'tac.txt')))
    result.tac = formatTacForDisplay(result.tacArray)

    result.optimizedTacArray = parseTAC(readFileSafe(join(sessionDir, 'optimized_tac.txt')))
    result.optimizedTac = formatTacForDisplay(result.optimizedTacArray)

    result.generatedC = readFileSafe(join(sessionDir, 'output.c'))

    if (statusText === 'LEX_ERROR') {
      for (const tok of result.tokens) {
        if (tok.type === 'ERROR') {
          result.errors.push({ type: 'Lexical Error', line: tok.line, col: tok.col, message: `Invalid character/token: '${tok.value}'` })
        }
      }
    }

    if (statusText !== 'SUCCESS') return result
    if (!result.generatedC) {
      result.errors.push({ type: 'Code Generation Error', message: 'No C code was generated.' })
      return result
    }

    const cFile = join(sessionDir, 'output.c')
    const exeFile = join(sessionDir, 'output.exe')

    const gccProc = spawnSync('gcc', ['-O2', '-o', exeFile, cFile], { timeout: 10000, encoding: 'utf8' })
    if (gccProc.error) {
      // Native execution isn't guaranteed on every deployment target; the
      // pipeline output up to code generation is still fully valid.
      result.errors.push({ type: 'Execution Unavailable', message: 'Program execution is not available in this deployment environment. Compiler pipeline output above is complete.' })
      result.success = result.semanticErrors.length === 0
      return result
    }
    if (gccProc.status !== 0) {
      result.errors.push({ type: 'GCC Compilation Error', message: (gccProc.stderr || gccProc.stdout || 'GCC failed').trim() })
      return result
    }

    if (runProgram) {
      const runProc = spawnSync(exeFile, [], { timeout: EXECUTION_TIMEOUT, encoding: 'utf8', input: '' })
      result.output = runProc.stdout || ''
      if (runProc.stderr) result.errors.push({ type: 'Runtime Output', message: runProc.stderr })
      if (runProc.error) {
        result.errors.push({ type: 'Runtime Error', message: runProc.error.message })
      } else if (runProc.status !== null && runProc.status !== 0) {
        result.errors.push({ type: 'Runtime', message: `Program exited with code ${runProc.status}` })
      }
    }

    result.success = result.errors.filter((e) => e.type && (e.type.includes('Error') || e.type.includes('Failed'))).length === 0
      && result.semanticErrors.length === 0
    return result
  } finally {
    try { rmSync(sessionDir, { recursive: true, force: true }) } catch {}
  }
}

export default async (req: Request) => {
  if (req.method !== 'POST') {
    return Response.json({ success: false, errors: [{ message: 'Method not allowed.' }] }, { status: 405 })
  }

  let body: any
  try {
    body = await req.json()
  } catch {
    return Response.json({ success: false, errors: [{ message: 'Invalid JSON body.' }] }, { status: 400 })
  }

  const { code, run } = body ?? {}
  if (typeof code !== 'string') {
    return Response.json({ success: false, errors: [{ message: 'Missing "code" in request body.' }] }, { status: 400 })
  }

  try {
    const result = await compileAndRun(code, run === true)
    return Response.json(result)
  } catch (err: any) {
    return Response.json({ success: false, errors: [{ type: 'Server Error', message: err?.message || String(err) }] }, { status: 500 })
  }
}

export const config: Config = {
  path: '/api/compile',
  method: 'POST',
}
