# Usage Guide

This guide covers every feature of the nickname-generator library in detail. For a quick overview, see the [README](../README.md). For the full API reference, run `doxygen Doxyfile` from the repository root and open `doc/api/html/index.html`.

[TOC]

<!-- GitHub-rendered TOC (Doxygen uses [TOC] above instead) -->
<ul>
<li><a href="#quick-start">Quick Start</a></li>
<li><a href="#installation">Installation</a></li>
<li><a href="#loading-resources">Loading Resources</a></li>
<li><a href="#generating-nicknames">Generating Nicknames</a></li>
<li><a href="#nickname-transforms">Nickname Transforms</a></li>
<li><a href="#word-lists">Word Lists</a></li>
<li><a href="#thread-safety">Thread Safety</a></li>
<li><a href="#seeding-and-deterministic-generation">Seeding and Deterministic Generation</a></li>
<li><a href="#multi-instance-support">Multi-Instance Support</a></li>
<li><a href="#error-reference">Error Reference</a></li>
</ul>

## Quick Start

```cpp
#include <dasmig/nicknamegen.hpp>
#include <iostream>

int main()
{
    auto& gen = dasmig::nng::instance();

    // Random nickname from word lists.
    std::wcout << gen.get_nickname() << L"\n";

    // Nickname based on a player name.
    std::wcout << gen.get_nickname(L"John Smith") << L"\n";
}
```

## Installation

1. Copy `dasmig/nicknamegen.hpp` and `dasmig/random.hpp` into your include path.
2. Copy the `resources/` folder (containing `.words` files) so it is accessible at runtime.
3. Compile with C++23 enabled: `-std=c++23`.

## Loading Resources

On first access the singleton constructor probes these relative paths automatically:

| Priority | Path |
|----------|------|
| 1 | `resources/` |
| 2 | `../resources/` |
| 3 | `nickname-generator/resources/` |

If your resources are elsewhere, call `load()` explicitly:

```cpp
dasmig::nng::instance().load("/opt/data/wordlists");
```

Calling `load()` multiple times is safe — each call adds to the existing word lists.

## Generating Nicknames

### Random (word list based)

```cpp
auto nick = dasmig::nng::instance().get_nickname();
std::wstring text = nick;  // implicit conversion
```

### Name-based

When a name is provided there is a 25 % chance the nickname will be derived
from it (using initials, mixing name parts, etc.). Otherwise a random word
from the loaded lists is used.

```cpp
auto nick = dasmig::nng::instance().get_nickname(L"Alberto Bins Elis");
```

### Accessing the original

```cpp
auto nick = dasmig::nng::instance().get_nickname(L"Jane Doe");
std::wcout << nick.plain();   // "Jane Doe" or the source word
std::wcout << nick;           // the transformed nickname
```

## Nickname Transforms

Each generated nickname passes through two stages:

### Leetify stage

One of these transforms is randomly applied (50 % chance):

| Transform | Example |
|-----------|---------|
| reverse | `emankcin` |
| duovowel | `nicknamee` |
| oneleet | `n1ckname` |
| allleet | `n1ckn4m3` |
| xfy | `XnicknameX` |
| yfy | `nicknamy` |
| numify | `nickname2000` |
| tracefy | `nickname-` |
| ingify | `nicknaming` |

### Format stage

A case format is randomly selected with weighted distribution:

| Format | Weight | Example |
|--------|--------|---------|
| lower_case | 8 | `nickname` |
| sentence_case | 5 | `Nickname` |
| upper_case | 4 | `NICKNAME` |
| bathtub_case | 3 | `NicknamE` |
| title_case | 2 | `NickName` |
| camel_case | 2 | `nickName` |
| reverse_sentence_case | 2 | `nicknamE` |
| winding_case | 1 | `nIcKnAmE` |
| random_case | 1 | `niCKnaMe` |
| random_single_case | 1 | `nicknaMe` |

