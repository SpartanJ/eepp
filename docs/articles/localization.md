# Localization with Locale and Translator

Use `EE::System::Translator` to choose the language of application text, and
`EE::System::Locale` to choose how numbers and percentages are displayed. The two
settings can be independent. An English interface can display a number as `12.345,67`
using the Argentine Spanish numeric locale.

## Choose locale IDs

`LocaleId` accepts a two- or three-letter language, an optional script, an optional
region, and one optional variant. It normalizes the separator and case, so `zh_Hant_TW` becomes `zh-Hant-TW` and
`en_US.UTF-8` becomes `en-US`. POSIX script modifiers such as `sr_RS@latin` and
variant modifiers such as `ca_ES@valencia` are also supported. Use `isValid()` to
check the shape of a tag from user input. Use `getTag()` to read its normalized form
while the `LocaleId` is alive.

A default-constructed `Translator` starts with the system UI language and uses `en` as
its catalog fallback. A default-constructed `Locale` uses the system's numeric setting.
`LocaleId::systemUiLanguage()` and `LocaleId::systemNumberLocale()` expose the two
settings for applications that want to display or store them. On Linux and other POSIX
systems, the UI language comes from `LC_ALL`, then `LC_MESSAGES`, then `LANG`. A
nonempty GNU `LANGUAGE` list such as `pt_BR:es` takes priority, as in gettext, unless
that locale is `C` or `POSIX`. The numeric setting reads `LC_NUMERIC` in place of
`LC_MESSAGES` and ignores `LANGUAGE`.

To search for data or catalogs, `LocaleId::fallback()` returns the next ID to try. It
usually removes the last subtag, as `parent()` does. When an ID has a region but no
script, and CLDR's likely script for that region differs from the bare language's,
it first inserts the script. For example, `zh-TW` continues with
`zh-Hant-TW → zh-Hant → zh`, and `sr-ME` continues with `sr-Latn-ME → sr-Latn → sr`.
`zh-CN` continues directly with `zh`, because bare `zh` already uses that script.

## Load and look up translations

Save UTF-8 catalogs with a `<resources>` root. This example is `i18n/en.xml`.

```xml
<resources language="en">
  <string name="files">Files</string>
  <string name="open_file">Open %s</string>
  <plurals name="file_count">
    <item quantity="one">%s file</item>
    <item quantity="other">%s files</item>
  </plurals>
</resources>
```

Load the catalogs before querying them. `loadFromFile()` uses the explicit language
argument when supplied, then `<resources language="…">`, then a locale ID in the
filename. The filename is only used for a `<resources>` element without a valid
attribute, so a catalog named `strings.xml` keeps its declared language. When the
declared language differs from a locale ID in the filename, the declaration is used
and a warning is logged. `loadFromString()` uses the explicit argument or the
attribute. Two-letter filenames such as `en.xml` and regional filenames such as
`es-AR.xml` work with the same resource format.

```cpp
#include <eepp/system/locale.hpp>
#include <eepp/system/translator.hpp>

using namespace EE;
using namespace EE::System;

Translator translations;
translations.setDefaultLanguage( "en" );
translations.setCurrentLanguage( "en-GB" );
const bool loaded = translations.loadFromFile( "i18n/en.xml" );

const String heading = translations.getString( "files", "Files" );
const String action = translations.getStringf( "open_file", "report.txt" ); // Open report.txt
```

`loadFromFile()` returns `true` on success. `loadFromDirectory( "i18n" )` returns
`true` if a catalog fails to load. It resolves each file's language as `loadFromFile()`
does, and loads nothing when two files resolve to the same language. For example,
`strings.xml` and `fr.xml` collide if both declare `fr`, and `es-AR.xml` and `ES_ar.xml`
collide if neither declares a language.

For each key, `Translator` visits the current language and each
`LocaleId::fallback()` candidate, followed by the default language and its candidates.
For example, `zh-Hant-TW` tries `zh-Hant-TW → zh-Hant → zh`, and `zh-TW` tries
`zh-TW → zh-Hant-TW → zh-Hant → zh` before the default chain. This keeps the lookup
on the requested or likely script branch until it reaches the bare language. If the
current chain already includes the default language, as with `en-GB` and `en`, the
default chain is not searched again. Load the parent catalogs you want to use.
`getString()` returns the supplied fallback text when the key is absent from all
loaded catalogs in the chain. Its fallback is an empty `String` if omitted.

In a native UI, configure the translator returned by `UISceneNode::getTranslator()`
before loading the layout. Translated attributes can use `@string/files`, for example
`<TextView text="@string/files" />`. In C++, `scene.i18n( "files", "Files" )` looks up
the key with fallback text. Refresh existing widget text when the application changes
languages. [Application UI authoring](ui_authoring.md)

