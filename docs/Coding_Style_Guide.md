# C++ Style Guide
The goal of this document is to describe our C++ style convention. Our
coding style generally follows
[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html).
This document highlights core coding style.

## Header Files
### `#define` Guard

Use `#define` directive in all header files to prevent multiple
inclusion of header files. The format of the identifier name should be
`__[Project Name][HeaderFileName]__`. For example, for
`MyVeryCoolClass.h` in `Starfish` project, the guard should be
`__StarfishMyVeryCoolClass__` .


```cpp
#ifndef __StarfishMyVeryCoolClass__
#define __StarfishMyVeryCoolClass__
...
#endif
```

### Use `#include` only in `.cpp` files
To prevent possible loops in header file inclusion, try to your best to include header files only
in .cpp files. In this case, the order of header file inclusion is important.

'including headers in a header' is allowed for the followings. otherwise, try to use forward declarations.
* Class Inheritance
* Class member as an instance

## Formatting
### Indentation
Use spaces only, and use 4 spaces at a time. Tabs should not be used.

### Line Length
Each line of text should be at most 80 characters long.

### Empty Lines between Functions
Add one empty line between function implementations in `.cpp` files.
Add zero lines between function declarations (or implementations) in `.h` files
to explicitly group related functions. Add one empty line to separate between
these groups.

### `if` Statements
Add a space before the opening parenthesis, and between closing parenthesis and
opening curly brace. Always enclose statements with a pair of braces even for
a single line statement.

```cpp
if (condition) {
    ...
}

if (condition) {
    a = b;
}
```

