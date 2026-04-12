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

- **Leetifying Process**. After selecting a name/word it will randomly apply any number of customizations to uniquefy the nickname.

- **Word Lists Supported**. Currently the following word lists are supported and a matching database is packed together with the library:
  - Adjectives 
  - Animals
  - Japanese

- **Deterministic Seeding**. Per-call `get_nickname(name, seed)` for reproducible results, generator-level `seed()` / `unseed()` for deterministic sequences, and `nickname::seed()` for replaying a previous generation.

- **Resource Inspection**. `has_wordlists()` lets you verify resources loaded before generating.

- **Multi-Instance Support**. Construct independent `nng` instances with their own word lists and random engine — ideal for embedding inside other generators. The classic `instance()` singleton is still available for convenience.


## Integration
 
[`nicknamegen.hpp`](https://github.com/dasmig/nickname-generator/blob/master/dasmig/nicknamegen.hpp) is the single required file [released here](https://github.com/dasmig/nickname-generator/releases). You need to add

```cpp
#include <dasmig/nicknamegen.hpp>

// For convenience.
using nng = dasmig::nng;
```

to the files you want to generate nicknames and set the necessary switches to enable C++23 (e.g., `-std=c++23` for GCC and Clang).

Additionally you must supply the nickname generator with the [`resources`](https://github.com/dasmig/nickname-generator/tree/master/resources) folder also available in the [release](https://github.com/dasmig/nickname-generator/releases).


## Usage

It's important to note that due to the necessity of supporting multiple cultures characters and the way std::string works on windows, this library uses std::wstring to return the generated nicknames.

When requesting a nickname for the first time the library will attempt to load the resource files containing each word list (the default path is ./resources). It's important to manually load the resources folder if it's present in a different location. The library will recursively iterate through all entries in the loading directory, so only a single call to the root folder containing the word lists is necessary.

```cpp
#include <dasmig/nicknamegen.hpp>

// For convenience.
using nng = dasmig::nng;

// Manually load the resources folder if necessary.
nng::instance().load("path/to/resources");

// Generate a nickname based on a player name.
std::wstring github_nickname = nng::instance().get_nickname(L"Kind Github User");

// Generate a random nickname from the loaded word lists.
std::wstring random_nickname = nng::instance().get_nickname();

// Access the original source string.
auto nick = nng::instance().get_nickname(L"Jane Doe");
std::wcout << nick.plain() << L" -> " << nick << std::endl;

// Deterministic generation — same seed always produces the same nickname.
auto seeded = nng::instance().get_nickname(L"Jane Doe", 42);

// Replay a previous nickname using its seed.
auto replay = nng::instance().get_nickname(L"Jane Doe", seeded.seed());

// Seed the engine for a deterministic sequence.
nng::instance().seed(100);
// ... generate nicknames ...
nng::instance().unseed(); // restore non-deterministic state

// Independent instance — separate word lists and random engine.
nng my_gen;
my_gen.load("path/to/resources");
std::wstring nick2 = my_gen.get_nickname(L"Own Instance");
```

For the complete feature guide — transforms, word lists, thread safety, and more — see the **[Usage Guide](doc/usage.md)**.

## Related Libraries

| Library | Description |
|---------|-------------|
| [name-generator](https://github.com/dasmig/name-generator) | Culturally appropriate full names |
| [birth-generator](https://github.com/dasmig/birth-generator) | Demographically plausible birthdays |
| [biodata-generator](https://github.com/dasmig/biodata-generator) | Procedural human physical characteristics |
| [city-generator](https://github.com/dasmig/city-generator) | Weighted city selection by population |
| [country-generator](https://github.com/dasmig/country-generator) | Weighted country selection by population |
| [entity-generator](https://github.com/dasmig/entity-generator) | ECS-based entity generation |