## Select plural templates

Give each `<plurals>` entry an `other` item and the forms the catalog language uses.
The cardinal categories are `zero`, `one`, `two`, `few`, `many`, and `other`.
For each catalog in the lookup chain, `getPlural()` tries the selected category
and then `other`. It returns the supplied fallback template if neither is found
anywhere in the chain.

`getPlural()` uses the current language's rules for the current catalog chain.
Pass an optional `LocaleId` as its fourth argument to select another rule locale for
that chain. The default catalog chain uses the default language's rules. Pass an
integer for a whole count or an ordinary ASCII decimal string to retain visible
fraction digits. For example, English selects `one` for `1` and `other` for `"1.0"`.
`bool` and character types are not accepted as counts.
`PluralRules::select()` exposes the same category selection for applications that
manage their own messages. Its declaration is in `<eepp/system/pluralrules.hpp>`.

Plural entries are stored apart from strings. `getDictionary()` holds only `<string>`
entries, and `getPluralDictionary()` maps each language and key to its
`Translator::PluralForms`, whose `get( PluralCategory )` returns the template or
`nullptr`. `setPlural( "fr", "file_count", PluralCategory::One, "%s fichier" )` adds a
template in code. It ignores an invalid language, a key that is empty or contains NUL,
or an invalid category, because such an entry could not be saved and loaded again.
`setString()` applies the same key rule. `saveToStream()` writes
both kinds of entries.

`getPlural()` returns a template. To insert a localized count, format the same value
separately and pass its UTF-8 representation to the template's `%s` placeholder.

```cpp
const Locale numbers( "es-AR" );
const int count = 12345;
const auto format = translations.getPlural( "file_count", count, "%s files" ).toUtf8();
const auto displayedCount = numbers.formatNumber( count ).toUtf8();
const auto message = String::format( format, displayedCount ); // 12.345 files
```

Keep the printf argument types consistent across translations of a key. Write `%%`
for a literal percent in a template used with `getStringf()` or `String::format()`.

## Format numbers and percentages

`formatNumber()` and `formatPercent()` return `EE::String`. They accept signed and
unsigned integers up to 64 bits, doubles, and exact ASCII decimals such as
`"12345.67"`. `bool` and character types do not compile, rather than formatting as
numbers. The decimal-string form accepts an optional sign, one or more digits,
and an optional dot followed by one or more digits. Use this form when decimal ties
need to be exact. For percentages, pass a ratio. The formatter multiplies it by 100
before rounding.

```cpp
const Locale argentine( "es-AR" );
const auto amount = argentine.formatNumber( "12345.67" ); // 12.345,67
const auto wholePercent = argentine.formatPercent( "0.125" ); // 12%

NumberFormatOptions fixed;
fixed.minFractionDigits = 2;
fixed.maxFractionDigits = 2;
const auto share = argentine.formatPercent( "0.125", fixed ); // 12,50%
const auto rounded = argentine.formatNumber( "1.245", fixed ); // 1,24
```

The CLDR pattern supplies the default precision, typically up to three fraction
digits for numbers and zero for percentages. The default rounding mode is
`NumberRounding::HalfEven`. `HalfAwayFromZero` and `TowardZero` are also available.
Set `grouping = false` to suppress separators or `showPlusSign = true` to request
a localized plus sign. Fraction limits can be at most
`NumberFormatOptions::MaxFractionDigits` (100). Invalid decimal strings and
negative, conflicting or larger fraction limits return an empty `String`. NaN and infinity use the locale's symbols. Negative zero, and a
negative value that rounds to zero, keep the localized negative sign: `-0.4` with
no fraction digits formats as `-0` in English.

Formatting uses the locale's CLDR digits and symbols. For example, `Locale( "ar-EG" )`
uses Arabic digits, while `Locale( "ar-EG", NumberingSystem::Latin )` uses the locale's
Latin digits and associated symbols. The formatter tries the locale and each
`LocaleId::fallback()` candidate, and uses US English when no profile matches. As in
catalog lookup, `Locale( "zh-TW" )` tries `zh-TW`, then `zh-Hant-TW`, and uses the
`zh-Hant` profile, since CLDR has no separate `zh-Hant-TW` number data.

For date and time settings, pass `numbers.getId().getTag()` to
`DateTimeLocale::fromLocaleName()` and use the result with `DateTimeFormatter` or
the date and time controls. For right-to-left layouts, set the UI's direction
separately. [Date and time controls](ui_date_time_pickers.md) · [UI direction and RTL](ui_rtl.md)

For the pinned CLDR data, regeneration command, and conformance checks, see the
[CLDR maintenance guide](../../projects/scripts/cldr/README.md).
