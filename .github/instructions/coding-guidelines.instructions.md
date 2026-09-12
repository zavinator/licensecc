---
description: "Use when writing or editing C++ source files (cpp/hpp/h/cc/cxx) in licensecc. Covers RAII, value semantics, const-correctness, C++11 constraints, smart-pointer construction, error handling across the C API boundary (return error codes, don't throw), the backward-compatibility policy (prefer a clean solution over a compatibility shim), and common pitfalls like dangling c_str() and temporaries bound to references."
name: "C++ Coding Guidelines"
applyTo: "**/*.{cpp,hpp,h,cc,cxx}"
---

# Coding Guidelines

These guidelines are for the `licensecc` library specifically.

## General rule

Keep the implementation simple. This is a goal.
  - If implementing a new feature spirals down in sea of details and it start to become very complicated for a sea of little issues (backward compatibility, edge cases, pre-esistent logic) stop, ask and evaluate different approaches. 
  - Don't be afraid to do changes to the current code/functions if this simplifies the future implementation.
  - Simplify every new implementation. Do not care of backward compatibility.


## Repo Conventions

### Naming

- Functions and methods: `CamelCase` (`readLicenses`, `mergeLicenses`, `toLicenseInfo`, `getAdapterInfos`) 
- files use `snake_case` ('license_verifier.hpp')

### Comments

- The documentation will be rendered with Doxygen. Use its specific syntax to comment
- Do not put comments inside the methods. Only reserve for the very few cases where the code is extremely hard to comprehend (bit shifts, machine operations)
- Comment only classes declaration in .hpp files.
- Do not automatically add comments on methods if not specifically asked.

### Header guards

Use `#ifndef FOO_H_` / `#define FOO_H_` / `#endif`, matching the convention used in
the large majority of headers in this repo. Do not use `#pragma once` pick the `#ifndef` guard style for new headers.

### `using namespace`

`using namespace std;` (and similar) is used throughout `.cpp` files in this
codebase and that's fine — but it must **never** appear in a header (`.h`/`.hpp`),
since headers are included into consumers' translation units and would leak the
namespace into their code.

### Error handling across the C API boundary

Failure is a return value in this library, not an exception: leaf functions return
`FUNCTION_RETURN`, the C API returns `LCC_EVENT_TYPE`, and the caller decides what to
do with it. The C API (`include/licensecc/licensecc.h`: `identify_pc`,
`acquire_license`, `confirm_license`, `release_license`) is why this matters: a caller
may be plain C, a different runtime, or across a DLL boundary, where an escaping
exception is undefined behavior, not a catchable error.

For new and refactored code that means:
- Report failure by returning an error code; don't throw. If you touch a leaf that
  throws, convert it in the same change (`base/file_utils.cpp` `get_file_contents`
  and `base/string_utils.cpp` `identify_format` are already converted — keep them
  that way).
- `license.cpp` holds the one and only `catch`: `acquire_license` converts anything
  that still escapes into an error code before it reaches the caller. Don't add
  another one on the path, and don't reach for `noexcept` to silence a throw — it
  documents intent but doesn't make a failing function safe. `Licensecc::identify_pc`
  catches internally because `generate_user_pc_signature` reports failure by throwing.

### Smart pointer construction

- This project targets **C++11** (`CMAKE_CXX_STANDARD 11` in the top-level
  `CMakeLists.txt`) — `std::make_unique` is **C++14** and is not available here.
  Construct with `std::unique_ptr<T>(new T(...))` directly; don't suggest or add
  `make_unique` calls.
- Don't `dynamic_cast` a freshly-constructed derived pointer up to its own base
  (`unique_ptr<Base>(dynamic_cast<Base*>(new Derived()))`, seen in
  `identification_strategy.cpp` and `LocatorFactory.cpp`) — the derived-to-base
  conversion is implicit and always safe at that point (a `unique_ptr<Derived>`
  converts to `unique_ptr<Base>` on its own in C++11), so `dynamic_cast` there only
  adds a needless runtime check. Just write `unique_ptr<Base>(new Derived())`.

### Buffers

Prefer `std::vector<unsigned char>` or `std::string` over raw `new[]`/`delete[]`
for byte buffers (signature buffers, hashes, raw hardware data). Where a raw buffer
is unavoidable (e.g. interop with an OS or OpenSSL API that wants a raw pointer),
scope its lifetime with a smart pointer with an array deleter or a `std::vector`'s
`.data()`, not a bare `new[]` paired with a manual `delete[]`.

### String copying into fixed-size buffers

`strncpy` is banned: it doesn't always NUL-terminate (when the source is ≥ n bytes)
and silently truncates. Use the repo's `mstrlcpy` helper (declared in
`src/library/base/string_utils.h`, a `strlcpy`-style implementation) for any copy
into a fixed-size C buffer, and pass the full destination size — it always writes
the terminating `\0`:

```cpp
// WRONG: may leave the buffer unterminated when src >= sizeof(dst)
strncpy(dst, src, sizeof(dst) - 1);

// CORRECT: always NUL-terminates, copies at most sizeof(dst) - 1 bytes
mstrlcpy(dst, src, sizeof(dst));
```

