#include "../src/x86_64_asm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t now_ns(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static char *build_large_program(size_t blocks) {
    const char *header =
        "section .text\n"
        "global _start\n"
        "_start:\n";
    const size_t per_block = 128u;
    size_t cap = strlen(header) + (blocks * per_block) + 1u;
    char *buf = (char *)malloc(cap);
    size_t pos = 0;

    if (!buf) {
        return NULL;
    }

    pos += (size_t)snprintf(buf + pos, cap - pos, "%s", header);
    for (size_t i = 0; i < blocks; i++) {
        pos += (size_t)snprintf(
            buf + pos,
            cap - pos,
            "L%zu: mov rax, $1\n"
            "     lea rbx, [rax+rcx*4+8]\n"
            "     add rax, rbx\n"
            "     cmp rax, $3\n",
            i
        );
        if (i + 1u < blocks) {
            pos += (size_t)snprintf(buf + pos, cap - pos, "     jne L%zu\n", i + 1u);
        }
    }
    pos += (size_t)snprintf(buf + pos, cap - pos, "     mov rax, $60\n     xor rdi, rdi\n     syscall\n");

    return buf;
}

static uint64_t linear_lookup(const assembler_context_t *ctx, const char *name) {
    for (int i = 0; i < ctx->symbol_count; i++) {
        if (strcmp(ctx->symbols[i].name, name) == 0) {
            return ctx->symbols[i].address;
        }
    }
    return 0;
}

int main(void) {
    const size_t blocks = 3000u;
    const int parser_runs = 5;
    char *program = build_large_program(blocks);
    uint64_t parser_total_ns = 0;
    parser_profile_stats_t stats;
    assembler_context_t *profile_ctx = asm_init();

    if (!program) {
        fprintf(stderr, "failed to allocate parser profile program\n");
        return 1;
    }

    if (!profile_ctx) {
        fprintf(stderr, "failed to allocate parser profile context\n");
        free(program);
        return 1;
    }

    parser_profile_reset(profile_ctx);
    parser_profile_enable(profile_ctx, true);

    for (int i = 0; i < parser_runs; i++) {
        int count = 0;
        uint64_t start_ns = now_ns();
        parsed_instruction_t *insts = parse_source_with_context(profile_ctx, program, &count);
        uint64_t end_ns = now_ns();

        if (!insts) {
            fprintf(stderr, "parse_source failed on run %d\n", i + 1);
            asm_free(profile_ctx);
            free(program);
            return 1;
        }

        parser_total_ns += (end_ns - start_ns);
        free_instructions(insts);
    }

    parser_profile_enable(profile_ctx, false);
    parser_profile_get(profile_ctx, &stats);

        printf("Parser profiling on synthetic source\n");
        printf("- Blocks: %zu (about %zu lines)\n", blocks, blocks * 5u + 6u);
        printf("- Runs: %d\n", parser_runs);
        printf("- Average parse time: %.3f ms\n", (double)parser_total_ns / (double)parser_runs / 1000000.0);
        printf("- parse_source_internal time: %.3f ms across %llu runs\n",
           (double)stats.parse_ns / 1000000.0,
           (unsigned long long)stats.parse_calls);
        printf("- parse_instruction time: %.3f ms across %llu calls\n",
           (double)stats.parse_instruction_ns / 1000000.0,
           (unsigned long long)stats.parse_instruction_calls);
        printf("- next_token time: %.3f ms across %llu calls\n",
           (double)stats.next_token_ns / 1000000.0,
           (unsigned long long)stats.next_token_calls);
        printf("- Instruction buffer realloc events: %llu\n",
           (unsigned long long)stats.realloc_events);
        printf("- Peak instruction buffer capacity: %llu\n",
           (unsigned long long)stats.peak_instruction_capacity);

    {
        assembler_context_t *ctx = asm_init();
        const int symbols = 3500;
        const int lookups = 400000;
        uint64_t linear_sum = 0;
        uint64_t hash_sum = 0;
        uint64_t linear_ns;
        uint64_t hash_ns;

        if (!ctx) {
            fprintf(stderr, "failed to allocate assembler context\n");
            free(program);
            return 1;
        }

        for (int i = 0; i < symbols; i++) {
            char name[MAX_LABEL_LENGTH];
            symbol_t *sym = &ctx->symbols[ctx->symbol_count++];

            (void)snprintf(name, sizeof(name), "SYM_%04d", i);
            memset(sym, 0, sizeof(*sym));
            (void)snprintf(sym->name, sizeof(sym->name), "%s", name);
            sym->address = (uint64_t)(0x400000 + i * 16);
            sym->is_resolved = true;
            sym->section = 0;

            if (symbol_hash_insert(ctx, name, sym->address, false, false, 0) < 0) {
                fprintf(stderr, "failed to insert symbol '%s' into hash table\n", name);
                asm_free(ctx);
                free(program);
                return 1;
            }
        }

        {
            uint64_t start = now_ns();
            for (int i = 0; i < lookups; i++) {
                char name[MAX_LABEL_LENGTH];
                int idx = (i * 17) % symbols;
                (void)snprintf(name, sizeof(name), "SYM_%04d", idx);
                linear_sum += linear_lookup(ctx, name);
            }
            linear_ns = now_ns() - start;
        }

        {
            uint64_t start = now_ns();
            for (int i = 0; i < lookups; i++) {
                char name[MAX_LABEL_LENGTH];
                int idx = (i * 17) % symbols;
                hash_entry_t *entry;
                (void)snprintf(name, sizeof(name), "SYM_%04d", idx);
                entry = symbol_hash_lookup(ctx, name);
                hash_sum += entry ? entry->address : 0;
            }
            hash_ns = now_ns() - start;
        }

        printf("\nSymbol lookup benchmark\n");
        printf("- Symbol count: %d\n", symbols);
        printf("- Lookup iterations: %d\n", lookups);
        printf("- Linear lookup: %.3f ms\n", (double)linear_ns / 1000000.0);
        printf("- Hash lookup: %.3f ms\n", (double)hash_ns / 1000000.0);
        if (hash_ns > 0) {
            printf("- Speedup (linear/hash): %.2fx\n", (double)linear_ns / (double)hash_ns);
        }
        printf("- Checksum guard: linear=%llu hash=%llu\n",
               (unsigned long long)linear_sum,
               (unsigned long long)hash_sum);

        asm_free(ctx);
    }

    asm_free(profile_ctx);
    free(program);
    return 0;
}
