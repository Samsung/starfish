# Debug Utilities

## Trace macros

This module provides macros defined in [`Trace.h`](../../src/core/util/debug/Trace.h). All macros can be toggled with
the `TRACE=value` environment variable. Types inheriting from std::stream are
supported. Each macro checks the current thread and adds a human-readable thread
ID prefix (e.g., `[1]`).

`TRACE` is enabled in debug builds. Defining `ENABLE_TRACE` in a build will also enable it in release builds.

```shell
CXXFLAGS="-DENABLE_TRACE=1" cmake CMakeLists.txt ...
```

### `TRACE(identifier[,data][,...args])`

- identifier {string} An unique identifier assigned to this log. It doesn't require wrapped with double quotes, `"`.
- data {arithmetic types|stream buffers|manipulators} C++ string that contains the text.
- ...args {arithmetic types|stream buffers|manipulators} Additional arguments depending on the previous data.

```cpp
// Test.cpp
class Test {
  ...

  void Func(Nullable<String*> url) {
    TRACE(ID1, "logs for ID1");
    TRACE(ID2, "logs for ID2");

    // printf-like formmatting is supported.
    TRACE(ID3, "logs for ID %d", 3);

    // Actually, the above format specifier, %d, isn't needed.
    // TRACE(ID3, "logs for ID ", 3);

    // `<<` operator with `std::string` is supported.
    TRACE(ID4) << url->value()->toUTF8NonGCString();
  }
};
```

Usage:

```shell
$ export TRACE=ID1,ID3
$ ./Starfish

[1] TRACE (ID1     )   Test::Func (Test.cpp:10) logs for ID1
[1] TRACE (ID3     )   Test::Func (Test.cpp:12) logs for ID2


$ export TRACE=*,-ID1
$ ./Starfish

[1] TRACE (ID2     )   Test::Func (Test.cpp:XX) logs for ID2
[1] TRACE (ID3     )   Test::Func (Test.cpp:XX) logs for ID3
[1] TRACE (ID4     )   Test::Func (Test.cpp:XX) url


$ export TRACE=
$ ./Starfish

// print nothing
```

### `TRACE_SCOPE(identifier[,data][,...args])`

- identifier {string} An unique identifier assigned to this log. It doesn't require wrapped with double quotes, `"`.
- data {arithmetic types|stream buffers|manipulators} C++ string that contains the text.
- ...args {arithmetic types|stream buffers|manipulators} Additional arguments depending on the previous data.

A stack scope-based logger mainly for printing function call graphs.

```cpp
// Test.cpp
class Test {
  ...

  void Func1() {
    TRACE_SCOPE(ID1);
    Func2();
    Func3();
  }

  void Func2() {
    TRACE_SCOPE(ID1);
    Func3();
  }

  void Func3() {
    TRACE_SCOPE(ID1);
  }
};
```

Usage:

```shell
$ export TRACE=ID1
$ ./Starfish

[1] TRACE (ID1     )   Test::Func1 (Test.cpp:XX)
[1] TRACE (ID1     )     Test::Func2 (Test.cpp:XX)
[1] TRACE (ID1     )      Test::Func3 (Test.cpp:XX)
[1] TRACE (ID1     )     Test::Func3 (Test.cpp:XX)
```

### `TRACE0(identifier[,data][,...args])`

- identifier {string} An unique identifier assigned to this log. It doesn't require wrapped with double quotes, `"`.
- data {arithmetic types|stream buffers|manipulators} C++ string that contains the text.
- ...args {arithmetic types|stream buffers|manipulators} Additional arguments depending on the previous data.

This behaves similar to `TRACE` but doesn't print the code location.

```cpp
// Test.cpp
class Test {
  ...

  void Func(Nullable<String*> url) {
    TRACE(ID1, "messages from TRACE with ID1");
    TRACE0(ID1, "messages from TRACE0 with ID1");
  }
};
```

Usage:

```shell
$ export TRACE=*
$ ./Starfish

[1] TRACE (ID1     )   Test::Func (Test.cpp:XX) messages from TRACE with ID1
[1] TRACE (ID1     )   messages from TRACE0 with ID1
```
