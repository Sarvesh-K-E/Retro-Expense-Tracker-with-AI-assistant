# Personal Finance Management System

A WebAssembly-powered, retro-terminal styled expense tracker with cloud synchronization and AI financial insights. Built in C and deployed to the modern web.

## Overview

The Personal Finance Management System is a lightweight, high-performance financial tracking application. Originally written in C, it is compiled to WebAssembly (Wasm) to run entirely within the browser. It features a custom ANSI-compliant terminal emulator wrapped in a sleek, retro-inspired graphical interface. The application offers a robust set of tools to manage expenses, track budgets, and gain AI-driven insights natively without needing a traditional backend server.

## Technologies Used

*   **C (C11):** Core logic, utilizing standard library I/O adapted for Emscripten's virtual file systems.
*   **WebAssembly (Wasm):** The C code is compiled into WebAssembly to execute securely and directly in the browser environment.
*   **Emscripten:** Used as the compiler toolchain to handle synchronous C input paradigms natively in an asynchronous browser thread via Asyncify.
*   **HTML, CSS, Vanilla JavaScript:** Provides the web layer, including the glassmorphic terminal shell, retro theme rendering, and bridging between JavaScript APIs and the Wasm standard streams.
*   **Supabase:** Used as the storage API for cloud synchronization to seamlessly persist data files across devices.
*   **Google Gemini API:** Integrated to analyze monthly spending and provide actionable, context-aware financial tips.

## Key Features

*   **Blazing Fast Wasm Core:** The entire backend logic runs securely and instantly in the browser.
*   **Retro Terminal Interface:** Full ANSI color support, custom keyboard navigation, and an interactive command-line experience built for the web.
*   **Cloud Sync:** Secure, PIN-protected synchronization with Supabase storage to persist serialized database files.
*   **AI Financial Advisor:** Integrated Google Gemini architecture to provide context-aware financial tips based on your spending.
*   **Comprehensive Analytics:** Real-time monthly budget dashboard, detailed itemized reports, and category-wise spending charts.
*   **Data Portability:** Complete import and export capabilities utilizing standard CSV formats, including a drag-and-drop web import module.
*   **PIN Protection System:** A secure PIN layer used to decrypt API keys and unlock the cloud sync environments.

## How It Works

1.  **Initialization:** Upon launching the application, the user is prompted to enter a numeric PIN to unlock the environment.
2.  **Decryption & Sync:** The PIN decrypts the API keys via XOR encryption. Once authenticated, the terminal initializes and loads the user's host files from the Supabase cloud bucket into Emscripten's virtual file system.
3.  **User Interaction:** The core C loop runs seamlessly in the browser. System navigation leverages standard keyboard interfaces (Up/Down arrows, Enter). Emscripten's Asyncify prevents the application from blocking the browser's main thread during user input.
4.  **AI Insights:** When the AI Advisor is triggered, the C program summarizes the current month's expenses and passes this context to JavaScript via an Emscripten bridge. JavaScript makes the API call to Gemini and returns the insights back to the C program for display in the terminal.
5.  **Cloud Persistence:** Any modifications made to the expenses trigger a background synchronization routine that uploads the updated database files back to the Supabase cloud bucket.

## Build and Deployment

### Prerequisites

*   Emscripten SDK (emsdk) configured in your environment PATH.
*   PowerShell (for executing the build script).

### Compilation

1.  Navigate to the `src` directory containing the source files.
2.  Execute the automated PowerShell build sequence:
    ```powershell
    .\build_web.ps1
    ```
    This compiles `main.c`, initializes the Emscripten runtime, and bundles `web_shell.html` and `web_terminal.js` into the root directory.

### Running Locally

Start a local static server to serve the generated Wasm and HTML files. For example, using Python:
```bash
python -m http.server 8000
```
Navigate to `http://localhost:8000` in your preferred browser.

## Configuration

To prevent exposing sensitive credentials, the Supabase and Gemini API keys are not stored as plain text. The project uses placeholders for these keys in `index.html` and `src/web_shell.html`. 

To use your own Supabase project and Gemini API, you must encrypt your URLs and Keys by XOR-ing them with your chosen PIN, encoding the result in base64, and replacing the `encUrl`, `encKey`, and `encAi` placeholder variables in the source files.
