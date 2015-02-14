ad-C
====

A tool to do metaprogramming with C99.

`adc/example/` shows how it is used.

_Warning_ `#include <adc.h>` in Windows, without properly specifying the Include directory for adc, will by default load a Windows SDK header!

Features
--------
- Simple template (text substitution) engine.
- C99 type parser. It can be extended for some light C++.

To-Do
-----
- Reflection engine, using the parser
