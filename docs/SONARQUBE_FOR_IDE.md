# SonarQube For IDE (VS Code)

This repository is configured for SonarQube for IDE (formerly SonarLint)
using the VS Code extension `sonarsource.sonarlint-vscode`.

## Workspace Setup

1. Install recommended workspace extensions.
2. Generate compile commands for C/C++ semantic analysis:

```bash
make compile-commands
```

This command requires `bear` and creates `compile_commands.json` in the
repository root.

If `bear` is unavailable in your environment, the helper automatically falls
back to a `make -n` transcript parser and still writes
`compile_commands.json` in the repository root.

## Connected Mode (Optional but Recommended)

To use server-side quality profiles and issues:

1. Open the `SONARQUBE SETUP` view in VS Code.
2. Add a SonarQube Server or SonarQube Cloud connection.
3. Provide URL/token (or cloud token/org) and save the connection.
4. Bind this workspace to the target project.

## Local Conformance Gate

Run a local static analyzer pass before pushing:

```bash
make static-analyze
```

This target compiles with GCC `-fanalyzer` and runs core test targets.
