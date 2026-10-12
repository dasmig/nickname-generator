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
<li><a href="#extending-data">Extending Data</a></li>
<li><a href="#replay-and-json">Replay and JSON</a></li>
<li><a href="#compiled-in-data">Compiled-in Data</a></li>
<li><a href="#migrating-from-v1">Migrating from v1</a></li>
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

1. Copy `dasmig/nicknamegen.hpp`, `dasmig/core.hpp` and the `dasmig/core/` directory into your include path, keeping them side by side.
2. Copy the `resources/` folder (`lite/` with the `.words` files) so it is accessible at runtime, or compile the lists in (see Compiled-in Data).
3. Compile with C++23 enabled: `-std=c++23`.

## Loading Resources

The library ships one tier, `resources/lite/`; `dataset::full` loads the same lists.

On first access the singleton (`nng::instance()`) loads it. It uses the compiled-in copy when built with `DASMIG_NICKNAME_EMBED`, and otherwise looks for `lite/animals.words` in these directories, relative to the working directory:

1. `resources`
2. `../resources`
3. `nickname-generator/resources`
4. `../nickname-generator/resources`

`load(dataset::lite)` searches the same way and returns `false` when it finds nothing. If your word lists are elsewhere, load the directory explicitly:

```cpp
dasmig::nng gen;
(void)gen.load(dasmig::dataset::lite);   // shipped lists
gen.load("/opt/data/wordlists");         // or a directory of your own
```

`load(path)` reads every `.words` and `.tsv` file below the directory, in path order, and **replaces** all lists and layers loaded before. A path that is not a directory is ignored; a malformed file throws `dasmig::core::tsv_error` and leaves the lists unchanged. To add to what is loaded, use `extend()` (see Extending Data).

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

