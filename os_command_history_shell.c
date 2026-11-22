#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define MAX_TOKEN 256
#define HISTORY_LIMIT 100

static char history_entries[HISTORY_LIMIT][MAX_LINE];
static size_t history_count = 0;
static int history_fd = -1;
static int history_warning_shown = 0;
static char history_path[PATH_MAX];

static int write_all(int fd, const char *buffer, size_t length) {
    size_t written = 0;
    while (written < length) {
        ssize_t n = write(fd, buffer + written, length - written);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        written += (size_t)n;
    }
    return 0;
}

static void add_history_memory(const char *command) {
    if (history_count == HISTORY_LIMIT) {
        memmove(history_entries, history_entries + 1,
                (HISTORY_LIMIT - 1) * sizeof(history_entries[0]));
        history_count--;
    }
    snprintf(history_entries[history_count], MAX_LINE, "%s", command);
    history_count++;
}

static void warn_history_once(const char *message) {
    if (!history_warning_shown) {
        fprintf(stderr, "History warning: %s\n", message);
        history_warning_shown = 1;
    }
}

static int configure_history_path(void) {
    const char *custom = getenv("SHELL_HISTORY_FILE");
    if (custom && custom[0] != '\0') {
        if (strlen(custom) >= sizeof(history_path)) return -1;
        snprintf(history_path, sizeof(history_path), "%s", custom);
        return 0;
    }

    const char *home = getenv("HOME");
    if (!home || home[0] == '\0') home = ".";
    int n = snprintf(history_path, sizeof(history_path), "%s/.os_shell_history", home);
    return (n < 0 || (size_t)n >= sizeof(history_path)) ? -1 : 0;
}

static void load_history(void) {
    int fd = open(history_path, O_RDONLY | O_CLOEXEC
#ifdef O_NOFOLLOW
                  | O_NOFOLLOW
#endif
    );
    if (fd < 0) {
        if (errno != ENOENT) warn_history_once(strerror(errno));
        return;
    }

    struct stat st;
    if (fstat(fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        warn_history_once("history path is not a regular file; loading skipped");
        close(fd);
        return;
    }

    FILE *file = fdopen(fd, "r");
    if (!file) {
        warn_history_once(strerror(errno));
        close(fd);
        return;
    }

    char line[MAX_LINE + 2];
    while (fgets(line, sizeof(line), file)) {
        size_t length = strlen(line);
        int complete = length > 0 && line[length - 1] == '\n';
        if (!complete && !feof(file)) {
            int ch;
            while ((ch = fgetc(file)) != '\n' && ch != EOF) {}
            continue;
        }
        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[--length] = '\0';
        }
        if (length > 0 && length < MAX_LINE) add_history_memory(line);
    }
    fclose(file);
}

static void open_history_for_append(void) {
    history_fd = open(history_path, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC
#ifdef O_NOFOLLOW
                      | O_NOFOLLOW
#endif
                      , S_IRUSR | S_IWUSR);
    if (history_fd < 0) {
        warn_history_once(strerror(errno));
        return;
    }

    struct stat st;
    if (fstat(history_fd, &st) < 0 || !S_ISREG(st.st_mode)) {
        warn_history_once("history path is not a regular file; saving disabled");
        close(history_fd);
        history_fd = -1;
        return;
    }
    if (fchmod(history_fd, S_IRUSR | S_IWUSR) < 0) {
        warn_history_once("could not restrict history file permissions");
    }
}

static void record_history(const char *command) {
    add_history_memory(command);
    if (history_fd < 0) return;
    size_t length = strlen(command);
    if (write_all(history_fd, command, length) < 0 || write_all(history_fd, "\n", 1) < 0) {
        warn_history_once("could not write to the history file");
        close(history_fd);
        history_fd = -1;
    }
}

