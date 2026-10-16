# Nickname Generator for C++

> **Requires C++23** (e.g., `-std=c++23` for GCC/Clang, `/std:c++latest` for MSVC).

[![Nickname Generator for C++](https://raw.githubusercontent.com/dasmig/nickname-generator/master/doc/nickname-generator.png)](https://github.com/dasmig/nickname-generator/releases)

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/dasmig/nickname-generator/master/LICENSE.MIT)
[![CI](https://github.com/dasmig/nickname-generator/actions/workflows/ci.yml/badge.svg)](https://github.com/dasmig/nickname-generator/actions/workflows/ci.yml)
[![GitHub Releases](https://img.shields.io/github/release/dasmig/nickname-generator.svg)](https://github.com/dasmig/nickname-generator/releases)
[![GitHub Issues](https://img.shields.io/github/issues/dasmig/nickname-generator.svg)](https://github.com/dasmig/nickname-generator/issues)
[![C++23](https://img.shields.io/badge/standard-C%2B%2B23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Header-only](https://img.shields.io/badge/type-header--only-green.svg)](https://github.com/dasmig/nickname-generator#integration)
[![Platform](https://img.shields.io/badge/platform-linux%20|%20windows%20|%20macos-lightgrey.svg)](https://github.com/dasmig/nickname-generator)
[![Documentation](https://img.shields.io/badge/docs-GitHub%20Pages-blue.svg)](https://dasmig.github.io/nickname-generator/)

**[API Reference](https://dasmig.github.io/nickname-generator/)** · **[Usage Guide](doc/usage.md)** · **[Releases](https://github.com/dasmig/nickname-generator/releases)**

## Features

The library currently supports the following:

- **Nickname Generation**. The library randomly generates a nickname based on either a name or a list of words.

- **Batch Generation**. Generate thousands of results from one seed, on several threads with the same output for any thread count, optionally unique by a key, and stream them as JSON Lines or CSV (see the usage guide).

- **Leetifying Process**. After selecting a name/word it will randomly apply any number of customizations to uniquefy the nickname.

- **Word Lists Supported**. Currently the following word lists are supported and a matching database is packed together with the library (see [Data and Method](doc/data.md)):
  - Adjectives
  - Animals
  - Japanese

- **Request-first API**. `nickname_request` holds per-call choices; `get_nickname(request)` generates normally and `get_nickname(request, seed)` produces reproducible results. `nickname::seed()` records the seed for replay.

- **Identical Everywhere**. A seed gives the same nickname on every compiler, standard library and CPU (GCC, Clang, MSVC, Emscripten; x86-64 and ARM64), checked by known-answer tests. Case changes do not depend on the locale.

- **Extensible Word Lists**. Load your own lists, or add and remove words in the shipped ones, from `.words` and `.tsv` files or text (`extend()`), checked before they are applied.

- **JSON and Replay**. `to_json()` on every result, and a `signature()` recording the seed and the word lists it came from.

- **Compiled-in Data**. Define `DASMIG_NICKNAME_EMBED` to build the word lists into the program, for WebAssembly and other builds without files.

- **Resource Inspection**. `has_wordlists()` and `list_names()` let you verify what is loaded before generating.

- **Multi-Instance Support**. Construct independent `nng` instances with their own word lists and random engine — ideal for embedding inside other generators. The classic `instance()` singleton is still available for convenience.


## Integration
 
[`nicknamegen.hpp`](https://github.com/dasmig/nickname-generator/blob/master/dasmig/nicknamegen.hpp) is the library [released here](https://github.com/dasmig/nickname-generator/releases). Keep [`core.hpp`](https://github.com/dasmig/nickname-generator/blob/master/dasmig/core.hpp) and the [`core/`](https://github.com/dasmig/nickname-generator/tree/master/dasmig/core) directory ([dasmig-core](https://github.com/dasmig/dasmig-core)) in the same directory. You need to add

```cpp
#include <dasmig/nicknamegen.hpp>

// For convenience.
using nng = dasmig::nng;
```

to the files you want to generate nicknames and set the necessary switches to enable C++23 (e.g., `-std=c++23` for GCC and Clang).

Additionally you must supply the nickname generator with the [`resources`](https://github.com/dasmig/nickname-generator/tree/master/resources) folder also available in the [release](https://github.com/dasmig/nickname-generator/releases), or define `DASMIG_NICKNAME_EMBED` to compile the word lists in.

Version 2 changes every seeded result; see [Migrating from v1](doc/usage.md#migrating-from-v1).


## Usage

It's important to note that due to the necessity of supporting multiple cultures characters and the way std::string works on windows, this library uses std::wstring to return the generated nicknames.

When requesting a nickname for the first time the singleton loads the shipped word lists, from the compiled-in copy or from `resources/lite` (it looks in `resources`, `../resources`, `nickname-generator/resources` and `../nickname-generator/resources`). If the resources are elsewhere, load them manually. `load(path)` reads every word list below the directory and replaces what was loaded; `extend(path)` adds to it.

```cpp
#include <dasmig/nicknamegen.hpp>

// For convenience.
using nng = dasmig::nng;

// Manually load the resources folder if necessary.
nng::instance().load("path/to/resources");

// Generate a nickname based on a player name.
std::wstring github_nickname = nng::instance().get_nickname({.name = L"Kind Github User"});

// Generate a random nickname from the loaded word lists.
std::wstring random_nickname = nng::instance().get_nickname({});

// Access the original source string.
const dasmig::nickname_request request{.name = L"Jane Doe"};
auto nick = nng::instance().get_nickname(request);
std::wcout << nick.plain() << L" -> " << nick << L'\n';

// Deterministic generation — same seed always produces the same nickname.
auto seeded = nng::instance().get_nickname(request, 42);

// Replay a previous nickname using its seed.
auto replay = nng::instance().get_nickname(request, seeded.seed());

// Deterministic sequences use an explicit seed for each call.
for (std::uint64_t seed = 100; seed < 103; ++seed)
{
    auto next = nng::instance().get_nickname(request, seed);
    std::wcout << next << L'\n';
}

// The result as JSON, with the seed and the word lists it came from.
std::string json = seeded.to_json();

// Add a word list of your own on top of the shipped ones.
nng::instance().extend("mods/planets.words");

// Independent instance — separate word lists and random engine.
nng my_gen;
my_gen.load("path/to/resources");
std::wstring nick2 = my_gen.get_nickname({.name = L"Own Instance"});
```

`nickname_request{}.name` is empty by default, selecting from word lists. A
non-empty name retains the existing 25% name-derived / 75% word-list behavior
(always name-derived without lists). Resource loading stays on the generator.
The former string/no-argument overloads and generator-level `seed()` /
`unseed()` have been removed; see [Request API migration](doc/usage.md#request-api-migration).

For the complete feature guide — transforms, word lists, extending data, compiled-in data, thread safety, and more — see the **[Usage Guide](doc/usage.md)**.

## Building

```bash
# Example
make

# Tests (and with the word lists compiled in)
make test
make test-embed

# Benchmarks (results in build/bench.tsv)
make bench

# Code coverage
make coverage

# API docs
make docs

# Regenerate the compiled-in word lists
make embed
```

## Compiler Support

Tested with:
- Clang 18+ (`-std=c++23`)
- GCC 14+ (`-std=c++23`)
- MSVC 19.38+ (`/std:c++latest`)

## Dependencies

| Dependency | Version | Bundled | Purpose |
|-----------|---------|---------|---------|
| [dasmig-core](https://github.com/dasmig/dasmig-core) | 1.0 | Yes (`core.hpp`, `core/`) | Platform-independent random engine and choices, data layers, JSON |
| [Catch2](https://github.com/catchorg/Catch2) | 3.x | Yes (amalgamated) | Unit testing |

## Related Libraries

| Library | Description |
|---------|-------------|
| [name-generator](https://github.com/dasmig/name-generator) | Culturally appropriate full names |
| [birth-generator](https://github.com/dasmig/birth-generator) | Demographically plausible birthdays |
| [biodata-generator](https://github.com/dasmig/biodata-generator) | Procedural human physical characteristics |
| [city-generator](https://github.com/dasmig/city-generator) | Weighted city selection by population |
| [country-generator](https://github.com/dasmig/country-generator) | Weighted country selection by population |
| [entity-generator](https://github.com/dasmig/entity-generator) | ECS-based entity generation |

## License

This library is released under the [MIT License](https://opensource.org/licenses/MIT).

```
MIT License

Copyright (c) 2020-2026 Diego Dasso Migotto
```
