# nshell — A POSIX Shell in C++

A custom POSIX-like shell built from scratch in C++17. Supports builtin commands, foreground/background process execution, pipelines, I/O redirection, signal handling, and persistent history — all structured around four classic **design patterns**.

---

## Table of Contents

- [Building \& Running](#building--running)
- [Features](#features)
  - [Builtin Commands](#builtin-commands)
  - [External Commands](#external-commands)
  - [Piping](#piping)
  - [I/O Redirection](#io-redirection)
  - [Foreground \& Background Execution](#foreground--background-execution)
  - [Signal Handling](#signal-handling)
  - [Command Sequencing](#command-sequencing)
  - [Quote Handling](#quote-handling)
- [Architecture \& Design Patterns](#architecture--design-patterns)
  - [1. Command Pattern](#1-command-pattern)
  - [2. Chain of Responsibility — Parsing Pipeline](#2-chain-of-responsibility--parsing-pipeline)
  - [3. State Pattern — Job Control](#3-state-pattern--job-control)
  - [4. Facade Pattern — ShellContext](#4-facade-pattern--shellcontext)
- [Project Structure](#project-structure)
- [Testing](#testing)

---

## Building & Running

```bash
make clean && make
./nshell
```

Requires **g++** with **C++17** support.

---

## Features

### Builtin Commands

| Command | Description | Flags / Arguments |
|---------|-------------|-------------------|
| `cd [dir]` | Change working directory | `~` (home), `.` (current), `..` (parent), `-` (previous), `~/path` (home-relative), relative paths, absolute paths. No arguments → home. |
| `echo [text]` | Print text to stdout | Supports single and double quoted strings. Collapses consecutive whitespace in unquoted arguments. |
| `pwd` | Print current working directory | — |
| `ls [flags] [dir...]` | List directory contents | `-a` (show hidden), `-l` (long format), `-al` / `-la` (both). Supports multiple directory arguments. `~` resolves to home. |
| `pinfo [pid]` | Display process information | No argument → shell's own PID. Shows PID, process state (with `+` if foreground), virtual memory size, and executable path via `/proc`. |
| `search <name>` | Recursive file/directory search | Searches from CWD downward using iterative DFS. Prints `Yes` or `No`. |
| `history [n]` | Show command history | No argument or `0` → last 10 commands. `n` → last n commands (max 20 stored). Persisted to `~/history.txt`. |
| `exit` | Exit the shell | — |

### External Commands

Any command not recognized as a builtin is executed via `execvp`, which searches the system `PATH`. Examples: `cat`, `grep`, `wc`, `head`, `tr`, `sleep`, etc.

### Piping

Commands can be chained with the pipe operator `|`. Each stage runs in its own child process with `stdout` of the previous stage connected to `stdin` of the next.

```
ls -l | head -n 5
cat file.txt | grep "pattern" | wc -l
echo hello | tr a-z A-Z
```

- Unlimited pipeline depth
- Both builtins and external commands can appear in any pipeline stage
- Pipes respect quote boundaries (a `|` inside quotes is literal)

### I/O Redirection

| Operator | Description |
|----------|-------------|
| `>` | Redirect stdout to a file (overwrite) |
| `>>` | Redirect stdout to a file (append) |
| `<` | Redirect stdin from a file |

```
echo hello > output.txt
echo world >> output.txt
cat < input.txt | wc -c
ls | wc -l > count.txt
```

Redirection can be combined with piping. Redirections are parsed as part of the AST and applied in child processes before command execution.

### Foreground & Background Execution

- **Foreground** (default): The shell waits for the command to finish before showing the next prompt.
- **Background** (trailing `&`): The shell prints the child PID and immediately returns to the prompt.

```
sleep 10 &         # Runs in background, prints PID
ls -l | sort &     # Entire pipeline runs in background
```

Background children are reaped via `SIGCHLD` handling to prevent zombie processes.

### Signal Handling

| Signal | Behavior |
|--------|----------|
| `Ctrl+C` (`SIGINT`) | Sends `SIGINT` to the foreground process group. If no foreground job exists, prints a message. The shell itself is never killed. |
| `Ctrl+Z` (`SIGTSTP`) | Suspends the foreground process group with `SIGSTOP`. The shell regains control. |
| `Ctrl+D` (EOF) | Exits the shell. |
| `SIGCHLD` | Asynchronously reaps terminated background children to prevent zombies. |

All signals use `sigaction` with `SA_RESTART` to avoid interrupted system call issues. Signal handlers set atomic flags that are checked between commands via `handlePendingSignals()`.

### Command Sequencing

Multiple commands can be separated by `;` on a single line:

```
cd /tmp ; ls -l ; pwd
echo one ; echo two ; echo three
```

Semicolons inside quotes are treated as literal characters.

### Quote Handling

- **Single quotes** (`'...'`): Content is treated literally. No interpretation of special characters.
- **Double quotes** (`"..."`): Content is preserved as-is. Whitespace and special characters within are not split.

Quotes are respected during semicolon splitting, pipe splitting, and tokenization — a `;`, `|`, or `>` inside quotes is never treated as a delimiter.

---

## Architecture & Design Patterns

The codebase is structured around four design patterns that separate concerns and make the shell extensible.

### 1. Command Pattern

**Goal**: Unify all builtin commands under a single interface, replacing a fragile switch-case dispatcher.

**Implementation**:

```
ICommand (interface)
├── CdCommand
├── EchoCommand
├── PwdCommand
├── LsCommand
├── PinfoCommand
├── SearchCommand
└── HistoryCommand
```

- `ICommand` defines a single method: `execute(ShellContext&, ExecContext&) → int`
- `CommandRegistry` maps command names (`"cd"`, `"ls"`, etc.) to `ICommand` instances via `std::unordered_map<string, unique_ptr<ICommand>>`
- At runtime, `commandRegistry.lookup("ls")` returns the `LsCommand*`, or `nullptr` for external commands → fall through to `execvp`

**Key files**: `ICommand.h`, `CommandRegistry.h`, `CommandRegistry.cpp`, `ExecContext.h`

**Before**: A string array + index lookup (`isMyCommand`) feeding a `switch(int)` with 7 cases.
**After**: A type-safe map with polymorphic dispatch. Adding a new builtin = one class + one `registerCommand()` call.

---

### 2. Chain of Responsibility — Parsing Pipeline

**Goal**: Replace the monolithic ad-hoc parsing (scattered `splitByDelimiter`, `hasPipes`, `tokenizeLine`, inline redirection extraction) with a modular, staged pipeline.

**Implementation**:

```
raw input string
    ↓ SequenceSplitStage    — splits by ; (respecting quotes)
    ↓ PipelineSplitStage    — splits by | (respecting quotes)
    ↓ CommandParseStage     — tokenizes each segment into args
    ↓ BackgroundDetectStage — strips trailing & and sets flag
    ↓ RedirectionExtractStage — extracts >, >>, < into Redirection structs
SequenceNode (fully structured AST)
```

Each stage implements `IParseStage::process(ParseContext&)`. The chain is linked via `setNext()`. If any stage detects a syntax error (e.g., `>` with no filename), it sets `ctx.valid = false` and the chain short-circuits.

**AST node hierarchy** (defined in `ParseTypes.h`):
```
SequenceNode
 └── vector<PipelineNode>
       ├── rawText (for history)
       ├── background (bool)
       └── vector<CommandNode>
             ├── args (vector<string>)
             └── redirections (vector<Redirection>)
                   ├── type: Input | Output | Append
                   └── filename
```

**Key files**: `ParseTypes.h`, `ParseChain.h`, `ParseChain.cpp`

**Before**: Redirection parsing was duplicated in two 40-line inline blocks (`handleRedirectionswithoutPipe` and `handleRedirectionswithPipe`). The REPL had to manually check `hasPipes()` and branch into different code paths.
**After**: One parse chain produces a clean AST. One `applyRedirections()` function (20 lines) handles all redirection. The REPL simply iterates over `SequenceNode.pipelines`.

---

### 3. State Pattern — Job Control

**Goal**: Centralize foreground/background/stopped/done job lifecycle management instead of scattering raw PID and boolean manipulation across 6+ call sites.

**Implementation**:

```cpp
enum class JobState { Foreground, Background, Stopped, Done };

struct Job {
    pid_t pgid;
    JobState state;
};

class JobManager {
    void startForeground(pid_t pgid);   // → Foreground state
    void startBackground(pid_t pgid);   // → Background state, prints PID
    void suspendForeground();           // → Stopped state, sends SIGSTOP
    void interruptForeground();         // sends SIGINT to fg group
    void reapChildren();               // waitpid WNOHANG loop
    void waitForForeground();           // blocking wait
    bool hasForeground() const;
};
```

All state transitions go through named methods. Signal handlers call `jobManager.interruptForeground()` / `jobManager.suspendForeground()` instead of directly manipulating PIDs and booleans.

**Key file**: `JobManager.h`

**Before**: `shellContext.foregroundPID = pid; shellContext.foreground = true;` repeated at 6 sites with subtle variations.
**After**: `context.jobManager.startForeground(pid)` — one call, one place that defines the transition.

---

### 4. Facade Pattern — ShellContext

**Goal**: Consolidate all shell state into a single entry point, eliminating the long parameter lists that were threaded through every function.

**Implementation**:

```cpp
struct ShellContext {
    // Environment
    string systemName, username, homeDir;

    // Working directory
    string currDir, prevDir;
    DIR* currDirHandle, *prevDirHandle;

    // Subsystems
    JobManager jobManager;
    vector<string> historyStore;

    // Facade helpers
    string promptPath() const;  // currDir with ~ substitution
    string prompt() const;      // "user@host:path> "
};
```

Every function that needs shell state takes a single `ShellContext&` parameter. The prompt is generated via `shellContext.prompt()` instead of manual string concatenation.

**Key file**: `ShellContext.h`

**Before**: Functions like `handleRedirectionswithoutPipe` took 8 parameters (`DIR *curr, DIR *prev, string &currD, string &prevD, const string &home_dir, ...`).
**After**: Same function takes 3 parameters (`command, background, ShellContext&`). Directory state, environment, job control, and history are all accessed through the facade.

---

## Project Structure

```
POSIX_SHELL/
├── main.cpp               # REPL loop, signal setup, AST-driven execution
├── ShellContext.h          # Facade: all shell state + prompt helpers
├── JobManager.h            # State pattern: job lifecycle management
│
├── ICommand.h              # Command pattern: interface
├── ExecContext.h            # Per-command execution context
├── CommandRegistry.h/cpp    # Command registry + 7 builtin implementations
│
├── ParseTypes.h             # AST nodes: Sequence, Pipeline, Command, Redirection
├── ParseChain.h/cpp         # Chain of Responsibility: 5-stage parse pipeline
│
├── cd.cpp/h                 # cd builtin implementation
├── echos.cpp/h              # echo + pwd implementations
├── ls.cpp/h                 # ls builtin (-a, -l, -al, directory args)
├── history.cpp/h            # History management + file persistence
├── pinfo.cpp/h              # Process info via /proc
├── search.cpp/h             # Recursive file search (DFS)
├── getInfo.cpp/h            # System info (username, hostname, home dir)
│
├── commandCentre.cpp/h      # Legacy dispatcher (kept for reference, unused)
│
├── Makefile                 # Build system (g++, C++17)
├── run_tests.sh             # Automated test harness (36 tests)
├── tests.txt                # Test cases documentation
└── history.txt              # Persistent history file
```

---

## Testing

An automated test harness covers all shell features:

```bash
bash run_tests.sh
```

**36 tests** across 12 categories:

| Category | Tests |
|----------|-------|
| Core builtins | `pwd`, `cd /`, `cd ~`, `cd -` |
| Echo & piping | `echo alpha`, `echo alpha \| tr a-z A-Z` |
| Redirection | `>` (overwrite), `>>` (append), `<` (input), combined with pipes |
| ls variants | `ls`, `ls -a`, `ls -l`, piped to `wc -l` |
| Pipelines | `ls -l \| head -n 2`, `cat /etc/hostname \| wc -c` |
| Search | `search Makefile` (Yes), `search does_not_exist` (No) |
| pinfo | Process info for shell PID |
| History | Recording and retrieval |
| Background jobs | `sleep 1 &` with PID output |
| Complex combos | Builtins + external commands in pipes and redirections |
| Mixed redir+pipes | `cat < file \| wc -c`, `ls \| wc -l > file` |

```
=== Results: 36 passed, 0 failed, 36 total ===
```
