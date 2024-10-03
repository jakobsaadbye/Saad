; ModuleID = 'llvm.c'
source_filename = "llvm.c"
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

@.str = constant [14 x i8] c"Hello, World!\00"

define i32 @main() #0 {
  %1 = call i32 @printf(ptr @.str)
  ret i32 0
}

declare i32 @printf(ptr, ...)