# x86_64 Assembler Makefile

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
LDFLAGS =
DEPFLAGS = -MMD -MP
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

.PHONY: all clean test test-encoder test-parser test-integration coverage dirs

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
	$(CC) $(CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(CFLAGS) -c -o $(TESTDIR)/test_encoder.o $(TESTDIR)/test_encoder.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_encoder $(TESTDIR)/test_encoder.o $(TESTDIR)/test_framework.o \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/test_encoder

test-parser: all $(TESTDIR)/test_parser.c $(TEST_FRAMEWORK)
	$(CC) $(CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(CFLAGS) -c -o $(TESTDIR)/test_parser.o $(TESTDIR)/test_parser.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_parser $(TESTDIR)/test_parser.o $(TESTDIR)/test_framework.o \
		$(filter-out $(OBJDIR)/asm.o,$(OBJECTS))
	./$(BINDIR)/test_parser

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
	$(CC) $(CFLAGS) -c -o $(TESTDIR)/test_framework.o $(TEST_FRAMEWORK)
	$(CC) $(CFLAGS) -c -o $(TESTDIR)/test_integration.o $(TESTDIR)/test_integration.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(BINDIR)/test_integration $(TESTDIR)/test_integration.o $(TESTDIR)/test_framework.o \
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

test: all
	@echo "Running integration tests..."
	@for f in examples/*.asm; do \
		echo "Testing $$f..."; \
		$(TARGET) "$$f" -o test_out && chmod +x test_out && ./test_out; \
		echo "Exit: $$?"; \
		echo; \
	done

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
