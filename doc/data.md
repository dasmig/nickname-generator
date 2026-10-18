# Data and Method

What the word lists are and how nicknames are built from them. For using the library, see the [usage guide](usage.md).

[TOC]

## Word Lists

The library ships one tier, `resources/lite/`; `dataset::full` loads the same lists. There are nine plain lists, drawn from for any nickname, and 43 theme lists, drawn from only when a request names their tag.

| List | Words | Content |
| --- | --- | --- |
| `adjectives` | 4,842 | English adjectives, some hyphenated or multi-word ("well-argued") |
| `animals` | 108 | English animal names |
| `japanese` | 233 | Japanese words and slang in Latin script (romaji): "Kawaiso", "Sempai", "Yokatta", ... |
| `colors` | 57 | Colour names ("Crimson", "Cobalt") |
| `food` | 63 | Dishes, sweets, spices and nuts ("Pretzel", "Paprika") |
| `nature` | 68 | Weather, landscape and plants ("Tempest", "Willow") |
| `space` | 58 | Astronomy and spaceflight ("Nebula", "Voyager") |
| `myth` | 60 | Creatures, gods and legends from many traditions ("Kraken", "Valkyrie") |
| `gems` | 41 | Gemstones and minerals ("Opal", "Obsidian") |
| `tag_<tag>` | 8 each | 43 theme lists, below |

The first three lists are curated; where they came from was not recorded. The six lists added in version 4 and all the theme lists were written for this library. Every word is ASCII. A list file has one word per line; the list's name is the file name.

### Themes

A list named `tag_<tag>` is a theme. It is not part of the plain pool; it is drawn from when a request lists its tag. The 43 shipped themes are the tags that `trait-generator` puts on traits and `occupation-generator` puts on jobs, so a caller can pass them on as they are: `academic`, `activity`, `art`, `care`, `caring`, `chaos`, `combat`, `crafting`, `creative`, `desk`, `discipline`, `dreaming`, `duty`, `exploration`, `faith`, `flight`, `haste`, `knowledge`, `labour`, `leadership`, `manual`, `medicine`, `morale`, `outdoor`, `physical`, `planning`, `rest`, `risk`, `routine`, `safety`, `social`, `solitude`, `solo`, `stealth`, `survival`, `team`, `technical`, `trade`, `training`, `violence`, `waiting`, `wealth`, `willpower`. Each has eight words that suit the tag ("Brawler" and "Gladiator" for `combat`, "Hermit" and "Recluse" for `solitude`); they are a flavour, not a definition, and a word can be in a theme and a plain list.

Version 2 removed repeated words, which v1 counted twice: 38 in `animals` (Dog, Rabbit, Fish, ... appeared two or more times) and 4 in `japanese` (Ii, Ki, Omae).

## Generation Model

The per-call input is `nickname_request`; its `name` defaults to empty.
`get_nickname(request)` draws a call seed from the instance's random engine,
and `get_nickname(request, seed)` uses the supplied seed without advancing
that engine. Requests do not load or change word lists.

1. **Source.** With a name, one nickname in four is built from it (always, if no list is loaded): first name, last name, any part (counted twice), initials, a mix of the last two parts, first name plus last initial, first initial plus last name, or one part with its inner vowels removed, each equally likely. Otherwise, if the request has tags and at least one of them is a theme, one nickname in three is a word of a theme (a theme chosen uniformly among the request's themes, each counted once, then a word from it uniformly). Otherwise a plain list is chosen uniformly, then a word from it uniformly.
2. **Decoration**, with probability one half: either one of reverse, double the first a/e/i/o, replace one letter with a leet digit, or replace up to two (retried while the result does not change, at most 7 times), or one of add X, reverse, end in y, add digits, add a dash, end in ing.
3. **Case**: snake case first with probability 1%, then one of upper (4/29), lower (8/29), as is (2/29), sentence (5/29), camel (2/29), last letter capital (2/29), first and last capital (3/29), alternating (1/29), random per letter (1/29) or one random capital (1/29).

Everything draws from the `"nickname"` stream split from the call seed, so the result is the same on every platform. Case changes use a fixed mapping for Basic Latin, Latin-1 Supplement and Latin Extended-A, not the C locale; other letters keep their case.

## Known Limitations

- The lists are English and romanised Japanese only.
- A theme has eight words, so a character with one tag meets the same few words often. Add words to a theme with `extend()`.
- The tags choose a flavour, not a meaning: nothing checks that a word fits a particular trait.
- Name-based nicknames split on spaces only; hyphenated and particle names ("da Silva") are treated as separate parts.
- Leet speak maps a fixed set of Latin letters (o, i, s, e, a, g, t, b, q).
