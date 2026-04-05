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

| Operation | Thread-safe? |
|-----------|-------------|
| `instance()` | Yes (static local) |
| `get_nickname()` | Yes (read-only) |
| `load()` | **No** — must not be called concurrently with `get_nickname()` |

Call `load()` once during initialization before spawning threads.

## Error Reference

| Exception | Thrown by | Condition |
|-----------|-----------|----------|
| `std::invalid_argument` | `nng::get_nickname()` | Empty name **and** no word lists loaded |