static int parse_command(const char *line, char storage[MAX_ARGS][MAX_TOKEN], char *argv[MAX_ARGS + 1]) {
    size_t i = 0;
    int argc = 0;

    while (line[i] != '\0') {
        while (isspace((unsigned char)line[i])) i++;
        if (line[i] == '\0') break;
        if (argc >= MAX_ARGS) {
            fprintf(stderr, "Input rejected: too many arguments (maximum %d).\n", MAX_ARGS);
            return -1;
        }

        char quote = '\0';
        size_t token_length = 0;
        int token_started = 0;

        while (line[i] != '\0') {
            char c = line[i];
            if (quote == '\0' && isspace((unsigned char)c)) break;
            i++;
            token_started = 1;

            if (quote != '\0') {
                if (c == quote) {
                    quote = '\0';
                    continue;
                }
                if (c == '\\' && quote == '"') {
                    if (line[i] == '\0') {
                        fprintf(stderr, "Input rejected: incomplete escape sequence.\n");
                        return -1;
                    }
                    c = line[i++];
                }
            } else {
                if (c == '\'' || c == '"') {
                    quote = c;
                    continue;
                }
                if (c == '\\') {
                    if (line[i] == '\0') {
                        fprintf(stderr, "Input rejected: incomplete escape sequence.\n");
                        return -1;
                    }
                    c = line[i++];
                }
            }

            if (iscntrl((unsigned char)c)) {
                fprintf(stderr, "Input rejected: unsupported control character.\n");
                return -1;
            }
            if (token_length + 1 >= MAX_TOKEN) {
                fprintf(stderr, "Input rejected: an argument exceeds %d characters.\n", MAX_TOKEN - 1);
                return -1;
            }
            storage[argc][token_length++] = c;
        }

        if (quote != '\0') {
            fprintf(stderr, "Input rejected: unmatched quotation mark.\n");
            return -1;
        }
        if (token_started) {
            storage[argc][token_length] = '\0';
            argv[argc] = storage[argc];
            argc++;
        }
    }
    argv[argc] = NULL;
    return argc;
}

static void show_history(void) {
    for (size_t i = 0; i < history_count; i++) {
        printf("%3zu  %s\n", i + 1, history_entries[i]);
    }
}

static void search_history(const char *term) {
    int found = 0;
    for (size_t i = 0; i < history_count; i++) {
        if (strstr(history_entries[i], term) != NULL) {
            printf("%3zu  %s\n", i + 1, history_entries[i]);
            found = 1;
        }
    }
    if (!found) printf("No history entries matched: %s\n", term);
}

static int run_external(char *argv[]) {
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    int status;
    while (waitpid(child, &status, 0) < 0) {
        if (errno == EINTR) continue;
        perror("waitpid");
        return 1;
    }
    if (WIFEXITED(status)) return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "Command ended after signal %d.\n", WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return 1;
}

int main(void) {
    if (configure_history_path() < 0) {
        fprintf(stderr, "History warning: history file path is too long; persistence disabled.\n");
        snprintf(history_path, sizeof(history_path), ".os_shell_history");
    }
    load_history();
    open_history_for_append();

    char line[MAX_LINE];
    char storage[MAX_ARGS][MAX_TOKEN];
    char *argv[MAX_ARGS + 1];

    puts("OS Command History Shell");
    puts("Built-ins: cd, history, search <text>, exit");

    for (;;) {
        printf("os-shell> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            putchar('\n');
            break;
        }

        size_t length = strlen(line);
        int has_newline = length > 0 && line[length - 1] == '\n';
        if (!has_newline && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            fprintf(stderr, "Input rejected: command exceeds %d characters.\n", MAX_LINE - 1);
            continue;
        }

        while (length > 0 && (line[length - 1] == '\n' || line[length - 1] == '\r')) {
            line[--length] = '\0';
        }
        if (length == 0) continue;

        int argc = parse_command(line, storage, argv);
        if (argc < 0) continue;
        if (argc == 0) continue;

        if (strcmp(argv[0], "exit") != 0) record_history(line);

        if (strcmp(argv[0], "exit") == 0) {
            if (argc != 1) fprintf(stderr, "Usage: exit\n");
            else break;
        } else if (strcmp(argv[0], "cd") == 0) {
            if (argc > 2) {
                fprintf(stderr, "Usage: cd [directory]\n");
            } else {
                const char *target = (argc == 1) ? getenv("HOME") : argv[1];
                if (!target || chdir(target) < 0) perror("cd");
            }
        } else if (strcmp(argv[0], "history") == 0) {
            if (argc != 1) fprintf(stderr, "Usage: history\n");
            else show_history();
        } else if (strcmp(argv[0], "search") == 0) {
            if (argc != 2) fprintf(stderr, "Usage: search <text>\n");
            else search_history(argv[1]);
        } else {
            (void)run_external(argv);
        }
    }

    if (history_fd >= 0) close(history_fd);
    puts("Shell closed.");
    return 0;
}