### Backward compatibility

Prefer the clean solution. Do **not** add code that exists only to keep an old configuration, an old header, or an old behavior compiling — no `#ifndef`/`#define` fallbacks that silently supply a
default for a setting the project should declare, no deprecated overloads, no `#if` branches whose
only purpose is "a project generated three years ago didn't define this".

Use the setting directly and let the compiler produce the error. Then document the break in
`CHANGELOG.md` under a `## Breaking Changes:` section for the release: say what has to be redone
(e.g. regenerate `licensecc_properties.h`, re-issue licenses) and how.

Exception: the *public* API (`include/licensecc/`, the C and C++ entry points `identify_pc`, `acquire_license`, `confirm_license`, `release_license`) is what third-party applications link against — keep it stable, don't change signatures or remove enum values without an explicit request from the user.

## General C++ Guidance

### 1. Resource Management

#### RAII (Resource Acquisition Is Initialization)

Unlike Java's garbage collection, C++ uses RAII for automatic resource management:
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) instead of raw pointers
- Resources should be acquired in constructors and released in destructors
- Prefer stack allocation over heap allocation when possible

### 2. Value Semantics vs Reference Semantics

#### Copy vs Move Semantics

- Understand when objects are copied vs moved
- Use `std::move()` explicitly when transferring ownership
- Be aware of copy elision optimizations in modern C++

### 3. Const Correctness

- Use `const` wherever possible to prevent unintended modifications
- `const` member functions guarantee they won't modify the object's state
- Helps compiler optimize and makes code intentions clearer

### 4. Memory Safety

- Avoid raw `new`/`delete`, prefer containers and smart pointers
- Be cautious with arrays and pointer arithmetic
- Use `std::vector` instead of raw arrays, `std::string` instead of C-strings

### 5. Failure Handling

- Report failure with an error code and let the caller decide, as described in
  "Error handling across the C API boundary" — don't throw.
- Use RAII so resources are released on every early-return path; there is no stack
  unwinding to fall back on.
- Keep cleanup explicit: one return path per outcome, resources freed before returning.

### 6. Method visibility

- Prefer static methods inside .cpp files over methods in 'private:' sections.  Evaluate case by case depending on how much the method needs to access class fields.
- reduce to the minimum the surface of a module .cpp exposed in .h and .hpp files. 
- if some method is used only in tests add the declaration in the test file only.

## Avoid C++ Common Errors

### Temporary Objects Pitfalls

#### Binding Temporary Objects to Non-const References

**Problem:** Do not bind a temporary object to a non-const reference, unlike in some other languages where object passing is by reference by default.

```cpp
// WRONG - This will not compile
void modifyValue(int& val) { val++; }
modifyValue(5); // Error: cannot bind rvalue to non-const lvalue reference

// CORRECT - Use const reference or pass by value
void modifyValue(const int& val) { /* read-only access */ }
void modifyValue(int val) { /* copy and modify local */ }
```

#### Returning References to Local Objects

**Problem:** Returning a reference to a local/temporary object creates dangling references.

```cpp
// WRONG - Dangling reference
const std::string& getTempString() {
    return std::string("temporary"); // Returns reference to destroyed object
}

// CORRECT - Return by value or use smart pointers
std::string getTempString() {
    return std::string("temporary"); // Safe copy/move
}
```

#### Returning references to Temporary Objects [often see in '.c_str()']

**Problem:** `std::string::c_str()` does not own its underlying data — it points to
memory owned by the `std::string` it was called on. If you call it on a temporary
object, that pointer becomes invalid instantly: the temporary `std::string` vanishes
at the end of the full expression (the semicolon), and `filename` now points to freed
stack space. (Note: `std::string_view` is a C++17 type and isn't relevant here — this
project is C++11 — but the same dangling-pointer trap applies to any raw pointer or
reference taken from a temporary's internals, `c_str()` included.)

**The Fix:** Keep the temporary std::string alive in a variable first, or pass .c_str() directly into the function parameter without storing the pointer in a intermediate variable:

```cpp
// WRONG: Calling .c_str() on a temporary string
const char* filename = (std::string("data_") + "2026.txt").c_str();
// The temporary string created above dies AT THE END OF THIS LINE.
FILE* f = fopen(filename, "r"); // 💥 Undefined Behavior! 'filename' is a dangling pointer.


// CORRECT: Pass directly to the function
FILE* f = fopen((std::string("data_") + "2026.txt").c_str(), "r");
```

#### Passing Temporary Objects to Functions That Store References

**Problem:** Temporary objects passed to functions may outlive the function call but not the expression, causing dangling references.
```cpp
// WRONG - Potential dangling reference
class MyClass {
    const std::string& stored_ref;
public:
    MyClass(const std::string& ref) : stored_ref(ref) {} // Dangerous if ref is temporary
};
MyClass obj(std::string("temporary")); // stored_ref now points to destroyed object

// CORRECT - Store by value or ensure lifetime
class MyClass {
    std::string stored_value; // Store by value
public:
    MyClass(const std::string& val) : stored_value(val) {}
};
```
