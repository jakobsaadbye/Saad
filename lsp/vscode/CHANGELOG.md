# Changelog

The changes of the language and the language server are in the
[changelog of Saad](https://github.com/jakobsaadbye/Saad#changelog).

## 0.4.1

* Debugging on Linux links the program with tools/linux/link.sh, so programs that use libpq or OpenSSL can be debugged

## 0.4.0

* Debugging of Saad programs with CodeLLDB: F5 in a `.sd` file, breakpoints, stepping, the calls and the variables,
  with strings, lists, optionals, unions and `any` shown like Saad writes them. The configuration type is `saad`

## 0.3.1

* Highlighting of `#if` and of `OS` and `ARCH` in its condition

## 0.3.0

* The first version on the Marketplace and Open VSX. Highlighting of `try`, `catch`, `throw` and `defer`