Write the condition of `if` scope with explicit expressions. For details, please refer to [Code readability](#Code-readability)

### `for`, `while`, `do-while` Statements
Add a space before the opening parenthesis, and between closing parenthesis and
opening curly brace. Always have statements with a pair of braces even for a
single line statement.

```cpp
while (condition) {
    ...
}

while (condition) {
    a = b;
}
```

Write the condition of `for`, `while`, and `do-while` scopes with explicit expressions. For details, please refer to [Code readability](#Code-readability)

### Binary Operators
When binary operators cannot fit in the same line, split operands after the
binary operators, and align operands with the first operand. Try to use
parentheses as much as possible even if the correct precedence can be derived
without using parentheses. This often helps readers to understand the code
better.

```cpp
if (condition1 ||
   (condition2 && condition3)) {
    ...
}
```

Write the conditions among binary operators with explicit expressions. For details, please refer to [Code readability](#Code-readability)

## Classes
### Constructors
Initialization of the member variables should be done in the initializer as much as possible.
When initializing member variables in a constructor, always split each
initializer on a separate line, and align the commas with the colon.

```cpp
Dog::Dog(String name, Breed breed)
    : Animal()
    , m_name(name)
    , m_breed(breed)
{
...
}
```

Calling other constructors inside a constructor should be avoided.
This is in preparation for an environment where we cannot use c++11 features (such as embedded devices).
```cpp
Dog::Dog()
    , m_isAnimal(true)
{
}

Dog::Dog(String name, Breed breed)
    : Dog()  //<--- Not allowed
    , m_name(name)
    , m_breed(breed)
{
...
}

```

Write it one by one.
```cpp
Dog::Dog()
    , m_isAnimal(true)
    , m_name(String::emptyString)
    , m_breed(Breed::emptyBreed)
{
}

Dog::Dog(String name, Breed breed)
    : m_isAnimal(true)
    , m_name(name)
    , m_breed(breed)
{
...
}

```


### Class Modifiers
Do not indent class modifiers (i.e., `public`, `protected`, and `private`) in
a class declaration.

```cpp
class Dog {
public:
    ...
private:
    ...
}
```

## Naming

- Types (classes, structs, enums) use PascalCase. A class lives in a file
  pair named after it (`Element.h` / `Element.cpp`).
- Functions and local variables use camelCase.
- Data members use the `m_` prefix (`m_firstChild`).
- Boolean predicates read as questions — `isXxx`, `hasXxx`, `shouldXxx`,
  `didXxx`, `inXxx` (see Code readability below for how they are checked).
- The public embedder API namespace `LWE` also contains a class named `LWE`
  (`inc/LWEWebView.h`), so an unqualified `LWE::` is ambiguous wherever that
  class is in scope (e.g. under `using namespace LWE;`). Reference the
  namespace with the global qualifier — `::LWE::KeyValue` — as the bridge
  implementations do.

## Functions
### Function Calls
Write a function call all in the same line if it fits. If not, split the
function call into multiple lines by either adding a newline after the
assignment operator or between function parameters. When splitting between
parameters, align them with the first parameter. In addition, do not add spaces
before and after the first and last function parameter, respectively.

```cpp
int val = foo(arg1, arg2, arg3);

int val =
    foo(arg1, arg2, arg3);

int val = foo(arg1, arg2
              arg3);
```

### Function Declaration and Definition
Use named parameters in function declarations.

```cpp
returnType functionName(int, int); // Not allowed
returnType functionName(int arg1, int arg2);  // Use named parameters
```

Return type should be on the same line as the function declaration
(and definition) if it fits. If not, split function parameters into multiple
lines, and align them with the first parameter.

```cpp
returnType functionName(int arg1,
                        int arg2);
```

If the first parameter does not fit in a line, write parameters in the next
line with 4 space indent.

```cpp
returnType functionName(
    int arg1, int arg2);
```

The opening curly brace should be on its own in the next line.
A Closing curly brace should be on the next line as the opening brace.

```cpp
returnType functionName()
{
    ...
}
```

### Short and Empty Functions
Do not write a function all in one line even for a very short function that
can be fit in one line. One exception is an empty, inline function that is
declared in a header file.

```cpp
void f() { } // Allowed only in a header file

int g()
{
    return 0;
}
```

## C++ Features
### Run-Time Type Information (RTTI)
Do not use Run Time Type Information.

### C++11
Use of C++11 features are encouraged. In addition, use of C++11 compatible
style formatting is encouraged, e.g., use `A<B<int>>` instead of `A<B<int> >`

### Scoped enums
Prefer `enum class` over a plain `enum` when declaring a new enum type — it
doesn't leak enumerators into the enclosing scope and doesn't implicitly
convert to int. Plain enums remain in older code; don't mass-convert them.

### `override`
Mark every virtual function that overrides a base-class function with
`override`, so a signature mismatch fails to compile instead of silently
declaring a new virtual function.

### Use of `try-catch` statements
Do not use try-catch statements except throwing a DOMException.
## Assertions and nullptr
### Basic principle
* This codebase is GC-based and passes objects around as raw pointers by default: a pointer parameter or member is expected to be valid unless its type says otherwise. Express "can be absent" in the type with `Optional<T>` — not with asserts or defensive null checks.
* In our strategy, Starfish will be terminated along with an error message when a memory allocation attempt fails.
* If you use c-style allocator like malloc/free, you should check allocation fail.
* Don't use native(not GC) operator new [] like new char[1240000]
* Don't hold GC-managed pointers in a non-GC container such as `std::vector`, even as a short-lived local. The container's backing buffer is allocated outside the GC heap, so the collector does not scan it and a still-referenced element can be collected. Use `GCVector`/`GCTightVector` (see `StarfishBase.h`) instead.
* While you don't use GC allocator, check before dereferencing with ASSERT or if, depends on the expected behavior what you want to achieve.

### Assertions on pointer arguments are legacy
Blanket-asserting every pointer argument used to be the rule here:
```cpp
void A::functionA(B* arg1, int arg2) {
    STARFISH_ASSERT(arg1 != nullptr); // legacy pattern -- don't add new ones
}
```
It is outdated: validity is the default expectation for a pointer in this
codebase, and nullability belongs in the type (`Optional<T>`). Don't add new
blanket asserts; the remaining ones should gradually disappear. Reserve
assertions for real invariants — conditions a caller could plausibly violate
that the type system can't express.

* Don't make String* as nullptr
Use String::emptyString instead of nullptr.
If you want to represent an absent String*, use Optional<String*>


### Handling a nullable pointer

There are the following choices where you handle a pointer which can be nullable.

* Use `Optional<T>` (preferred)

A variable, member, or return value that can be absent should be typed as
`Optional<T>` rather than a raw pointer overloaded with `nullptr`. Absence is
`NullOption`; presence is checked with implicit truthiness, mirroring plain
pointer null checks.

```cpp
Optional<Object*> A::functionA(ObjectC* c) {
    // `c` is not Optional: it is expected valid as-is.

    // Absence must be considered for an Optional value before use.
    Optional<ObjectB*> b = c->getObjectB();
    if (b && (*b)->isLoaded()) {
        ...
    }

    if (cnd) {
        return new Object;
    } else {
        return NullOption;
    }
}
```

Note: the pointer specialization `Optional<T*>` keeps no separate has-value
flag — assigning a null pointer collapses indistinguishably into the empty
state (see the comment at its definition in `StarfishBase.h`). Where
"explicitly set to null" must stay distinct from "empty", don't use
`Optional<T*>`.

* Use reference instead of pointer

### Be sure to explicitly assign nullptr if you need to release it.
```cpp
A::releaseMemeber() {
    // free(m_pointerMemeber); if you needs
    m_pointerMemeber = nullptr;
}
```
### Supported assertions macro list in Starfish
```cpp
...
STARFISH_ASSERT()
STARFISH_ASSERT_NOT_REACHED()
STARFISH_RELEASE_ASSERT_SHOULD_NOT_BE_HERE()
// ETC, See StarfishBase.h for more information
...
```

## Comments
### Comment Style
Both `//` and `/* */` style comments can be used, although `//` style is
much preferred.
### Add comment for newly function
* Having a clear function name and parameters will solve many readability problems.
* We are not aiming to generate an API doc. We think it is unneeded.
* We don't need to write comment for every function. We prefer to write comments at a place where function definition in cpp file
* What we want to write are (unusual, important, or pre/post conditions of) function behaviors that are difficult to deliver to readers by code. Some examples include, "This function should be called after finishing xxx, or it will give you yyy."
* Since writing comments is optional, we do not want to have rigid formats. (Also, not updating comments after updating actual code is bad). We are thinking of having simple comments starting with `//` in the header file above the function we want to add comments. (Again this is more like informal comment rule.)
* Which function to write the comment is more like up to developers. Each developer needs to decide what to write comments (or not)

* @yichoi says
I do not encourage to add comment
 - in the function body
 - in the header file


## Code readability

Make sure your code is obvious and readable with the following conventions.

```diff
++ // NOTE: the statements in green are preferred.

// Pointer and Optional emptiness checks use implicit truthiness, mirroring
// Optional's own operator bool (the prevailing style of the codebase).
- if (ptr != nullptr)
+ if (ptr)

// Non-boolean values (counts, lengths, ...) still compare explicitly --
// don't let an integer masquerade as a boolean.
- if (verbose && strlen(verbose))
+ if (verbose && strlen(verbose) > 0)

// Single-operand `!` is fine for pointer/Optional emptiness checks and for
// boolean-identifiable names ("isXXX", "shouldXXX", "didXXX", "hasXXX",
// "inXXX", "flagXXX"). Booleans never compare against true/false; other
// value categories keep an explicit right operand.
bool isLoaded();
bool sunnyToday();
int howMuchLoaded();

- if (!isLoaded() && howMuchLoaded() && ptr)
+ if (!isLoaded() && howMuchLoaded() != 0 && ptr)
- if (sunnyToday() == true)
+ if (sunnyToday())
```
