# Line count of the compiler with lib

## With lib
```console
wc -l ./src/*.c, ./src/*.h, ./src/lib/*.c, ./src/lib/*.h | Sort-Object -Descending
```

## Without lib
```console
wc -l ./src/*.c, ./src/*.h | Sort-Object -Descending
```
