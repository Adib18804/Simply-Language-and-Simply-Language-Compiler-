/* ═══════════════════════════════════════════════════════════════
   Simply Compiler — Frontend Logic
   ═══════════════════════════════════════════════════════════════ */
(function () {
  'use strict';

  // ── Examples ────────────────────────────────────────────────
  const EXAMPLES = [
    {
      name: 'Hello World',
      code: 'show "Hello, World!"'
    },
    {
      name: 'Variables & Arithmetic',
      code: [
        'make x = 10',
        'make y = 25',
        'make z = x + y',
        'make product = x * y',
        'show z',
        'show product'
      ].join('\n')
    },
    {
      name: 'Conditional (check/otherwise)',
      code: [
        'make age = 21',
        'check age >= 18',
        '    show "You are an adult."',
        'otherwise',
        '    show "You are a minor."',
        'finish'
      ].join('\n')
    },
    {
      name: 'Loop (repeat)',
      code: [
        'make i = 0',
        'repeat 5',
        '    make i = i + 1',
        '    show i',
        'finish'
      ].join('\n')
    },
    {
      name: 'Fibonacci Sequence',
      code: [
        'make a = 0',
        'make b = 1',
        'repeat 8',
        '    make c = a + b',
        '    make a = b',
        '    make b = c',
        '    show a',
        'finish'
      ].join('\n')
    },
    {
      name: 'Boolean & Logic',
      code: [
        'make x = 5',
        'make y = 10',
        'make isLess = x < y',
        'make both = x > 3 && y < 20',
        'show isLess',
        'show both'
      ].join('\n')
    },
    {
      name: 'Nested Conditions',
      code: [
        'make score = 85',
        'check score >= 90',
        '    show "Grade: A"',
        'otherwise',
        '    check score >= 75',
        '        show "Grade: B"',
        '    otherwise',
        '        show "Grade: C"',
        '    finish',
        'finish'
      ].join('\n')
    }
  ];

  // ── State ────────────────────────────────────────────────────
  let editor = null;
  let isBusy = false;

  // ── Helpers ──────────────────────────────────────────────────
  const qs  = id => document.getElementById(id);
  const esc = s  => String(s)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');

  // ── Editor init ──────────────────────────────────────────────
  function initEditor() {
    editor = CodeMirror.fromTextArea(qs('codeEditor'), {
      mode: 'text/x-c',
      theme: 'one-dark',
      lineNumbers: true,
      tabSize: 4,
      indentUnit: 4,
      indentWithTabs: false,
      lineWrapping: false,
      autofocus: true,
      styleActiveLine: true,
      matchBrackets: true,
      extraKeys: {
        'Ctrl-Enter':       () => doRequest(false),
        'Ctrl-Shift-Enter': () => doRequest(true),
        'Ctrl-L':           clearAll
      }
    });
    editor.setValue(EXAMPLES[0].code);
    editor.on('cursorActivity', updateCursorInfo);
    editor.on('change', updateCursorInfo);
    updateCursorInfo();
  }

  function updateCursorInfo() {
    const cursor = editor.getCursor();
    qs('lineColInfo').textContent = `Ln ${cursor.line + 1}, Col ${cursor.ch + 1}`;
  }

  // ── Tab system ───────────────────────────────────────────────
  function initTabs() {
    document.querySelectorAll('.tab').forEach(tab => {
      tab.addEventListener('click', () => activateTab(tab.dataset.tab));
    });
  }

  function activateTab(name) {
    document.querySelectorAll('.tab').forEach(t => {
      const isTarget = t.dataset.tab === name;
      t.classList.toggle('active', isTarget);
      t.setAttribute('aria-selected', String(isTarget));
    });
    document.querySelectorAll('.tab-content').forEach(c => {
      c.classList.toggle('hidden', c.id !== 'tab-' + name);
    });
  }

  // ── Copy buttons ─────────────────────────────────────────────
  function initCopyButtons() {
    document.querySelectorAll('.btn-copy').forEach(btn => {
      btn.addEventListener('click', () => {
        const target = qs(btn.dataset.target);
        if (!target) return;
        const text = target.textContent || '';
        navigator.clipboard.writeText(text).then(() => {
          btn.textContent = '✓ Copied!';
          btn.classList.add('copied');
          setTimeout(() => {
            btn.innerHTML = '⎘ Copy';
            btn.classList.remove('copied');
          }, 1800);
        }).catch(() => {});
      });
    });
  }

  // ── Example picker ───────────────────────────────────────────
  function initExamplePicker() {
    const sel = qs('exampleSelect');
    sel.addEventListener('change', () => {
      const idx = parseInt(sel.value, 10);
      if (idx < 0) return;
      const ex = EXAMPLES[idx];
      if (!ex) return;
      editor.setValue(ex.code);
      editor.focus();
      setStatusMsg(`Loaded: ${ex.name}`);
      sel.value = '-1';
    });
  }

  // ── Status helpers ───────────────────────────────────────────
  function setStatusMsg(msg, type) {
    const el = qs('statusMsg');
    el.textContent = msg;
    el.className = type || '';
  }

  function setChip(text, cls) {
    const chip = qs('statusChip');
    chip.textContent = text;
    chip.className = 'status-chip' + (cls ? ' ' + cls : '');
  }

  function setLoading(busy, message) {
    isBusy = busy;
    const overlay = qs('loadingOverlay');
    const loadingText = qs('loadingText');
    overlay.classList.toggle('hidden', !busy);
    if (message) loadingText.textContent = message;
    qs('btnCompile').disabled = busy;
    qs('btnRun').disabled = busy;
    qs('btnClear').disabled = busy;
  }

  // ── Pipeline step control ────────────────────────────────────
  const PS_IDS = ['ps-lex', 'ps-parse', 'ps-sem', 'ps-tac', 'ps-opt', 'ps-gen', 'ps-run'];

  function resetPipeline() {
    PS_IDS.forEach(id => {
      const el = qs(id);
      el.classList.remove('active', 'done', 'error');
    });
  }

  function setPipelineStep(stepId, state) {
    const el = qs(stepId);
    if (!el) return;
    el.classList.remove('active', 'done', 'error');
    if (state) el.classList.add(state);
  }

  function animatePipeline(result) {
    resetPipeline();
    const steps = PS_IDS;
    let upTo = 0;

    if (result.tokens && result.tokens.length)                    upTo = 1;
    if (result.ast)                                               upTo = 2;
    if (result.symbols && result.symbols.length)                  upTo = 3;
    if (result.tac)                                               upTo = 4;
    if (result.optimizedTac)                                      upTo = 5;
    if (result.generatedC)                                        upTo = 6;
    if (typeof result.output === 'string' && result.output !== '') upTo = 7;

    const hasErrors = (result.errors && result.errors.length > 0) ||
                      (result.semanticErrors && result.semanticErrors.length > 0);

    // If nothing ran but there are errors, at least light the first step red
    if (upTo === 0 && hasErrors) {
      setPipelineStep(steps[0], 'error');
      return;
    }

    let i = 0;
    function tick() {
      if (i > 0) setPipelineStep(steps[i - 1], 'done');
      if (i < upTo) {
        setPipelineStep(steps[i], 'active');
        i++;
        setTimeout(tick, 120);
      } else {
        // Animation finished — mark the last reached step as error if there are errors
        if (hasErrors && upTo > 0) {
          setPipelineStep(steps[upTo - 1], 'error');
        }
      }
    }
    tick();
  }

  // ── Render functions ─────────────────────────────────────────
  function renderTokens(tokens) {
    const tbody = qs('tokenBody');
    const stat  = qs('tokenStat');
    if (!tokens || !tokens.length) {
      tbody.innerHTML = '<tr class="placeholder-row"><td colspan="6">No tokens produced.</td></tr>';
      stat.textContent = '';
      qs('tokenCount').textContent = '0 tokens';
      return;
    }
    stat.textContent = `${tokens.length} token${tokens.length !== 1 ? 's' : ''}`;
    qs('tokenCount').textContent = `${tokens.length} tokens`;
    tbody.innerHTML = tokens.map((t, i) =>
      `<tr>
        <td>${i}</td>
        <td>${esc(t.type)}</td>
        <td>${esc(t.subType || '—')}</td>
        <td>${esc(t.value)}</td>
        <td>${t.line}</td>
        <td>${t.col}</td>
      </tr>`
    ).join('');
  }

  function renderSymbols(symbols) {
    const tbody = qs('symbolBody');
    if (!symbols || !symbols.length) {
      tbody.innerHTML = '<tr class="placeholder-row"><td colspan="4">No symbols declared.</td></tr>';
      return;
    }
    tbody.innerHTML = symbols.map(s =>
      `<tr>
        <td>${esc(s.name)}</td>
        <td>${esc(s.type)}</td>
        <td>${s.scope}</td>
        <td>Line ${s.line}</td>
      </tr>`
    ).join('');
  }

  function renderSemantic(result) {
    const box  = qs('semStatusBox');
    const text = qs('semStatusText');
    const out  = qs('semanticOut');
    const icon = box.querySelector('.sem-icon');
    const hasErrs = result.semanticErrors && result.semanticErrors.length > 0;

    box.className = 'sem-status-box ' + (hasErrs ? 'fail' : 'pass');
    out.style.color = '';
    out.classList.remove('error-text', 'success-text');

    if (hasErrs) {
      if (icon) icon.textContent = '✕';
      text.textContent = `${result.semanticErrors.length} semantic error(s) found.`;
      out.textContent = result.semanticErrors
        .map(e => `[${e.type}] Line ${e.line}: ${e.message}`)
        .join('\n\n');
      out.classList.add('error-text');
    } else {
      if (icon) icon.textContent = '✓';
      text.textContent = 'Semantic analysis passed — no undeclared variables, no type mismatches.';
      out.textContent = 'All checks passed.';
      out.classList.add('success-text');
    }
  }

  function renderErrors(result) {
    const out      = qs('errOut');
    const badge    = qs('errTabBadge');
    const topBadge = qs('errorBadge');
    const countEl  = qs('errorCount');
    const list = [];

    // Non-semantic errors (lexical, parse, gcc, runtime)
    if (result.errors && result.errors.length) {
      result.errors.forEach(e => {
        // Skip semantic errors here — they are already in result.semanticErrors
        if (e.type && e.type.startsWith('Semantic:')) return;
        const loc = e.line ? `Line ${e.line}${e.col ? `, Col ${e.col}` : ''}` : '';
        list.push(`[${e.type || 'Error'}]${loc ? ' ' + loc + ':' : ''} ${e.message}`);
      });
    }
    // Semantic errors (from dedicated field)
    if (result.semanticErrors && result.semanticErrors.length) {
      result.semanticErrors.forEach(e => {
        list.push(`[${e.type}] Line ${e.line}: ${e.message}`);
      });
    }

    const count = list.length;
    if (count > 0) {
      out.textContent = list.join('\n\n');
      out.style.color = '';
      out.classList.remove('success-text');
      out.classList.add('error-text');
      badge.textContent = count;
      badge.classList.remove('hidden');
      topBadge.classList.remove('hidden');
      countEl.textContent = count;
    } else {
      out.textContent = 'No errors reported.';
      out.style.color = '';
      out.classList.remove('error-text');
      out.classList.add('success-text');
      badge.classList.add('hidden');
      topBadge.classList.add('hidden');
    }
  }

  function renderAll(result) {
    renderTokens(result.tokens || []);

    qs('astOut').textContent = result.ast
      ? result.ast
      : '(AST not available — check for parse errors)';

    renderSymbols(result.symbols || []);
    renderSemantic(result);

    qs('tacOut').textContent     = result.tac          || '(TAC not generated)';
    qs('optOut').textContent     = result.optimizedTac || '(Optimized TAC not generated)';
    qs('genOut').textContent     = result.generatedC   || '(No C code generated)';

    const runEl = qs('runOut');
    if (typeof result.output === 'string' && result.output.length > 0) {
      runEl.innerHTML = esc(result.output);
      runEl.style.color = 'var(--green)';
    } else if (result.output === '') {
      runEl.innerHTML = '<em style="color:var(--text-muted)">(program produced no output)</em>';
    } else {
      runEl.innerHTML = 'Click <strong>Run</strong> to compile and execute your Simply program...';
      runEl.style.color = '';
    }

    renderErrors(result);
  }

  // ── Compile / Run ─────────────────────────────────────────────
  async function doRequest(run) {
    if (isBusy) return;
    const code = editor.getValue().trim();
    if (!code) {
      setStatusMsg('Editor is empty — write some Simply code first.', 'error');
      setChip('Empty', 'error');
      return;
    }

    const label = run ? 'Compiling & Running...' : 'Compiling...';
    setLoading(true, label);
    setChip(label.replace('...', ''), 'active');
    setStatusMsg(label);
    resetPipeline();

    try {
      const resp = await fetch('/api/compile', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code, run: !!run })
      });

      if (!resp.ok) {
        throw new Error(`Server responded with ${resp.status}`);
      }

      const result = await resp.json();
      renderAll(result);
      animatePipeline(result);

      const hasErrors = (result.errors && result.errors.length > 0) ||
                        (result.semanticErrors && result.semanticErrors.length > 0);

      if (hasErrors) {
        const errCount = (result.errors?.length || 0) + (result.semanticErrors?.length || 0);
        setChip(`${errCount} error(s)`, 'error');
        setStatusMsg(`Compilation finished with ${errCount} error(s).`, 'error');
        activateTab('errors');
      } else if (run && result.success) {
        setChip('Executed', 'success');
        setStatusMsg('Program compiled and executed successfully.');
        activateTab('output');
      } else if (result.success) {
        setChip('Compiled', 'success');
        setStatusMsg('Compilation successful — all phases complete.');
        activateTab('tokens');
      } else {
        setChip('Failed', 'error');
        setStatusMsg('Compilation failed — see Errors tab.', 'error');
        activateTab('errors');
      }
    } catch (err) {
      console.error(err);
      qs('errOut').textContent = 'Network / server error: ' + err.message;
      qs('errOut').style.color = 'var(--red)';
      setChip('Server Error', 'error');
      setStatusMsg('Could not reach the compiler server.', 'error');
      activateTab('errors');
    } finally {
      setLoading(false);
    }
  }

  // ── Clear ─────────────────────────────────────────────────────
  function clearAll() {
    editor.setValue('');
    editor.focus();

    qs('tokenBody').innerHTML = '<tr class="placeholder-row"><td colspan="6">Press <strong>Compile</strong> to see tokens generated by the lexer.</td></tr>';
    qs('tokenStat').textContent = '';
    qs('tokenCount').textContent = '0 tokens';
    qs('astOut').textContent = 'Press Compile to see the Abstract Syntax Tree...';
    qs('symbolBody').innerHTML = '<tr class="placeholder-row"><td colspan="4">Symbol table will appear here after compilation.</td></tr>';
    qs('semanticOut').textContent = 'No semantic issues.';
    qs('semanticOut').style.color = '';
    qs('semStatusText').textContent = 'Run Compile to perform semantic analysis.';
    qs('semStatusBox').className = 'sem-status-box';
    const semIcon = document.querySelector('#semStatusBox .sem-icon');
    if (semIcon) semIcon.textContent = '○';
    qs('tacOut').textContent = 'Press Compile to generate the Three-Address Code (TAC)...';
    qs('optOut').textContent = 'Press Compile to see the Optimized TAC...';
    qs('genOut').textContent = 'Press Compile to generate C source code...';
    qs('runOut').innerHTML   = 'Click <strong>Run</strong> to compile and execute your Simply program...';
    qs('runOut').style.color = '';
    qs('errOut').textContent = 'No errors reported.';
    qs('errOut').style.color = '';
    qs('errOut').classList.remove('error-text');
    qs('errOut').classList.add('success-text');
    qs('errTabBadge').classList.add('hidden');
    qs('errorBadge').classList.add('hidden');

    setChip('Ready');
    setStatusMsg('Editor cleared — ready for new code.');
    resetPipeline();
    activateTab('tokens');
  }

  // ── Boot ─────────────────────────────────────────────────────
  document.addEventListener('DOMContentLoaded', () => {
    initEditor();
    initTabs();
    initCopyButtons();
    initExamplePicker();

    qs('btnCompile').addEventListener('click', () => doRequest(false));
    qs('btnRun').addEventListener('click',     () => doRequest(true));
    qs('btnClear').addEventListener('click',   clearAll);
  });

})();