There is also a 1 % chance of `snake_case` being applied first (`Nick_Name`).

## Word Lists

The library ships three word lists in `resources/`:

| File | Contents |
|------|----------|
| `adjectives.words` | English adjectives |
| `animals.words` | Animal names |
| `japanese.words` | Japanese-themed words |

Custom word lists can be added by placing `.words` files in the resources
directory. Each file should contain one word per line.

## Thread Safety

Each `nng` instance is independent. The static `instance()` singleton uses a
local static for safe initialization.

| Operation | Thread-safe? |
|-----------|-------------|
| `instance()` | Yes (static local) |
| `get_nickname()` on **different** instances | Yes |
| `get_nickname()` on the **same** instance | **No** — requires external synchronization |
| `load()` | **No** — must not be called concurrently with `get_nickname()` on the same instance |

Call `load()` once during initialization before spawning threads. For
concurrent generation, give each thread its own `nng` instance.

## Seeding and Deterministic Generation

### Per-call seeding

Pass an explicit seed to `get_nickname()` to produce the same nickname every time:

```cpp
auto nick = dasmig::nng::instance().get_nickname(L"John Smith", 42);
// Always produces the same result for the same name + seed.
```

### Seed replay

Every nickname records its seed. Retrieve it with `nickname::seed()` and pass
it back to reproduce the exact same result:

```cpp
auto nick = dasmig::nng::instance().get_nickname(L"Jane Doe");
auto saved_seed = nick.seed();

// Later, replay:
auto replay = dasmig::nng::instance().get_nickname(L"Jane Doe", saved_seed);
// replay == nick
```

### Sequence seeding

Seed the generator's thread-local engine for a reproducible sequence of
nicknames:

```cpp
auto& gen = dasmig::nng::instance();

gen.seed(100);
auto a = gen.get_nickname(L"Alice");
auto b = gen.get_nickname(L"Bob");

gen.seed(100);                      // reset
auto a2 = gen.get_nickname(L"Alice"); // identical to a
auto b2 = gen.get_nickname(L"Bob");   // identical to b

gen.unseed(); // restore non-deterministic behavior
```

`seed()` and `unseed()` return `*this` for chaining:

```cpp
gen.seed(42).get_nickname(L"Test");
```

> **Note:** `seed()` / `unseed()` are thread-local — they only affect the
> calling thread.

### Checking resources

Use `has_wordlists()` before generating to verify resources loaded:

```cpp
if (!dasmig::nng::instance().has_wordlists())
{
    dasmig::nng::instance().load("/path/to/resources");
}
```

## Multi-Instance Support

In addition to the singleton `instance()`, you can construct independent `nng`
objects. Each instance owns its own word lists and random engine, so they
operate without shared state — ideal for embedding inside other generators or
running multiple configurations side by side.

```cpp
// Create an independent generator.
dasmig::nng gen;

// Load a custom word list directory.
gen.load("path/to/my-words");

// Generate nicknames — independent from instance().
std::wcout << gen.get_nickname(L"Player One") << L"\n";
```

### Separate Seeding

Each instance maintains its own engine, so seeding one does not affect another:

```cpp
dasmig::nng a;
dasmig::nng b;
a.load("resources");
b.load("resources");

a.seed(42);
b.seed(99);

// a and b produce different sequences.
std::wcout << a.get_nickname(L"Test") << L"\n";
std::wcout << b.get_nickname(L"Test") << L"\n";
```

### Move Semantics

Instances can be moved (but not copied), which lets you store them in
containers or transfer ownership:

```cpp
dasmig::nng src;
src.load("resources");

dasmig::nng dst = std::move(src);
// dst now owns the word lists and engine; src is in a moved-from state.
```

## Error Reference

| Exception | Thrown by | Condition |
|-----------|-----------|----------|
| `std::invalid_argument` | `nng::get_nickname()` | Empty name **and** no word lists loaded |
