/**
 * x86_64 Assembler - Command-line driver
 */

#include "x86_64_asm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [options] <input.asm>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -o <file>            Output file (default: a.out)\n");
    fprintf(stderr, "  -f <fmt>             Output format: elf64, bin, hex (default: elf64)\n");
    fprintf(stderr, "  -g                   Emit lightweight debug map (<output>.dbg)\n");
    fprintf(stderr, "  -l                   Emit listing file (<output>.lst)\n");
    fprintf(stderr, "  -d                   Dump symbols and code\n");
    fprintf(stderr, "  -I <dir>             Add include search path\n");
    fprintf(stderr, "  -D <sym>[=val]       Define symbol from command line\n");
    fprintf(stderr, "  -M                   Generate Makefile dependencies\n");
    fprintf(stderr, "  -MM                  Generate Makefile dependencies (skip system includes)\n");
    fprintf(stderr, "  -E                   Preprocess only; do not assemble\n");
    fprintf(stderr, "  -W                   Enable warnings\n");
    fprintf(stderr, "  -Wall                Enable all warnings\n");
    fprintf(stderr, "  -Werror              Treat warnings as errors\n");
    fprintf(stderr, "  --error-format=fmt   Error format: text or json (default: text)\n");
    fprintf(stderr, "  --disassemble <file> Disassemble ELF64 input file\n");
    fprintf(stderr, "  -h, --help           Show this help\n");
}

static void emit_cli_error(int json, const char *category,
                           const char *message, const char *suggestion)
{
    if (json) {
        fprintf(stderr,
                "{\"severity\":\"error\",\"line\":1,\"column\":1,"
                "\"category\":\"%s\",\"message\":\"%s\","
                "\"suggestion\":\"%s\"}\n",
                category, message, suggestion);
    } else {
        fprintf(stderr,
                "Error at line 1, column 1: [%s] %s\n"
                "Suggestion: %s\n",
                category, message, suggestion);
    }
}

static int parse_cli_define(const char *str, char *name, char *value,
                            size_t name_size, size_t value_size)
{
    const char *eq = strchr(str, '=');
    size_t name_len;

    if (eq) {
        name_len = (size_t)(eq - str);
        if (name_len >= name_size) {
            name_len = name_size - 1;
        }
        memcpy(name, str, name_len);
        name[name_len] = '\0';
        if (value && value_size > 0) {
            size_t val_len = strlen(eq + 1);
            if (val_len >= value_size) {
                val_len = value_size - 1;
            }
            memcpy(value, eq + 1, val_len);
            value[val_len] = '\0';
        }
    } else {
        if (name && name_size > 0) {
            size_t n = strlen(str);
            if (n >= name_size) {
                n = name_size - 1;
            }
            memcpy(name, str, n);
            name[n] = '\0';
        }
        if (value && value_size > 0) {
            value[0] = '\0';
        }
    }

    /* Validate: non-empty and valid C identifier [A-Za-z_][A-Za-z0-9_]* */
    if (!name || !name[0]) {
        return -1;
    }
    if (!isalpha((unsigned char)name[0]) && name[0] != '_') {
        return -1;
    }
    for (size_t i = 1; name[i] != '\0'; i++) {
        if (!isalnum((unsigned char)name[i]) && name[i] != '_') {
            return -1;
        }
    }
    return 0;
}

