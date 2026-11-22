# OS Command History Shell

A Linux shell prototype for the Operating Systems Unit V individual experiential learning project. The assigned feature is command history for a Linux-based kiosk use case, including persistent history, text search, and safe handling of malformed or oversized input.

## Student and course details

- Student: Mohammed Aslam Shahid
- USN: 25BTRCL299
- Project allocation: Individual Project 63
- Branch and section: CSE - AIML C
- Course: Operating Systems
- Course code: 25PCCS201

## Features

- Loads saved command history when the shell starts.
- Saves accepted commands to a history file.
- Displays numbered entries with the `history` command.
- Searches history with `search <text>`.
- Provides the built-in commands `cd`, `history`, `search`, and `exit`.
- Supports single-quoted and double-quoted arguments and basic backslash escaping.
- Rejects command lines longer than 1,023 characters.
- Rejects unmatched quotation marks, incomplete escapes, control characters, arguments longer than 255 characters, and commands with more than 64 arguments.
- Runs external commands using `fork()`, `execvp()`, and `waitpid()`.
- Creates the history file with owner-only read and write permissions and refuses a symbolic-link history path on systems that support `O_NOFOLLOW`.

## Requirements

- Linux or a Linux virtual machine
- GCC compiler
- Standard C library and POSIX/Linux process and file interfaces

## Compile

Open a terminal in this project directory and run:

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 os_command_history_shell.c -o os_command_history_shell
```

## Run

```bash
./os_command_history_shell
```

The default history file is `~/.os_shell_history`. To use a separate history file during testing, set `SHELL_HISTORY_FILE` before launching the program:

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

Any other supported command is launched as a foreground child process. This prototype does not implement pipelines, background jobs, input/output redirection, or a kiosk command allow-list.

## Test cases

The implementation has been checked with the following test cases. Example output excerpts and observations are recorded in `test_results.txt`.

1. Execute a command with a quoted argument.
2. Display numbered history and search for a text string.
3. Close and reopen the shell to verify that history persists.
4. Enter a command with an unmatched quotation mark.
5. Enter a line longer than 1,023 characters.
6. Enter more than 64 arguments.
7. Run an unknown command and confirm that the shell remains usable.
8. Configure a symbolic-link history path and verify that the target file is not modified.

Run the tests again in the approved university Linux environment and retain your own terminal screenshots or logs as submission evidence.

## Scope and limitation

This is an educational shell prototype, not a hardened kiosk deployment. It runs commands available to the current Linux user. A real kiosk environment would also need an appropriately restricted operating-system account, command allow-list, filesystem restrictions, desktop restrictions, and administrative configuration.

## Project files

- `os_command_history_shell.c`: C source code.
- `test_results.txt`: recorded test observations.
- `.gitignore`: excludes the compiled executable and local history file from Git.
