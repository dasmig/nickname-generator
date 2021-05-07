# Nickname Generator for C++

[![Nickname Generator for C++](https://raw.githubusercontent.com/dasmig/nickname-generator/master/doc/nickname.png)](https://github.com/dasmig/nickname-generator/releases)

[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://raw.githubusercontent.com/dasmig/nickname-generator/master/LICENSE.MIT)
[![GitHub Releases](https://img.shields.io/github/release/dasmig/nickname-generator.svg)](https://github.com/dasmig/nickname-generator/releases)
[![GitHub Downloads](https://img.shields.io/github/downloads/dasmig/nickname-generator/total)](https://github.com/dasmig/nickname-generator/releases)
[![GitHub Issues](https://img.shields.io/github/issues/dasmig/nickname-generator.svg)](https://github.com/dasmig/nickname-generator/issues)

## Features

The library currently supports the following:

- **Nickname Generation**. The library randomly generates a nickname based on either a name or a list of words.

- **'Leetifying' Process**. After selecting a name/word it will randomly apply any number of customizations to uniquefy the nickname.

- **Word Lists Supported**. Currently the following word lists are supported and a matching database is packed together with the library: 
  - None


## Integration
 
[`nicknamegen.hpp`](https://github.com/dasmig/nickname-generator/dasmig/blob/master/nicknamegen.hpp) is the single required file [released here](https://github.com/dasmig/name-generator/releases). You need to add

```cpp
#include <dasmig/nicknamegen.hpp>

// For convenience.
using nng = dasmig::nng;
```

to the files you want to generate nicknames and set the necessary switches to enable C++17 (e.g., `-std=c++17` for GCC and Clang).

Additionally you must supply the nickname generator with the [`resources`](https://github.com/dasmig/name-generator/tree/master/resources) folder also available in the [release](https://github.com/dasmig/name-generator/releases).

### Disclaimer

This README was heavily inspired by ['nlhomann/json'](https://github.com/nlohmann/json).