int main(int argc, char **argv)
{
    const char *input_file = NULL;
    const char *output_file = "a.out";
    const char *format = "elf64";
    const char *disassemble_file = NULL;
    int dump = 0;
    int debug_map = 0;
    int listing = 0;
    int gen_deps = 0;
    int gen_deps_local = 0;
    int preprocess_only = 0;
    int warn = 0;
    int warn_all = 0;
    int warn_error = 0;
    int error_format_json = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            format = argv[++i];
        } else if (strcmp(argv[i], "-g") == 0) {
            debug_map = 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            listing = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            dump = 1;
        } else if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
            i++;
        } else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2] != '\0') {
            ;
        } else if (strcmp(argv[i], "-D") == 0 && i + 1 < argc) {
            i++;
        } else if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2] != '\0') {
            ;
        } else if (strcmp(argv[i], "-M") == 0) {
            gen_deps = 1;
        } else if (strcmp(argv[i], "-MM") == 0) {
            gen_deps_local = 1;
        } else if (strcmp(argv[i], "-E") == 0) {
            preprocess_only = 1;
        } else if (strcmp(argv[i], "-W") == 0) {
            warn = 1;
        } else if (strcmp(argv[i], "-Wall") == 0) {
            warn_all = 1;
        } else if (strcmp(argv[i], "-Werror") == 0) {
            warn_error = 1;
        } else if (strncmp(argv[i], "--error-format=", 15) == 0) {
            if (strcmp(argv[i] + 15, "json") == 0) {
                error_format_json = 1;
            } else if (strcmp(argv[i] + 15, "text") != 0) {
                fprintf(stderr, "Unknown error format: %s\n", argv[i] + 15);
                print_usage(argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "--disassemble") == 0 && i + 1 < argc) {
            disassemble_file = argv[++i];
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (argv[i][0] != '-') {
            input_file = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (disassemble_file) {
        if (asm_disassemble_file(disassemble_file, stdout) < 0) {
            return 1;
        }
        return 0;
    }

    if (!input_file) {
        emit_cli_error(error_format_json, "CLI",
                       "No input file specified",
                       "Provide an input .asm file path.");
        if (!error_format_json) {
            print_usage(argv[0]);
        }
        return 1;
    }

    /* Initialize assembler */
    assembler_context_t *ctx = asm_init();
    if (!ctx) {
        emit_cli_error(error_format_json, "Memory",
                       "Failed to initialize assembler",
                       "Retry and verify memory availability.");
        return 1;
    }
    asm_ctx_set_emit_debug_map(ctx, (debug_map != 0));
    asm_ctx_set_emit_listing(ctx, (listing != 0));
    asm_ctx_set_preprocess_only(ctx, (preprocess_only != 0));
    asm_ctx_set_warnings_as_errors(ctx, (warn_error != 0));
    asm_ctx_set_warn_all(ctx, (warn_all != 0));
    asm_ctx_set_warn_unused_labels(ctx, (warn != 0 || warn_all != 0));
    asm_ctx_set_error_format_json(ctx, (error_format_json != 0));
    asm_ctx_set_generate_deps(ctx, (gen_deps != 0 || gen_deps_local != 0));
    asm_ctx_set_deps_exclude_system(ctx, (gen_deps_local != 0));

    /* Second pass: collect -I, -D flags into context */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
            if (asm_ctx_add_include_path(ctx, argv[++i]) < 0) {
                fprintf(stderr, "Warning: failed to add -I path (too many or too long)\n");
            }
        } else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2] != '\0') {
            if (asm_ctx_add_include_path(ctx, argv[i] + 2) < 0) {
                fprintf(stderr, "Warning: failed to add -I path (too many or too long)\n");
            }
        } else if (strcmp(argv[i], "-D") == 0 && i + 1 < argc) {
            char d_name[MAX_LABEL_LENGTH];
            char d_value[MAX_LINE_LENGTH];
            if (parse_cli_define(argv[++i], d_name, d_value,
                                 sizeof(d_name), sizeof(d_value)) < 0) {
                emit_cli_error(error_format_json, "CLI",
                               "Invalid -D define: expected NAME[=VALUE]",
                               "Use -D NAME or -D NAME=VALUE where NAME is a valid identifier.");
                asm_free(ctx);
                return 1;
            }
            if (asm_ctx_add_cli_define(ctx, d_name, d_value) < 0) {
                fprintf(stderr, "Warning: failed to add -D define (too many or too long)\n");
            }
        } else if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2] != '\0') {
            char d_name[MAX_LABEL_LENGTH];
            char d_value[MAX_LINE_LENGTH];
            if (parse_cli_define(argv[i] + 2, d_name, d_value,
                                  sizeof(d_name), sizeof(d_value)) < 0) {
                emit_cli_error(error_format_json, "CLI",
                               "Invalid -D define: expected NAME[=VALUE]",
                               "Use -D NAME or -D NAME=VALUE where NAME is a valid identifier.");
                asm_free(ctx);
                return 1;
            }
            if (asm_ctx_add_cli_define(ctx, d_name, d_value) < 0) {
                fprintf(stderr, "Warning: failed to add -D define (too many or too long)\n");
            }
        }
    }

    /* Preprocess-only mode */
    if (asm_ctx_get_preprocess_only(ctx)) {
        char *preprocessed = asm_preprocess_file(ctx, input_file);
        if (!preprocessed) {
            const char *err = asm_get_last_error(ctx);
            if (err && err[0] != '\0') {
                fprintf(stderr, "%s\n", err);
            }
            emit_cli_error(asm_ctx_get_error_format_json(ctx), "Assembler",
                           "Preprocessing failed",
                           "Check the diagnostic emitted above for the exact failure cause.");
            asm_free(ctx);
            return 1;
        }
        printf("%s", preprocessed);
        free(preprocessed);
        asm_free(ctx);
        return 0;
    }

    if (!gen_deps && !gen_deps_local) {
        printf("Assembling: %s\n", input_file);
    }

    /* Assemble */
    if (asm_assemble_file(ctx, input_file) < 0) {
        const char *err = asm_get_last_error(ctx);
        if (err && err[0] != '\0') {
            fprintf(stderr, "%s\n", err);
        }
        emit_cli_error(asm_ctx_get_error_format_json(ctx), "Assembler",
                       "Assembly failed",
                       "Check the diagnostic emitted above for the exact failure cause.");
        asm_free(ctx);
        return 1;
    }

    /* Treat warnings as errors if -Werror was set */
    if (asm_ctx_get_fatal_warning_occurred(ctx)) {
        emit_cli_error(asm_ctx_get_error_format_json(ctx), "Assembler",
                       "Assembly failed due to warnings treated as errors",
                       "Fix the warnings above or remove -Werror.");
        asm_free(ctx);
        return 1;
    }

    /* Dependency generation mode */
    if (gen_deps || gen_deps_local) {
        printf("%s:", output_file);
        printf(" %s", input_file);
        int dep_count = asm_ctx_get_dependency_count(ctx);
        for (int i = 0; i < dep_count; i++) {
            printf(" %s", asm_ctx_get_dependency(ctx, i));
        }
        printf("\n");
        asm_free(ctx);
        return 0;
    }

    if (!gen_deps && !gen_deps_local) {
        printf("Assembly successful: %zu bytes generated\n",
               asm_ctx_get_text_size(ctx));
    }

    /* Dump if requested */
    if (dump) {
        asm_dump_symbols(ctx);
        asm_dump_output(ctx);
    }

    /* Write output */
    int result = 0;
    if (strcmp(format, "elf64") == 0) {
        result = asm_write_elf64(ctx, output_file);
    } else if (strcmp(format, "bin") == 0) {
        result = asm_write_binary(ctx, output_file);
    } else if (strcmp(format, "hex") == 0) {
        result = asm_write_hex(ctx, output_file);
    } else {
        emit_cli_error(asm_ctx_get_error_format_json(ctx), "CLI",
                       "Unknown output format",
                       "Use one of: elf64, bin, or hex.");
        result = -1;
    }

    if (result < 0) {
        emit_cli_error(asm_ctx_get_error_format_json(ctx), "I/O",
                       "Failed to write output",
                       "Check output path, permissions, and available disk space.");
        asm_free(ctx);
        return 1;
    }

    if (asm_ctx_get_emit_debug_map(ctx)) {
        if (asm_write_debug_map(ctx, output_file) < 0) {
            emit_cli_error(asm_ctx_get_error_format_json(ctx), "I/O",
                           "Failed to write debug map",
                           "Check output path permissions for <output>.dbg.");
            asm_free(ctx);
            return 1;
        }
    }

    if (asm_ctx_get_emit_listing(ctx)) {
        if (asm_write_listing(ctx, output_file) < 0) {
            emit_cli_error(asm_ctx_get_error_format_json(ctx), "I/O",
                           "Failed to write listing file",
                           "Check output path permissions for <output>.lst.");
            asm_free(ctx);
            return 1;
        }
        printf("Listing written to: %s.lst\n", output_file);
    }

    printf("Output written to: %s\n", output_file);

    asm_free(ctx);
    return 0;
}
