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
    fprintf(stderr, "  -d           Dump symbols and code\n");
    fprintf(stderr, "  -h           Show this help\n");
}

int main(int argc, char **argv) {
    const char *input_file = NULL;
    const char *output_file = "a.out";
    const char *format = "elf64";
    int dump = 0;
    int debug_map = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            format = argv[++i];
        } else if (strcmp(argv[i], "-g") == 0) {
            debug_map = 1;
        } else if (strcmp(argv[i], "-d") == 0) {
            dump = 1;
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

    if (!input_file) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Initialize assembler */
    assembler_context_t *ctx = asm_init();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize assembler\n");
        return 1;
    }
    ctx->emit_debug_map = (debug_map != 0);

    printf("Assembling: %s\n", input_file);

    /* Assemble */
    if (asm_assemble_file(ctx, input_file) < 0) {
        fprintf(stderr, "Error: Assembly failed\n");
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
        fprintf(stderr, "Error: Unknown format '%s'\n", format);
        result = -1;
    }

    if (result < 0) {
        fprintf(stderr, "Error: Failed to write output\n");
        asm_free(ctx);
        return 1;
    }

    if (ctx->emit_debug_map) {
        if (asm_write_debug_map(ctx, output_file) < 0) {
            fprintf(stderr, "Error: Failed to write debug map\n");
            asm_free(ctx);
            return 1;
        }
    }

    printf("Output written to: %s\n", output_file);

    asm_free(ctx);
    return 0;
}
