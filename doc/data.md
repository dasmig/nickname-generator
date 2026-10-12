# Data and Method

What the word lists are and how nicknames are built from them. For using the library, see the [usage guide](usage.md).

[TOC]

## Word Lists

The library ships one tier, `resources/lite/`; `dataset::full` loads the same lists.

| List | Words | Content |
| --- | --- | --- |
| `adjectives` | 4,842 | English adjectives, some hyphenated or multi-word ("well-argued") |
| `animals` | 108 | English animal names |
| `japanese` | 233 | Japanese words and slang in Latin script (romaji): "Kawaiso", "Sempai", "Yokatta", ... |

The lists are curated; where they came from was not recorded. Every word is ASCII. A list file has one word per line; the list's name is the file name.

Version 2 removed repeated words, which v1 counted twice: 38 in `animals` (Dog, Rabbit, Fish, ... appeared two or more times) and 4 in `japanese` (Ii, Ki, Omae).

## Generation Model

1. **Source.** With a name, one nickname in four is built from it (always, if no list is loaded): first name, last name, any part (counted twice), initials, a mix of the last two parts, first name plus last initial, first initial plus last name, or one part with its inner vowels removed, each equally likely. Otherwise a list is chosen uniformly, then a word from it uniformly.
2. **Decoration**, with probability one half: either one of reverse, double the first a/e/i/o, replace one letter with a leet digit, or replace up to two (retried while the result does not change, at most 7 times), or one of add X, reverse, end in y, add digits, add a dash, end in ing.
3. **Case**: snake case first with probability 1%, then one of upper (4/29), lower (8/29), as is (2/29), sentence (5/29), camel (2/29), last letter capital (2/29), first and last capital (3/29), alternating (1/29), random per letter (1/29) or one random capital (1/29).

Everything draws from the `"nickname"` stream split from the call seed, so the result is the same on every platform. Case changes use a fixed mapping for Basic Latin, Latin-1 Supplement and Latin Extended-A, not the C locale; other letters keep their case.

## Known Limitations

- The lists are English and romanised Japanese only.
- Name-based nicknames split on spaces only; hyphenated and particle names ("da Silva") are treated as separate parts.
- Leet speak maps a fixed set of Latin letters (o, i, s, e, a, g, t, b, q).
