# x86_64 Assembler Makefile

CC = gcc
SAN_CC ?= $(shell command -v clang 2>/dev/null || echo $(CC))
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Wshadow -Wconversion \
	-Wsign-compare -Wmissing-prototypes -Wstrict-prototypes \
	-D_POSIX_C_SOURCE=200809L \
	-Iinclude \
	-fstack-protector-strong -O2 -g
TEST_CFLAGS = $(CFLAGS) -Werror
LDFLAGS =
DEPFLAGS = -MMD -MP
DEV_SAN_FLAGS = -fsanitize=address,undefined
SRCDIR = src
OBJDIR = obj
BINDIR = bin
TESTDIR = tests

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
DEPFILES = $(OBJECTS:.o=.d)
TARGET = $(BINDIR)/x86_64-asm

# Test files
TEST_SOURCES = $(wildcard $(TESTDIR)/test_*.c)
TEST_FRAMEWORK = $(TESTDIR)/test_framework.c

.PHONY: all clean test ci test-stress compare-nasm compare-nasm-gate compile-commands static-analyze test-encoder test-parser test-disassembler test-integration coverage dirs test-asan test-ubsan profile-parser profile-stress

all: dirs $(TARGET)

dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

-include $(DEPFILES)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	rm -f test_* *.bin *.hex
	@rm -f $(DEPFILES) 2>/dev/null || true
	@rm -f $(TESTDIR)/test_*.o $(TESTDIR)/*.o 2>/dev/null || true

# Unit tests
test-encoder: all $(TESTDIR)/test_encoder.c $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_encoder.o $(TESTDIR)/test_encoder.c
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_encoder $(TESTDIR)/test_encoder.o $(TESTDIR)/test_framework.o \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/test_encoder

test-parser: all $(TESTDIR)/test_parser.c $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_parser.o $(TESTDIR)/test_parser.c
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_parser $(TESTDIR)/test_parser.o $(TESTDIR)/test_framework.o \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/test_parser

test-disassembler: all $(TESTDIR)/test_disassembler.c $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_disassembler.o $(TESTDIR)/test_disassembler.c
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_disassembler $(TESTDIR)/test_disassembler.o $(TESTDIR)/test_framework.o \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/test_disassembler

test-unit:
	@set -e; \
	encoder_log=$$(mktemp); \
	parser_log=$$(mktemp); \
	trap 'rm -f "$$encoder_log" "$$parser_log"' EXIT; \
	$(MAKE) --no-print-directory test-encoder | tee "$$encoder_log"; \
	$(MAKE) --no-print-directory test-parser | tee "$$parser_log"; \
	enc_total=$$(grep -E "^Total:" "$$encoder_log" | tail -1 | awk '{print $$2}'); \
	enc_passed=$$(grep -E "^Passed:" "$$encoder_log" | tail -1 | awk '{print $$2}'); \
	enc_failed=$$(grep -E "^Failed:" "$$encoder_log" | tail -1 | awk '{print $$2}'); \
	par_total=$$(grep -E "^Total:" "$$parser_log" | tail -1 | awk '{print $$2}'); \
	par_passed=$$(grep -E "^Passed:" "$$parser_log" | tail -1 | awk '{print $$2}'); \
	par_failed=$$(grep -E "^Failed:" "$$parser_log" | tail -1 | awk '{print $$2}'); \
	all_total=$$((enc_total + par_total)); \
	all_passed=$$((enc_passed + par_passed)); \
	all_failed=$$((enc_failed + par_failed)); \
	echo ""; \
	echo "===================================="; \
	echo "      Combined Unit Summary"; \
	echo "===================================="; \
	echo "Total:  $$all_total"; \
	echo "Passed: $$all_passed"; \
	echo "Failed: $$all_failed"; \
	echo "===================================="

test-integration: all $(TESTDIR)/test_integration.c $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(TEST_CFLAGS) -c -o $(TESTDIR)/test_integration.o $(TESTDIR)/test_integration.c
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_integration $(TESTDIR)/test_integration.o $(TESTDIR)/test_framework.o \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/test_integration

coverage: clean
	@mkdir -p coverage
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LDFLAGS="--coverage" test-encoder
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LDFLAGS="--coverage" test-parser
	$(MAKE) CFLAGS="$(CFLAGS) --coverage" LDFLAGS="--coverage" test-integration
	@gcov -b -c -o $(OBJDIR) $(SRCDIR)/*.c | tee coverage/gcov.txt
	@awk -F'[:%]' '/^Lines executed:/ { sum += $$2; n++ } END { if (n > 0) printf("Average line coverage: %.2f%% across %d entries\n", sum / n, n); else print "Average line coverage: no data" }' coverage/gcov.txt
	@awk -F'[:%]' '/^Branches executed:/ { sum += $$2; n++ } END { if (n > 0) printf("Average branch coverage: %.2f%% across %d entries\n", sum / n, n); else print "Average branch coverage: no data" }' coverage/gcov.txt

test: compare-nasm-gate all
	@echo "Running integration tests..."
	@for f in examples/*.asm; do \
		case "$$f" in examples/stress_*.asm) continue ;; esac; \
		out="test_out_$$(basename "$$f" .asm)"; \
		echo "Testing $$f..."; \
		rm -f "$$out"; \
		$(TARGET) "$$f" -o "$$out" && chmod +x "$$out" && ./"$$out"; \
		echo "Exit: $$?"; \
		rm -f "$$out"; \
		echo; \
	done

ci: compare-nasm-gate test-unit test-disassembler test-integration test-stress

test-stress: all
	@echo "Running stress fixture assembly checks..."
	@set -e; \
	for f in examples/stress_*.asm; do \
		if [ ! -f "$$f" ]; then continue; fi; \
		out=$$(mktemp /tmp/iariaboot_stress_XXXXXX); \
		echo "Assembling $$f..."; \
		$(TARGET) "$$f" -o "$$out"; \
		size=$$(stat -c%s "$$out"); \
		if [ "$$size" -le 0 ]; then \
			echo "Error: output artifact for $$f is empty"; \
			rm -f "$$out"; \
			exit 1; \
		fi; \
		rm -f "$$out"; \
	done; \
	echo "Stress fixtures assembled successfully."

# Individual test targets
test-exit: all
	$(TARGET) examples/exit.asm -o test_out && chmod +x test_out && ./test_out

test-hello: all
	$(TARGET) examples/hello.asm -o test_out && chmod +x test_out && ./test_out

test-loop: all
	$(TARGET) examples/loop.asm -o test_out && chmod +x test_out && ./test_out

test-arith: all
	$(TARGET) examples/arith.asm -o test_out && chmod +x test_out && ./test_out

test-all: all
	$(TARGET) examples/test_all.asm -o test_out && chmod +x test_out && ./test_out

test-asan: clean
	@printf 'int main(void){return 0;}\n' | $(SAN_CC) -x c - -o /tmp/iariaboot_san_check $(DEV_SAN_FLAGS) >/dev/null 2>&1 || \
		(echo "Error: Sanitizer runtime not available for $(SAN_CC)."; \
		 echo "Hint: install ASan/UBSan runtime packages, or run with CC=clang if available."; \
		 rm -f /tmp/iariaboot_san_check; exit 1)
	@rm -f /tmp/iariaboot_san_check
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) CC="$(SAN_CC)" CFLAGS="$(CFLAGS) $(DEV_SAN_FLAGS)" LDFLAGS="$(LDFLAGS) $(DEV_SAN_FLAGS)" test-unit
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) CC="$(SAN_CC)" CFLAGS="$(CFLAGS) $(DEV_SAN_FLAGS)" LDFLAGS="$(LDFLAGS) $(DEV_SAN_FLAGS)" test-integration

test-ubsan: clean
	@printf 'int main(void){return 0;}\n' | $(SAN_CC) -x c - -o /tmp/iariaboot_san_check -fsanitize=undefined >/dev/null 2>&1 || \
		(echo "Error: UBSan runtime not available for $(SAN_CC)."; \
		 echo "Hint: install UBSan runtime packages, or run with CC=clang if available."; \
		 rm -f /tmp/iariaboot_san_check; exit 1)
	@rm -f /tmp/iariaboot_san_check
	$(MAKE) CC="$(SAN_CC)" CFLAGS="$(CFLAGS) -fsanitize=undefined" LDFLAGS="$(LDFLAGS) -fsanitize=undefined" test-unit
	$(MAKE) CC="$(SAN_CC)" CFLAGS="$(CFLAGS) -fsanitize=undefined" LDFLAGS="$(LDFLAGS) -fsanitize=undefined" test-integration

profile-parser: all tools/profile_parser.c
	$(CC) $(TEST_CFLAGS) $(LDFLAGS) -o $(BINDIR)/profile_parser tools/profile_parser.c \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/profile_parser

profile-stress: all tools/profile_stress.sh
	./tools/profile_stress.sh 3 docs/STRESS_METRICS.md

compare-nasm: all tools/compare_with_nasm.sh
	./tools/compare_with_nasm.sh docs/NASM_COMPARISON_REPORT.md

compare-nasm-gate: compare-nasm
	@if grep -Eq '^Summary: [0-9]+/[0-9]+ fixtures byte-identical; 0 mismatches total \(0 tool errors\)\.$$' docs/NASM_COMPARISON_REPORT.md; then \
		echo "NASM parity gate passed (0 mismatches)"; \
	else \
		echo "error: NASM parity gate failed (non-zero mismatches or tool errors)"; \
		exit 1; \
	fi

compile-commands: all tools/generate_compile_commands.sh
	./tools/generate_compile_commands.sh

static-analyze: clean
	$(MAKE) CFLAGS="$(CFLAGS) -fanalyzer -Wno-analyzer-fd-leak" TEST_CFLAGS="$(CFLAGS) -fanalyzer -Wno-analyzer-fd-leak -Werror" all
