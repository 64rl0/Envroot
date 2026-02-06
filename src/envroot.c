// envroot.c
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
typedef struct {
    char *buf;
    size_t len;
    size_t cap;
} StrBuf;
static void die_msg(const char *msg) {
    fprintf(stderr, "envroot: %s\n", msg);
    exit(1);
}
static void die_detail(const char *msg, const char *detail) {
    fprintf(stderr, "envroot: %s: %s\n", msg, detail);
    exit(1);
}
static void sb_init(StrBuf *sb) {
    sb->cap = 256;
    sb->len = 0;
    sb->buf = (char *)malloc(sb->cap);
    if (!sb->buf) {
        die_msg("out of memory");
    }
    sb->buf[0] = '\0';
}
static void sb_ensure(StrBuf *sb, size_t extra) {
    if (sb->len + extra + 1 <= sb->cap) {
        return;
    }
    size_t new_cap = sb->cap * 2;
    while (new_cap < sb->len + extra + 1) {
        new_cap *= 2;
    }
    char *new_buf = (char *)realloc(sb->buf, new_cap);
    if (!new_buf) {
        die_msg("out of memory");
    }
    sb->buf = new_buf;
    sb->cap = new_cap;
}
static void sb_append(StrBuf *sb, const char *s, size_t n) {
    sb_ensure(sb, n);
    memcpy(sb->buf + sb->len, s, n);
    sb->len += n;
    sb->buf[sb->len] = '\0';
}
static void sb_append_c(StrBuf *sb, char c) {
    sb_ensure(sb, 1);
    sb->buf[sb->len++] = c;
    sb->buf[sb->len] = '\0';
}
static char *strip_quotes(const char *s) {
    size_t n = strlen(s);
    char *out = (char *)malloc(n + 1);
    if (!out) {
        die_msg("out of memory");
    }
    size_t j = 0;
    for (size_t i = 0; i < n; i++) {
        if (s[i] != '"' && s[i] != '\'') {
            out[j++] = s[i];
        }
    }
    out[j] = '\0';
    return out;
}
static char *resolve_list_suffix(const char *spec) {
    const char *p = spec;
    if (*p != '$') {
        return NULL;
    }
    const char *var_start = NULL;
    const char *var_end = NULL;
    const char *suffix = NULL;
    if (p[1] == '{') {
        var_start = p + 2;
        const char *brace = strchr(var_start, '}');
        if (!brace || brace[1] != '/') {
            return NULL;
        }
        var_end = brace;
        suffix = brace + 1;
    } else {
        var_start = p + 1;
        const char *q = var_start;
        while (*q && (isalnum((unsigned char)*q) || *q == '_')) {
            q++;
        }
        if (*q != '/') {
            return NULL;
        }
        var_end = q;
        suffix = q;
    }
    if (!var_start || !var_end || var_end == var_start) {
        return NULL;
    }
    size_t var_len = (size_t)(var_end - var_start);
    char *var = (char *)malloc(var_len + 1);
    if (!var) {
        die_msg("out of memory");
    }
    memcpy(var, var_start, var_len);
    var[var_len] = '\0';
    const char *val = getenv(var);
    free(var);
    if (!val || !*val) {
        return NULL;
    }
    if (strchr(val, ':')) {
        char *val_copy = strdup(val);
        if (!val_copy) {
            die_msg("out of memory");
        }
        char *saveptr = NULL;
        char *tok = strtok_r(val_copy, ":", &saveptr);
        while (tok) {
            if (*tok) {
                size_t cand_len = strlen(tok) + strlen(suffix);
                char *cand = (char *)malloc(cand_len + 1);
                if (!cand) {
                    free(val_copy);
                    die_msg("out of memory");
                }
                strcpy(cand, tok);
                strcat(cand, suffix);
                if (access(cand, X_OK) == 0) {
                    free(val_copy);
                    return cand;
                }
                free(cand);
            }
            tok = strtok_r(NULL, ":", &saveptr);
        }
        free(val_copy);
        return NULL;
    }
    size_t cand_len = strlen(val) + strlen(suffix);
    char *cand = (char *)malloc(cand_len + 1);
    if (!cand) {
        die_msg("out of memory");
    }
    strcpy(cand, val);
    strcat(cand, suffix);
    if (access(cand, X_OK) == 0) {
        return cand;
    }
    free(cand);
    return NULL;
}
static char *expand_vars(const char *input) {
    StrBuf sb;
    sb_init(&sb);
    const char *rest = input;
    while (*rest) {
        if (*rest != '$') {
            sb_append_c(&sb, *rest++);
            continue;
        }
        rest++; // skip '$'
        if (*rest == '{') {
            rest++;
            const char *var_start = rest;
            while (*rest && *rest != '}') {
                rest++;
            }
            size_t var_len = (size_t)(rest - var_start);
            if (*rest == '}') {
                rest++;
            }
            if (var_len == 0) {
                sb_append_c(&sb, '$');
                continue;
            }
            char *var = (char *)malloc(var_len + 1);
            if (!var) {
                die_msg("out of memory");
            }
            memcpy(var, var_start, var_len);
            var[var_len] = '\0';
            const char *val = getenv(var);
            if (!val || !*val) {
                fprintf(stderr, "envroot: %s is not set\n", var);
                free(var);
                exit(1);
            }
            sb_append(&sb, val, strlen(val));
            free(var);
        } else {
            const char *var_start = rest;
            while (*rest && (isalnum((unsigned char)*rest) || *rest == '_')) {
                rest++;
            }
            size_t var_len = (size_t)(rest - var_start);
            if (var_len == 0) {
                sb_append_c(&sb, '$');
                continue;
            }
            char *var = (char *)malloc(var_len + 1);
            if (!var) {
                die_msg("out of memory");
            }
            memcpy(var, var_start, var_len);
            var[var_len] = '\0';
            const char *val = getenv(var);
            if (!val || !*val) {
                fprintf(stderr, "envroot: %s is not set\n", var);
                free(var);
                exit(1);
            }
            sb_append(&sb, val, strlen(val));
            free(var);
        }
    }
    return sb.buf;
}
int main(int argc, char **argv) {
    const char *template_arg = NULL;
    int shift = 0;
    if (argc >= 3) {
        template_arg = argv[1];
        shift = 1;
    }
    if (!template_arg || !*template_arg) {
        die_msg("missing EXECUTABLE (argv[1])");
    }
    char *template = strip_quotes(template_arg);
    char *resolved = resolve_list_suffix(template);
    if (!resolved || access(resolved, X_OK) != 0) {
        free(resolved);
        resolved = expand_vars(template);
    }
    if (!resolved || access(resolved, X_OK) != 0) {
        die_detail("EXECUTABLE is not executable", resolved ? resolved : "");
    }
    int exec_argc = argc - shift;
    char **exec_argv = (char **)calloc((size_t)exec_argc + 1, sizeof(char *));
    if (!exec_argv) {
        die_msg("out of memory");
    }
    exec_argv[0] = resolved;
    for (int i = 1; i < exec_argc; i++) {
        exec_argv[i] = argv[i + shift];
    }
    exec_argv[exec_argc] = NULL;
    execv(resolved, exec_argv);
    fprintf(stderr, "envroot: exec failed: %s\n", strerror(errno));
    return 1;
}