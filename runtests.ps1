$ErrorActionPreference = "Stop"

# Compiler flags (array!)
$CFLAGS = @(
    "-Wall"
    "-Wextra"
    "-pedantic"
    "-ggdb"
)

# Source file base names
$FILES = @(
    "ast"
    "lexer"
    "parser"
    "scope"
)

# Ensure build directory exists
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Compile each source file
foreach ($f in $FILES) {
    gcc -c "src/$f.c" -o "build/$f.o"
}

# Link all object files into one executable
$objects = $FILES | ForEach-Object { "build/$_.o" }

gcc @CFLAGS `
    -o "build/run_tests.exe" `
    @objects `
    "tests/_test_runner.c"

# Run
& "build/run_tests.exe"
