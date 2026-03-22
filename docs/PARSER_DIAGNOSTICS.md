# Parser Diagnostic Improvements

## Overview

The x86-64 assembler parser has been enhanced with improved error diagnostics that provide contextual information when parsing errors occur. This makes it much easier for users to identify and fix syntax errors in their assembly code.

## Diagnostic Features

### 1. Source Context Display

When a parser error occurs, the diagnostic system now displays:
- **Error message** with clear description of the problem
- **Source line** showing the exact code that caused the error
- **Position indicator** (^) pointing to where the error was detected

### 2. Error Examples

#### Missing Closing Bracket
```
Error at line 4: Unexpected token in memory operand
  mov [rbx + rcx, rax
                  ^
```

#### Missing Immediate Value
```
Error at line 5: Expected immediate value after '$'
  
  ^
```

#### Unknown Instruction
```
Error at line 3: Unknown instruction
  invalid_instr rax, rbx
                ^
```

## Diagnostic Function

### `parser_error_context(parser_state_t *p, const char *message)`

Located in `src/x86_64_parser.c`, this function provides contextual error reporting.

**Parameters:**
- `p`: Parser state containing current position and input
- `message`: Human-readable error message

**Functionality:**
1. Calculates the current line and column position
2. Prints error message with line number
3. Displays the problematic source line
4. Shows a visual indicator (^) at the error position

### Implementation Details

```c
static void parser_error_context(parser_state_t *p, const char *message) {
    /* Find end of current line */
    const char *line_end = p->pos;
    while (*line_end && *line_end != '\n') line_end++;
    
    /* Calculate line length and column */
    int line_len = (int)(line_end - p->line_start);
    int col = (int)(p->pos - p->line_start);
    
    /* Print error message */
    fprintf(stderr, "Error at line %d: %s\n", p->line, message);
    
    /* Print the source line */
    fprintf(stderr, "  ");
    for (int i = 0; i < line_len; i++) {
        fputc(p->line_start[i], stderr);
    }
    fprintf(stderr, "\n");
    
    /* Print column indicator */
    fprintf(stderr, "  ");
    for (int i = 0; i < col; i++) fputc(' ', stderr);
    fprintf(stderr, "^\n");
}
```

## Improved Error Messages

The following parser errors now provide contextual diagnostics:

### Memory Operand Errors
- "Expected '[' for memory operand"
- "Expected scale factor (1, 2, 4, or 8)" (multiple locations)
- "Too many registers in memory operand (max: base + index)"
- "Expected ']' to close memory operand"
- "Expected register or number after +/- in memory operand"
- "Unexpected token in memory operand"
- "Scale factor requires an index register"
- "Negative displacement with label reference not supported"

### Operand Errors
- "Expected immediate value after '$'"
- "Invalid operand syntax"

### Instruction Errors
- "Unknown instruction"
- "Too many operands"

## Usage Examples

### Before (Without Diagnostics)
```
Error: Expected immediate value at line 5
Error: Too many registers in memory operand at line 14
```

### After (With Diagnostics)
```
Error at line 5: Expected immediate value after '$'
  mov rax, $
             ^

Error at line 14: Too many registers in memory operand (max: base + index)
  movq [rbx + rcx + rdx], %rax
         ^
```

## Benefits

1. **Faster Debugging**: Users can immediately see the exact location of the error
2. **Clearer Messages**: Error messages explain what was expected
3. **Better Learning**: New users can understand assembly syntax rules more easily
4. **Reduced Iterations**: No need to count lines to find errors manually

## Future Enhancements

Potential improvements for future versions:

1. **SSE Instruction Validation**: Extend diagnostics to SSE-specific validation errors
2. **Error Suggestions**: Provide hints like "did you mean..." for typos
3. **Multiple Error Reporting**: Collect and report multiple errors in a single pass
4. **Syntax Highlighting**: Add ANSI color codes for better visual distinction
5. **Line Numbers in Output**: Show line numbers on the left side of source display

## Testing

The diagnostic system has been tested with various error scenarios:
- Missing brackets in memory operands
- Invalid scale factors
- Too many registers in addressing modes
- Missing immediate values
- Unknown instructions

No regressions were observed in parser-focused and example-based validation when introducing the diagnostic formatter.
