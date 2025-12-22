param (
    [switch]$WithLib
)

$files = @(
    "./src/*.c"
    "./src/*.h"
)

if ($WithLib) {
    $files += @(
        "./src/lib/*.c"
        "./src/lib/*.h"
    )
}

wc -l $files | Sort-Object -Descending

# wc -l ./src/*.c, ./src/*.h, ./src/lib/*.c, ./src/lib/*.h | Sort-Object -Descending