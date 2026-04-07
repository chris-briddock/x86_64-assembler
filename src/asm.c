/**
 * x86_64 Assembler - Command-line driver
 */

#include "x86_64_asm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *prog) {
    fprintf(stderr, "Usage: %s [options] <input.asm>\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -o <file>    Output file (default: a.out)\n");
    fprintf(stderr, "  -f <fmt>     Output format: elf64, bin, hex (default: elf64)\n");
    fprintf(stderr, "  -g           Emit lightweight debug map (<output>.dbg)\n");
    fprintf(stderr, "  -l           Emit listing file (<output>.lst)\n");
    fprintf(stderr, "  -d           Dump symbols and code\n");
    fprintf(stderr, "  -D <file>    Disassemble ELF64 input file\n");
    fprintf(stderr, "  --disassemble <file>  Disassemble ELF64 input file\n");
    fprintf(stderr, "  -h           Show this help\n");
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = "a.out";
    const char *format = "elf64";
    const char *disassemble_file = NULL;
    int dump = 0;
    int debug_map = 0;
    int listing = 0;

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
        } else if ((strcmp(argv[i], "-D") == 0 || strcmp(argv[i], "--disassemble") == 0) && i + 1 < argc) {
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
        fprintf(stderr,
                "Error at line 1, column 1: [CLI] No input file specified\n"
                "Suggestion: Provide an input .asm file path.\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Initialize assembler */
    assembler_context_t *ctx = asm_init();
    if (!ctx) {
        fprintf(stderr,
                "Error at line 1, column 1: [Memory] Failed to initialize assembler\n"
                "Suggestion: Retry and verify memory availability.\n");
        return 1;
    }
    ctx->emit_debug_map = (debug_map != 0);
    ctx->emit_listing = (listing != 0);

    printf("Assembling: %s\n", input_file);

    /* Assemble */
    if (asm_assemble_file(ctx, input_file) < 0) {
        fprintf(stderr,
                "Error at line 1, column 1: [Assembler] Assembly failed\n"
                "Suggestion: Check the diagnostic emitted above for the exact failure cause.\n");
        asm_free(ctx);
        return 1;
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
        fprintf(stderr,
                "Error at line 1, column 1: [CLI] Unknown format '%s'\n"
                "Suggestion: Use one of: elf64, bin, or hex.\n",
                format);
        result = -1;
    }

    if (result < 0) {
        fprintf(stderr,
                "Error at line 1, column 1: [I/O] Failed to write output\n"
                "Suggestion: Check output path, permissions, and available disk space.\n");
        asm_free(ctx);
        return 1;
    }

    if (ctx->emit_debug_map) {
        if (asm_write_debug_map(ctx, output_file) < 0) {
            fprintf(stderr,
                    "Error at line 1, column 1: [I/O] Failed to write debug map\n"
                    "Suggestion: Check output path permissions for <output>.dbg.\n");
            asm_free(ctx);
            return 1;
        }
    }

    if (ctx->emit_listing) {
        if (asm_write_listing(ctx, output_file) < 0) {
            fprintf(stderr,
                    "Error at line 1, column 1: [I/O] Failed to write listing file\n"
                    "Suggestion: Check output path permissions for <output>.lst.\n");
            asm_free(ctx);
            return 1;
        }
        printf("Listing written to: %s.lst\n", output_file);
    }

    printf("Output written to: %s\n", output_file);

    asm_free(ctx);
    return 0;
}
