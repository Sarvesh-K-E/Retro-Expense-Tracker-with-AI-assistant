(function () {
  var inputQueue = [];
  var terminalEl = null;
  var csvIndicatorEl = null;
  var viewCsvBtn = null;
  var downloadCsvBtn = null;
  var csvModalEl = null;
  var csvContentEl = null;
  var closeCsvBtn = null;
  var uiBound = false;

  var lineInputMode = false;
  var waitingInput = false;
  var lineDraft = '';
  var runtimeExited = false;
  var inputResolver = null;

  const COLOR_RED = '\x1b[31m';
  const COLOR_CYAN = '\x1b[36m';
  const COLOR_YELLOW = '\x1b[33m';
  const COLOR_GREEN = '\x1b[32m';
  const COLOR_RESET = '\x1b[0m';

  var trackedFiles = [
    { name: 'expenses.dat', path: '/expenses.dat' },
    { name: 'budgets.dat', path: '/budgets.dat' },
    { name: 'expenses_export.csv', path: '/expenses_export.csv' }
  ];
  var fileSignatures = {};
  var syncBusy = false;
  var syncTimer = null;
  var initialLoadDone = false;
  var cloudConnected = false;
  var syncFailCount = 0;
  
  var vaultResolver;
  var vaultPromise = new Promise(r => vaultResolver = r);
  window.onVaultUnlocked = function() {
    if (vaultResolver) vaultResolver();
  };

  var renderPending = false;
  var screen = [[]];
  var cursorRow = 0;
  var cursorCol = 0;
  var escState = 0;
  var csiBuffer = '';

  var styleState = { fg: null, bg: null, bold: false };

  var FG = {
    30: '#111827', 31: '#ef4444', 32: '#22c55e', 33: '#f59e0b',
    34: '#3b82f6', 35: '#ec4899', 36: '#06b6d4', 37: '#e5e7eb',
    90: '#6b7280', 91: '#f87171', 92: '#4ade80', 93: '#facc15',
    94: '#60a5fa', 95: '#f472b6', 96: '#22d3ee', 97: '#f9fafb'
  };

  var BG = {
    40: '#000000', 41: '#7f1d1d', 42: '#14532d', 43: '#854d0e',
    44: '#1e3a8a', 45: '#831843', 46: '#155e75', 47: '#d1d5db',
    100: '#374151', 101: '#b91c1c', 102: '#166534', 103: '#a16207',
    104: '#1d4ed8', 105: '#be185d', 106: '#0e7490', 107: '#f3f4f6'
  };

  function resolveUI() {
    if (!terminalEl) terminalEl = document.getElementById('terminal');
    if (!csvIndicatorEl) csvIndicatorEl = document.getElementById('csv-indicator');
    if (!viewCsvBtn) viewCsvBtn = document.getElementById('view-csv');
    if (!downloadCsvBtn) downloadCsvBtn = document.getElementById('download-csv');
    if (!csvModalEl) csvModalEl = document.getElementById('csv-modal');
    if (!csvContentEl) csvContentEl = document.getElementById('csv-content');
    if (!closeCsvBtn) closeCsvBtn = document.getElementById('close-csv');
  }

  function escapeHtml(s) {
    return s
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  function styleKey() {
    return (styleState.fg || '') + '|' + (styleState.bg || '') + '|' + (styleState.bold ? '1' : '0');
  }

  function styleCssFromKey(key) {
    if (!key) return '';
    var parts = key.split('|');
    var fg = parts[0];
    var bg = parts[1];
    var bold = parts[2] === '1';
    var css = '';
    if (fg) css += 'color:' + fg + ';';
    if (bg) css += 'background-color:' + bg + ';';
    if (bold) css += 'font-weight:700;';
    return css;
  }

  function ensureRow(row) {
    while (screen.length <= row) screen.push([]);
  }

  function clearScreen() {
    screen = [[]];
    cursorRow = 0;
    cursorCol = 0;
  }

  function putChar(ch) {
    if (ch === '\r') {
      ensureRow(cursorRow);
      screen[cursorRow].length = 0;
      cursorCol = 0;
      return;
    }

    if (ch === '\n') {
      cursorRow += 1;
      cursorCol = 0;
      ensureRow(cursorRow);
      return;
    }

    var out = ch === '\t' ? '    ' : ch;
    for (var i = 0; i < out.length; i++) {
      ensureRow(cursorRow);
      var line = screen[cursorRow];
      while (line.length < cursorCol) {
        line.push({ ch: ' ', style: '' });
      }
      line[cursorCol] = { ch: out[i], style: styleKey() };
      cursorCol += 1;
    }
  }

  function applySgr(params) {
    if (params.length === 0) params = [0];

    for (var i = 0; i < params.length; i++) {
      var p = params[i];
      if (p === 0) {
        styleState.fg = null;
        styleState.bg = null;
        styleState.bold = false;
      } else if (p === 1) {
        styleState.bold = true;
      } else if (p === 22) {
        styleState.bold = false;
      } else if (p === 39) {
        styleState.fg = null;
      } else if (p === 49) {
        styleState.bg = null;
      } else if (FG[p]) {
        styleState.fg = FG[p];
      } else if (BG[p]) {
        styleState.bg = BG[p];
      }
    }
  }

  function parseParams(raw) {
    if (!raw) return [];
    var parts = raw.split(';');
    var out = [];
    for (var i = 0; i < parts.length; i++) {
      var v = parseInt(parts[i], 10);
      out.push(isNaN(v) ? 0 : v);
    }
    return out;
  }

  function processCsi(sequence) {
    var finalByte = sequence[sequence.length - 1];
    var rawParams = sequence.slice(0, -1);
    var params = parseParams(rawParams);

    if (finalByte === 'm') {
      applySgr(params);
      return;
    }

    if (finalByte === 'H' || finalByte === 'f') {
      cursorRow = Math.max(0, (params[0] || 1) - 1);
      cursorCol = Math.max(0, (params[1] || 1) - 1);
      ensureRow(cursorRow);
      return;
    }

    if (finalByte === 'J') {
      var mode = params.length ? params[0] : 0;
      if (mode === 0 || mode === 2 || mode === 3) {
        clearScreen();
      }
      return;
    }

    if (finalByte === 'K') {
      ensureRow(cursorRow);
      screen[cursorRow].length = cursorCol;
      return;
    }

    if (finalByte === 'A') {
      cursorRow = Math.max(0, cursorRow - (params[0] || 1));
      ensureRow(cursorRow);
      return;
    }

    if (finalByte === 'B') {
      cursorRow += (params[0] || 1);
      ensureRow(cursorRow);
      return;
    }

    if (finalByte === 'C') {
      cursorCol += (params[0] || 1);
      return;
    }

    if (finalByte === 'D') {
      cursorCol = Math.max(0, cursorCol - (params[0] || 1));
      return;
    }
  }

  function writeChar(ch) {
    if (escState === 0) {
      if (ch === '\x1b') {
        escState = 1;
        return;
      }
      putChar(ch);
      scheduleRender();
      return;
    }

    if (escState === 1) {
      if (ch === '[') {
        escState = 2;
        csiBuffer = '';
      } else {
        escState = 0;
      }
      return;
    }

    if (escState === 2) {
      csiBuffer += ch;
      var code = ch.charCodeAt(0);
      if (code >= 0x40 && code <= 0x7e) {
        processCsi(csiBuffer);
        escState = 0;
        csiBuffer = '';
        scheduleRender();
      }
    }
  }

  function writeCode(code) {
    if (code === null || code === undefined) return;
    if (typeof code !== 'number') code = Number(code);
    if (!Number.isFinite(code)) return;
    code = code & 0xFF;

    if (code === 219) {
      writeChar('\u2588');
      return;
    }

    if (code >= 0 && code <= 255) {
      writeChar(String.fromCharCode(code));
    }
  }

  function writePlainLine(text) {
    for (var i = 0; i < text.length; i++) writeChar(text[i]);
    writeChar('\n');
  }



  function renderScreen() {
    renderPending = false;
    resolveUI();
    if (!terminalEl) return;

    var html = [];
    var maxRows = screen.length;
    if (cursorRow + 1 > maxRows) maxRows = cursorRow + 1;

    for (var r = 0; r < maxRows; r++) {
      var line = screen[r] || [];

      var overlayActive = waitingInput && lineInputMode && r === cursorRow;
      var overlayStart = cursorCol;
      var overlayText = overlayActive ? lineDraft : '';
      var needCursor = overlayActive && inputQueue.length === 0;
      var cursorDisplayCol = overlayStart + overlayText.length;

      var renderLen = line.length;
      var overlayEnd = overlayStart + overlayText.length;
      if (overlayEnd > renderLen) renderLen = overlayEnd;
      if (needCursor && cursorDisplayCol + 1 > renderLen) renderLen = cursorDisplayCol + 1;

      var currentStyle = null;
      var run = '';
      var lineHtml = '';

      for (var c = 0; c < renderLen; c++) {
        var fromOverlay = overlayActive && c >= overlayStart && c < overlayEnd;
        var cell = c < line.length ? line[c] : null;
        var ch = fromOverlay ? overlayText[c - overlayStart] : (cell && cell.ch ? cell.ch : ' ');
        var cellStyle = fromOverlay ? '' : (cell && cell.style ? cell.style : '');

        if (needCursor && c === cursorDisplayCol) {
          if (run.length > 0) {
            if (currentStyle) {
              lineHtml += '<span style="' + styleCssFromKey(currentStyle) + '">' + escapeHtml(run) + '</span>';
            } else {
              lineHtml += escapeHtml(run);
            }
            run = '';
          }
          currentStyle = null;
          lineHtml += '<span class="cursor">' + escapeHtml(ch) + '</span>';
          continue;
        }

        if (cellStyle !== currentStyle) {
          if (run.length > 0) {
            if (currentStyle) {
              lineHtml += '<span style="' + styleCssFromKey(currentStyle) + '">' + escapeHtml(run) + '</span>';
            } else {
              lineHtml += escapeHtml(run);
            }
            run = '';
          }
          currentStyle = cellStyle;
        }

        run += ch;
      }

      if (run.length > 0) {
        if (currentStyle) {
          lineHtml += '<span style="' + styleCssFromKey(currentStyle) + '">' + escapeHtml(run) + '</span>';
        } else {
          lineHtml += escapeHtml(run);
        }
      }

      html.push(lineHtml);
    }

    terminalEl.innerHTML = html.join('\n');
    terminalEl.scrollTop = terminalEl.scrollHeight; // Restore scroll to bottom
  }

  function scheduleRender() {
    if (renderPending) return;
    renderPending = true;
    window.requestAnimationFrame(renderScreen);
  }

  function enqueueCode(code) {
    inputQueue.push(code | 0);
  }

  function enqueueEnter() {
    enqueueCode(13);
  }

  function getFsSignature(path) {
    try {
      var st = FS.stat(path);
      return String(st.size) + ':' + String(Number(st.mtime || 0));
    } catch (e) {
      return null;
    }
  }

  async function loadTrackedFilesFromHost() {
    // Wait for the PIN Vault to be unlocked before proceeding
    await vaultPromise;
    
    if (typeof FS === 'undefined' || !window.supabase) {
      if (typeof window.setSyncState === 'function') window.setSyncState('error');
      // Record baseline signatures for preloaded files so they don't get uploaded
      for (var j = 0; j < trackedFiles.length; j++) {
        var s = getFsSignature(trackedFiles[j].path);
        if (s) fileSignatures[trackedFiles[j].name] = s;
      }
      initialLoadDone = true;
      return;
    }
    
    var connected = false;

    for (var i = 0; i < trackedFiles.length; i++) {
      var item = trackedFiles[i];
      try {
        const { data, error } = await window.supabase.storage
          .from(window.SUPABASE_BUCKET)
          .download(item.name);
          
        if (!error || (error.message && error.message.toLowerCase().includes('not found'))) {
          connected = true;
        }
        
        if (error || !data) continue;
        
        var bytes = new Uint8Array(await data.arrayBuffer());
        FS.writeFile(item.path, bytes);

        if (item.name === 'expenses_export.csv') {
           if (typeof window.setCsvAvailable === 'function') window.setCsvAvailable(true);
        }
      } catch (e) {
      }
    }

    // Record baseline signatures for ALL files currently in FS
    // This prevents stale preloaded files from being re-uploaded
    for (var k = 0; k < trackedFiles.length; k++) {
      var sig = getFsSignature(trackedFiles[k].path);
      if (sig) fileSignatures[trackedFiles[k].name] = sig;
    }

    cloudConnected = connected;
    if (typeof window.setSyncState === 'function') {
      window.setSyncState(connected ? 'active' : 'error');
    }
    initialLoadDone = true;
    if (connected) console.log('[web] Cloud load complete. Sync active.');
  }

  async function syncTrackedFiles(force) {
    if (syncBusy || !initialLoadDone || !cloudConnected || typeof FS === 'undefined') return;
    if (runtimeExited && !force) return;

    syncBusy = true;
    var csvTouched = false;

    try {
      for (var i = 0; i < trackedFiles.length; i++) {
        var item = trackedFiles[i];
        var sig = getFsSignature(item.path);
        if (!sig) continue;
        if (!force && fileSignatures[item.name] === sig) continue;

        // Ensure we send ONLY the data, not the whole buffer.
        // new Blob([data.buffer]) can accidentally upload much more than intended.
        var data = FS.readFile(item.path); 
        var blob = new Blob([data], { type: 'application/octet-stream' });



        var controller = new AbortController();
        var timer = window.setTimeout(function () {
          controller.abort();
        }, 5000);

        try {
          const { error } = await window.supabase.storage
            .from(window.SUPABASE_BUCKET)
            .upload(item.name, blob, { 
              upsert: true,
              cacheControl: '0',
              signal: controller.signal
            });

          if (error) {
            throw new Error('Supabase save failed: ' + error.message);
          }
        } finally {
          window.clearTimeout(timer);
        }

        fileSignatures[item.name] = sig;
        if (item.name === 'expenses_export.csv') csvTouched = true;
      }

      syncFailCount = 0;
      if (typeof window.setSyncState === 'function') window.setSyncState('active');

      if (csvTouched) {
        if (typeof window.setCsvAvailable === 'function') window.setCsvAvailable(true);
      }
    } catch (e) {
      syncFailCount++;
      if (typeof window.setSyncState === 'function') window.setSyncState('error');
      // No longer printing to terminal to avoid clutter. 
      // The sidebar indicator will show the error status.
    } finally {
      syncBusy = false;
    }
  }

  function commitLineDraft() {
    if (runtimeExited) return;

    const draft = lineDraft;

    if (inputResolver) {
      const resolve = inputResolver;
      inputResolver = null;
      lineInputMode = false;
      lineDraft = '';
      waitingInput = false;
      scheduleRender();
      resolve(draft);
      return;
    }

    for (var i = 0; i < lineDraft.length; i++) {
      enqueueCode(lineDraft.charCodeAt(i));
      writeChar(lineDraft[i]);
    }
    enqueueEnter();
    writeChar('\n');
    lineDraft = '';
    waitingInput = false;
    scheduleRender();
  }

  function bindUI() {
    if (uiBound) return;
    resolveUI();
    if (!terminalEl) return;

    uiBound = true;

    terminalEl.addEventListener('click', function () {
      terminalEl.focus();
    });

    if (closeCsvBtn) {
      closeCsvBtn.addEventListener('click', function () {
        var overlay = document.getElementById('csv-modal-overlay');
        if (overlay) overlay.classList.remove('open');
      });
    }

    /* CSV view/download button handlers are owned by web_shell.html script. */

    window.addEventListener('paste', function (e) {
      if (!lineInputMode || runtimeExited || !cloudConnected) return;

      const text = (e.clipboardData || window.clipboardData).getData('text');
      if (text) {
        lineDraft += text;
        scheduleRender();
      }
      e.preventDefault();
    });

    window.addEventListener('keydown', function (e) {
      if (runtimeExited || !cloudConnected) {
        // If the vault is locked or runtime exited, don't capture global keys
        // cloudConnected specifically becomes true only after vaultUnlock
        return;
      }

      if (e.ctrlKey || e.metaKey || e.altKey) return;

      if (lineInputMode) {
        if (e.key === 'Enter' || e.code === 'Enter' || e.code === 'NumpadEnter') {
          commitLineDraft();
          e.preventDefault();
          return;
        }

        if (e.key === 'Backspace') {
          if (lineDraft.length > 0) {
            lineDraft = lineDraft.slice(0, -1);
            scheduleRender();
          }
          e.preventDefault();
          return;
        }

        if (e.key === 'Tab') {
          lineDraft += '    ';
          scheduleRender();
          e.preventDefault();
          return;
        }

        if (e.key === 'ArrowUp' || e.key === 'ArrowDown' || e.key === 'ArrowLeft' || e.key === 'ArrowRight') {
          e.preventDefault();
          return;
        }

        if (e.key.length === 1) {
          lineDraft += e.key;
          scheduleRender();
          e.preventDefault();
        }
        return;
      }

      if (e.key === 'ArrowUp') {
        enqueueCode(224);
        enqueueCode(72);
        e.preventDefault();
        return;
      }

      if (e.key === 'ArrowDown') {
        enqueueCode(224);
        enqueueCode(80);
        e.preventDefault();
        return;
      }

      if (e.key === 'Enter' || e.code === 'Enter' || e.code === 'NumpadEnter') {
        enqueueEnter();
        e.preventDefault();
        return;
      }

      if (e.key === 'Backspace') {
        enqueueCode(8);
        e.preventDefault();
        return;
      }

      if (e.key === 'Tab') {
        enqueueCode(9);
        e.preventDefault();
        return;
      }

      if (e.key.length === 1) {
        enqueueCode(e.key.charCodeAt(0));
        e.preventDefault();
      }
    }, true);

    terminalEl.focus();
  }

  window.__wasmPopChar = function () {
    if (runtimeExited) return -1;
    if (inputQueue.length === 0) return -1;
    return inputQueue.shift();
  };

  window.__wasmHasChar = function () {
    return !runtimeExited && inputQueue.length > 0;
  };

  window.__wasmSetLineMode = function (mode) {
    lineInputMode = !!mode && !runtimeExited;
    lineDraft = '';
    if (!lineInputMode) {
      waitingInput = false;
    }
    scheduleRender();
  };

  window.__wasmSetWaitingInput = function (waiting) {
    waitingInput = !!waiting && lineInputMode && !runtimeExited;
    if (!waitingInput) {
      lineDraft = '';
    }
    scheduleRender();
  };

  window.flushInput = function() {
    inputQueue.length = 0;
  };

  window.promptForInput = function() {
    return new Promise(resolve => {
      inputResolver = resolve;
      lineInputMode = true;
      waitingInput = true;
      lineDraft = '';
      scheduleRender();
    });
  };

  window.ext_callAI = async function(summaryPtr, mode) {
    const summary = UTF8ToString(summaryPtr);
    let apiKey = '';

    if (mode === 2) {
      // Custom Key Mode
      writeChar('\n');
      writePlainLine(COLOR_CYAN + '[AI] MANUAL KEY SETUP' + COLOR_RESET);
      writePlainLine('To use your own AI intelligence, follow these steps:');
      writePlainLine('  1. Go to: ' + COLOR_YELLOW + 'https://aistudio.google.com/' + COLOR_RESET);
      writePlainLine('  2. Click "Get API Key" and copy the key (starts with AIzaSy).');
      writeChar('\n');
      
      writePlainLine('Please enter your Gemini API Key below:');
      apiKey = await window.promptForInput();
      if (!apiKey || !apiKey.startsWith('AIzaSy')) {
        writePlainLine(COLOR_RED + '[AI] ERROR: Invalid API key format.' + COLOR_RESET);
        return 0;
      }
      writePlainLine(COLOR_GREEN + '[AI] Key accepted.' + COLOR_RESET);
    } else {
      // Default Vault Mode
      apiKey = window.GEMINI_KEY;
      if (!apiKey) {
        writePlainLine(COLOR_RED + '[AI] ERROR: System Vault key not unlocked.' + COLOR_RESET);
        writePlainLine('Please ensure you entered the correct PIN at login.');
        return 0;
      }
    }

    writeChar('\n');
    writePlainLine(COLOR_CYAN + '[AI] Analyzing current month of expenses...' + COLOR_RESET);
    
    try {
      const response = await fetch(`https://generativelanguage.googleapis.com/v1/models/gemini-2.5-flash:generateContent?key=${apiKey}`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          contents: [{
            parts: [{
              text: `You are a professional Financial Advisor. 
              Analyze these expenses from the current month:
              ${summary}
              
              Rules:
              1. Be cautious. If data is sparse, acknowledge it but still provide a small insight.
              2. Highlight unusual expenses only if they are significantly higher than others.
              3. Provide EXACTLY ONE actionable tip.
              4. Be concise and use a professional tone. Keep it under 100 words.`
            }]
          }]
        })
      });

      const data = await response.json();
      if (data.error) {
        writePlainLine(COLOR_RED + '[AI] API Error: ' + data.error.message + COLOR_RESET);
        if (data.error.status === 'UNAUTHENTICATED' || data.error.status === 'INVALID_ARGUMENT') {
          writePlainLine('[AI] Hint: Your API key might be inactive or invalid.');
        }
        return 0;
      }

      const aiText = data.candidates[0].content.parts[0].text;
      
      // Typewriter effect
      writeChar('\n');
      for (let i = 0; i < aiText.length; i++) {
        writeChar(aiText[i]);
        if (i % 3 === 0) await new Promise(r => setTimeout(r, 10)); // Speed up for large text
      }
      writeChar('\n');
      
      // Wait a bit so the user can read before the menu prompt appears
      await new Promise(r => setTimeout(r, 3000));
      return 1;
    } catch (e) {
      writePlainLine(COLOR_RED + '[AI] Connection Error: ' + e.message + COLOR_RESET);
      return 0;
    }
  };

  function stdinCallback() {
    return null;
  }

  function stdoutCallback(ch) {
    writeCode(ch);
  }

  Module = Module || {};
  Module.preRun = Module.preRun || [];
  Module.noFSInit = true;

  Module.preRun.push(function () {
    FS.init(stdinCallback, stdoutCallback, stdoutCallback);

    if (typeof addRunDependency === 'function' && typeof removeRunDependency === 'function') {
      var dep = 'host-file-load';
      addRunDependency(dep);
      loadTrackedFilesFromHost().finally(function () {
        removeRunDependency(dep);
      });
    } else {
      loadTrackedFilesFromHost();
    }
  });

  Module.print = function (text) {
    if (runtimeExited) return;
    var s = String(text);
    for (var i = 0; i < s.length; i++) writeChar(s[i]);
    writeChar('\n');
  };

  Module.printErr = function (text) {
    var raw = String(text);

    if (raw.indexOf('program exited (with status: 0)') >= 0) return;
    if (raw.indexOf('native function `strerror` called after runtime exit') >= 0) return;
    if (raw.indexOf('keepRuntimeAlive() is set') >= 0) return;
    if (runtimeExited) return;

    var s = '[err] ' + raw;
    for (var i = 0; i < s.length; i++) writeChar(s[i]);
    writeChar('\n');
  };

  Module.onExit = function () {
    runtimeExited = true;
    lineInputMode = false;
    waitingInput = false;
    lineDraft = '';
    inputQueue.length = 0;
    if (syncTimer) {
      window.clearInterval(syncTimer);
      syncTimer = null;
    }
    // Final sync to make sure the last changes are saved
    syncTrackedFiles(true);
    writePlainLine('Session ended. Refresh the page to run again.');
    scheduleRender();
  };

  Module.onAbort = function () {
    runtimeExited = true;
    if (syncTimer) {
      window.clearInterval(syncTimer);
      syncTimer = null;
    }
  };

  var originalSetStatus = Module.setStatus;
  Module.setStatus = function (text) {
    if (originalSetStatus) originalSetStatus(text);
    var statusEl = document.getElementById('status');
    if (statusEl && (!text || text === '')) {
      statusEl.textContent = 'Ready';
    }
  };

  bindUI();
  window.addEventListener('beforeunload', function (e) {
    if (!runtimeExited) {
      syncTrackedFiles(true);
    }
  });

  syncTimer = window.setInterval(function () {
    syncTrackedFiles(false);
  }, 1000);

  window.setTimeout(function () {
    syncTrackedFiles(true);
  }, 2500);

  if (!uiBound) {
    window.addEventListener('DOMContentLoaded', function () {
      bindUI();
    });
  }
})();
