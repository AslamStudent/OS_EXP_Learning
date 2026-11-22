# OS Command History Shell

A Linux shell prototype for the Operating Systems Unit V individual experiential learning project. The assigned feature is command history for a Linux-based kiosk use case. The program keeps a persistent history, allows text search, and checks malformed or oversized input before running a command.

## Student and course details

- Student: Mohammed Aslam Shahid
- USN: 25BTRCL299
- Student number: 63
- Branch and section: CSE - AIML C
- Course: Operating Systems
- Course code: 25PCCS201
- Individual project: 63

## GitHub repository

https://github.com/AslamStudent/OS_EXP_Learning

## Features

- Loads saved command history when the shell starts.
- Appends accepted commands to a history file.
- Displays numbered entries with `history`.
- Searches saved entries with `search <text>`.
- Supports the built-in commands `cd`, `history`, `search`, and `exit`.
- Supports single-quoted and double-quoted arguments and basic backslash escaping.
- Rejects command lines longer than 1,023 characters.
- Rejects unmatched quotation marks, incomplete escapes, control characters, arguments longer than 255 characters, and more than 64 arguments.
- Runs external commands using `fork()`, `execvp()`, and `waitpid()`.
- Creates the history file with owner-only read/write permissions and rejects a symbolic-link history path on systems that support `O_NOFOLLOW`.

## Requirements

- Linux environment (for example, a Linux virtual machine or GitHub Codespaces)
- GCC compiler
- Standard C library and Linux/POSIX process and file interfaces

## Compile

Open a terminal in this directory and run:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 os_command_history_shell.c -o os_command_history_shell
```

## Run

```bash
./os_command_history_shell
```

The default history file is `~/.os_shell_history`. To use a separate history file during testing, run:

```bash
SHELL_HISTORY_FILE=/tmp/aslam_history.txt ./os_command_history_shell
```

## Built-in commands

```text
history
search echo
cd /path/to/directory
exit
```

Other supported commands are launched as foreground child processes. This prototype does not implement pipelines, background jobs, input/output redirection, or a full kiosk command allow-list.

## Test cases and evidence

The project test record contains eight test cases covering quoted command execution, search and history, persistence across sessions, unmatched quotation marks, oversized input, excessive arguments, unknown-command recovery, and symbolic-link history-path handling. See `test_results.txt` for the input, expected result, actual result, and status for each case. `build_verification.txt` records the GCC build command, compiler environment, exit status, and verification summary.

The `screenshots/` folder contains six terminal captures supplied for the report:

1. `test1_command_execution.png` - quoted command execution.
2. `test2_history_display.png` - numbered command history.
3. `test3_history_search.png` - search for a command containing `Hello`.
4. `test4_persistence_after_restart.png` - history displayed after restarting the shell.
5. `test5_unmatched_quote.png` - rejection of an unmatched quotation mark.
6. `test6_oversized_input.png` - rejection of an oversized input line.

## Scope and limitations

This is an educational shell prototype, not a hardened kiosk deployment. It runs commands available to the current Linux user. A real kiosk environment would also need a restricted operating-system account, a command allow-list, filesystem and desktop restrictions, and administrative configuration.

The final PDF report includes an algorithm flowchart, explained source-code extracts, an expected-versus-actual test table, the student-supplied terminal screenshots, and a separate compilation verification log. The terminal screenshots are included so that the implementation and its test evidence can be reviewed together.