The exact probabilities are in [Data and Method](data.md#generation-model).

### Leetify stage

With probability one half the nickname is decorated: half of the time with one of reverse, duovowel, oneleet or allleet (retried while the text does not change), otherwise with one of reverse, xfy, yfy, numify, tracefy or ingify.

| Transform | Example |
|-----------|---------|
| reverse | `emankcin` |
| duovowel | `nicknamee` |
| oneleet | `n1ckname` |
| allleet | `n1cknam3` |
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
| title_case (kept as is) | 2 | `NickName` |
| camel_case (first letter lower) | 2 | `nickName` |
| reverse_sentence_case | 2 | `nicknamE` |
| winding_case | 1 | `nIcKnAmE` |
| random_case | 1 | `niCKnaMe` |
| random_single_case | 1 | `nicknaMe` |

There is also a 1 % chance of `snake_case` being applied first (`Nick_Name`).

## Word Lists

The library ships three word lists in `resources/lite/` (see [Data and Method](data.md)):

| File | Contents |
|------|----------|
| `adjectives.words` | English adjectives |
| `animals.words` | Animal names |
| `japanese.words` | Japanese-themed words |

A `.words` file has one word per line; the list's name is the file name.
Lists of your own are added with `extend()` (see Extending Data).
A list is chosen uniformly, then a word from it, so a short list's words come
up more often than a long list's.

## Thread Safety

Each `nng` instance is independent. The static `instance()` singleton uses a
local static for safe initialization.

| Operation | Thread-safe? |
|-----------|-------------|
| `instance()` | Yes (static local) |
| `get_nickname()` on **different** instances | Yes |
| `get_nickname()` on the **same** instance | **No** — requires external synchronization |
| `load()`, `extend()` | **No** — must not be called concurrently with `get_nickname()` on the same instance |

Load and extend once during initialization before spawning threads. For
concurrent generation, give each thread its own `nng` instance; the seeded
overload `get_nickname(name, seed)` is `const` and does not touch the engine.

## Seeding and Deterministic Generation

### Per-call seeding

Pass an explicit seed to `get_nickname()` to produce the same nickname every time:

```cpp
auto nick = dasmig::nng::instance().get_nickname(L"John Smith", 42);
// The same name, seed and word lists give the same nickname on every
// compiler, standard library and CPU.
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

Seed the generator's engine for a reproducible sequence of nicknames:

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

> **Note:** `seed()` / `unseed()` affect only that instance's engine.

### Checking resources

Use `has_wordlists()` before generating to verify resources loaded, and
`list_names()` to see which lists are in use:

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

## Extending Data

The shipped lists are a base layer. Your own files are loaded on top of them to add lists, add words or remove words, without editing the shipped files. A list's words are its keys.

```cpp
auto& gen = dasmig::nng::instance();

// A .words file adds its words to the list named after the file:
// mods/animals.words adds animals, mods/planets.words adds a new list.
gen.extend("mods/animals.words");

// A directory: every .words and .tsv file below it, applied together.
gen.extend("mods/");

// From TSV text with a `word` column, for example supplied by a
// WebAssembly host or a game scenario.
gen.extend_from_string("planets", "word\nMercury\nVenus\nMars\n", "scenario: planets");

// Remove words with a `disabled` column; a list left with no words is not used.
gen.extend_from_string("animals", "word\tdisabled\nCockroach\t1\n", "cleanup");

// Add mode refuses words that are already in the list.
gen.extend("mods/japanese.words", dasmig::core::layer_mode::add);
```

- A layer is checked before anything changes. A bad layer throws `dasmig::core::tsv_error` with the file, line and column, and the lists stay as they were; a directory is applied all or nothing.
- `layers("animals")` lists what is loaded for one list: source, mode, words added, replaced and removed, and warnings such as unknown columns.
- A file that is neither `.words` nor `.tsv` is refused with `std::invalid_argument`.
- `load(...)` replaces the base and drops every layer.

## Replay and JSON

Every result records the seed and the data it came from:

```cpp
auto n = gen.get_nickname(L"Alberto Bins Elis");
n.seed();                    // the call seed
n.signature().data;          // gen.data_hash() when it was generated

// Same seed, same name, same lists: the same nickname everywhere.
auto again = gen.get_nickname(L"Alberto Bins Elis", n.seed());

if (n.signature().data != gen.data_hash())
{
    // Different lists or layers are loaded now; the replay would differ.
}
```

`to_json()` gives the result as JSON (UTF-8), the form the WebAssembly layer and the game consume:

```json
{"nickname":"SILE","source":"Alberto Bins Elis","signature":{"seed":"000000000000002a","data":"87dd5622316ce1fe"}}
```

Seeds are strings because JavaScript numbers cannot hold every 64-bit value. `to_json(writer)` writes into an existing `dasmig::core::json_writer`, for composing larger documents.

## Compiled-in Data

Define `DASMIG_NICKNAME_EMBED` before including the header to compile the word lists into the program (about 50 KB). `load(dataset::lite)` and `instance()` then use them and need no files, which is what a WebAssembly build wants. The compiled-in lists have the same `data_hash()` as the files, so seeds replay the same either way.

```cpp
#define DASMIG_NICKNAME_EMBED
#include <dasmig/nicknamegen.hpp>
```

`dasmig/nicknamegen_data.hpp` is generated from `resources/lite` by `make embed`; CI checks it is up to date.

## Migrating from v1

Version 2 moves the library onto [dasmig-core](https://github.com/dasmig/dasmig-core).

- **Every generated nickname changes.** The engine (xoshiro256\*\*) and the random choices are now the core's own, so a seed gives the same nickname on every compiler, standard library and CPU. Seeds saved with v1 do not reproduce their v1 results. The generation model is unchanged (checked against v1 over 20,000 draws per scenario).
- **Case changes do not depend on the locale.** v1 used `std::towupper` and `std::towlower`, so whether accented letters changed case depended on the C locale and the platform. v2 maps Basic Latin, Latin-1 Supplement and Latin Extended-A itself; other letters keep their case.
- **Word lists.** The files moved to `resources/lite/`, and repeated words were removed (38 in `animals`, 4 in `japanese`), so those words are no longer two or three times as likely. Lists are used in name order, not in the order the filesystem returns them.
- **`load(path)` replaces** the lists instead of adding to them; use `extend()` to add. A malformed file now throws `dasmig::core::tsv_error`. `load(dataset)` is new.
- **Files:** replace `random.hpp` with `core.hpp` and the `core/` directory (see Installation).
- **New:** `extend()`, `extend_from_string()`, `layers()`, `list_names()`, `data_hash()`, `nickname::signature()`, `nickname::to_json()`, `DASMIG_NICKNAME_EMBED`, `.tsv` word lists.

## Error Reference

| Exception | Thrown by | Condition |
|-----------|-----------|----------|
| `std::invalid_argument` | `nng::get_nickname()` | Empty name **and** no word lists loaded |
| `std::invalid_argument` | `nng::extend(path)` | A file that is neither `.words` nor `.tsv` |
| `std::out_of_range` | `nng::layers(list)` | No list with that name |
| `dasmig::core::tsv_error` | `load()`, `extend()`, `extend_from_string()` | A malformed file or layer (message gives file, line and column); the lists are unchanged |
