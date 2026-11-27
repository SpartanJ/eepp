# CLDR locale data in eepp

This directory maintains the Unicode CLDR tables that `EE::System::Locale`,
`LocaleId` and `PluralRules` use. For the consumer API (locale IDs, catalog lookup,
plural templates and number formatting), see
[Localization with Locale and Translator](../../../docs/articles/localization.md).

The tables hold cardinal plural rules, number symbols and patterns, digit sets, and
the regional script aliases that `LocaleId::fallback()` inserts, such as `zh-TW` to
`zh-Hant-TW`.

## Regeneration

Normal builds compile the checked-in
[`localedata.generated.cpp`](../../../src/eepp/system/localedata.generated.cpp).
The standalone Premake action is used when refreshing the data. It reads the full
Unicode CLDR JSON 49.0.0 packages.

| Package archive | SHA-256 |
| --- | --- |
| [cldr-core-49.0.0.tgz](https://registry.npmjs.org/cldr-core/-/cldr-core-49.0.0.tgz) | `085b1c7d83f00692505c79d575c21eea89534466eb3e70b455688851a45c87bc` |
| [cldr-numbers-full-49.0.0.tgz](https://registry.npmjs.org/cldr-numbers-full/-/cldr-numbers-full-49.0.0.tgz) | `753da9481ac5c0d0566a23ab89c3945d7e4cb1dae6f6d5a87c0329a8640e7593` |

From the repository root, verify the archive checksums and unpack each archive
with `--strip-components=1` into its package directory under `/path/to/input/cldr-json`.
Then generate and compare a temporary copy of the tables.

```sh
out=$(mktemp -d)
premake5 --file=projects/scripts/cldr/premake5.lua \
  --cldr=/path/to/input --out="$out" --all-number-locales generate-cldr
cmp "$out/localedata.generated.cpp" src/eepp/system/localedata.generated.cpp
```

The generator checks all 657 number locales and 225 explicit cardinal rule
entries. It also verifies that each regional ID that resolved through a CLDR
likely-subtags alias still selects the same number profile through
`LocaleId::fallback()`. It validates the supported pattern grammar, numeric systems and table
field widths before writing output. The source is reproducible. It contains no
timestamps or local paths. See the [Unicode 3.0 license](../../../LICENSES/UNICODE-LICENSE-3.0.txt)
and the [CLDR 49 number and plural specification](https://www.unicode.org/reports/tr35/tr35-79/tr35-numbers.html).

## Independent checks

The regular eepp tests cover catalog fallback and serialization, numeric
precision and UTF-8, and plural operands. The optional maintainer check builds
a small driver against a release eepp library and the installed ICU development
package. The Python script also requires Babel.

```sh
c++ -std=c++20 -O2 -Iinclude projects/scripts/cldr/reference.cpp \
  -Llibs/linux/x86_64 -Wl,-rpath,"$PWD/libs/linux/x86_64" -leepp \
  $(pkg-config --cflags --libs icu-i18n icu-uc) -o /tmp/eepp-cldr-reference
python3 projects/scripts/cldr/check-reference.py \
  --cldr /path/to/input --binary /tmp/eepp-cldr-reference
```

The check supplies the pinned CLDR rules to Babel and the same release's
symbols and patterns to ICU. It compares the official ordinary-decimal plural
samples and additional values, and exercises every supplied number locale and
its Latin override where applicable. Compact exponents are outside the plural
API's ordinary-decimal input.
