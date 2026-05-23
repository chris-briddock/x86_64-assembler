/**
 * x86_64 Assembler - Command-line driver
 */

#include "x86_64_asm.h"
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

static void safe_copy(char *dst, size_t dst_size, const char *src)
{
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    size_t n = strlen(src);
    if (n >= dst_size) {
        n = dst_size - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
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

    if (eq) {
        size_t name_len = (size_t)(eq - str);

        if (name_len >= name_size) {
            name_len = name_size - 1;
        }
        memcpy(name, str, name_len);
        name[name_len] = '\0';
        safe_copy(value, value_size, eq + 1);
    } else {
        safe_copy(name, name_size, str);
        value[0] = '\0';
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
        print_usage(argv[0]);
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
    ctx->emit_debug_map = (debug_map != 0);
    ctx->emit_listing = (listing != 0);
    ctx->preprocess_only = (preprocess_only != 0);
    ctx->warnings_as_errors = (warn_error != 0);
    ctx->warn_all = (warn_all != 0);
    ctx->warn_unused_labels = (warn != 0 || warn_all != 0);
    ctx->error_format_json = (error_format_json != 0);
    ctx->generate_deps = (gen_deps != 0 || gen_deps_local != 0);
    ctx->deps_exclude_system = (gen_deps_local != 0);

    /* Second pass: collect -I, -D flags into context */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-I") == 0 && i + 1 < argc) {
            if (ctx->include_path_count < MAX_INCLUDE_PATHS) {
                safe_copy(ctx->include_paths[ctx->include_path_count],
                          MAX_FILEPATH_LENGTH, argv[++i]);
                ctx->include_path_count++;
            } else {
                fprintf(stderr, "Warning: too many -I paths (max %d)\n",
                        MAX_INCLUDE_PATHS);
            }
        } else if (strncmp(argv[i], "-I", 2) == 0 && argv[i][2] != '\0') {
            if (ctx->include_path_count < MAX_INCLUDE_PATHS) {
                safe_copy(ctx->include_paths[ctx->include_path_count],
                          MAX_FILEPATH_LENGTH, argv[i] + 2);
                ctx->include_path_count++;
            } else {
                fprintf(stderr, "Warning: too many -I paths (max %d)\n",
                        MAX_INCLUDE_PATHS);
            }
        } else if (strcmp(argv[i], "-D") == 0 && i + 1 < argc) {
            if (ctx->cli_define_count < MAX_CLI_DEFINES) {
                parse_cli_define(argv[++i],
                                 ctx->cli_defines[ctx->cli_define_count],
                                 ctx->cli_define_values[ctx->cli_define_count],
                                 MAX_LABEL_LENGTH, MAX_LINE_LENGTH);
                ctx->cli_define_count++;
            } else {
                fprintf(stderr, "Warning: too many -D defines (max %d)\n",
                        MAX_CLI_DEFINES);
            }
        } else if (strncmp(argv[i], "-D", 2) == 0 && argv[i][2] != '\0') {
            if (ctx->cli_define_count < MAX_CLI_DEFINES) {
                parse_cli_define(argv[i] + 2,
                                 ctx->cli_defines[ctx->cli_define_count],
                                 ctx->cli_define_values[ctx->cli_define_count],
                                 MAX_LABEL_LENGTH, MAX_LINE_LENGTH);
                ctx->cli_define_count++;
            } else {
                fprintf(stderr, "Warning: too many -D defines (max %d)\n",
                        MAX_CLI_DEFINES);
            }
        }
    }

    /* Preprocess-only mode */
    if (ctx->preprocess_only) {
        char *preprocessed = asm_preprocess_file(ctx, input_file);
        if (!preprocessed) {
            emit_cli_error(ctx->error_format_json, "Assembler",
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

    printf("Assembling: %s\n", input_file);

    /* Assemble */
    if (asm_assemble_file(ctx, input_file) < 0) {
        emit_cli_error(ctx->error_format_json, "Assembler",
                       "Assembly failed",
                       "Check the diagnostic emitted above for the exact failure cause.");
        asm_free(ctx);
        return 1;
    }

    /* Treat warnings as errors if -Werror was set */
    if (ctx->fatal_warning_occurred) {
        emit_cli_error(ctx->error_format_json, "Assembler",
                       "Assembly failed due to warnings treated as errors",
                       "Fix the warnings above or remove -Werror.");
        asm_free(ctx);
        return 1;
    }

    /* Dependency generation mode */
    if (gen_deps || gen_deps_local) {
        printf("%s:", output_file);
        printf(" %s", input_file);
        for (int i = 0; i < ctx->dependency_count; i++) {
            printf(" %s", ctx->dependencies[i]);
        }
        printf("\n");
        asm_free(ctx);
        return 0;
    }

    printf("Assembly successful: %zu bytes generated\n", ctx->text_size);

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
        emit_cli_error(ctx->error_format_json, "CLI",
                       "Unknown output format",
                       "Use one of: elf64, bin, or hex.");
        result = -1;
    }

    if (result < 0) {
        emit_cli_error(ctx->error_format_json, "I/O",
                       "Failed to write output",
                       "Check output path, permissions, and available disk space.");
        asm_free(ctx);
        return 1;
    }

    if (ctx->emit_debug_map) {
        if (asm_write_debug_map(ctx, output_file) < 0) {
            emit_cli_error(ctx->error_format_json, "I/O",
                           "Failed to write debug map",
                           "Check output path permissions for <output>.dbg.");
            asm_free(ctx);
            return 1;
        }
    }

    if (ctx->emit_listing) {
        if (asm_write_listing(ctx, output_file) < 0) {
            emit_cli_error(ctx->error_format_json, "I/O",
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
