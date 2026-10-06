# Date and time controls

`UIDateTimeEdit` is one segmented `UITextInput` surface with date, time, and local date-time
modes. `UIDatePicker` adds a calendar popup, `UITimePicker` configures time mode without a
popup, and `UIDateTimePicker` combines a calendar and time editor in one popup.
`UICalendar` can also be used as a standalone widget.

## Native XML

```xml
<datepicker id="birthday" value="1990-06-17" />
<timepicker id="start" value="20:30:00" minute-step="15" />
<datetimepicker id="scheduled" value="2026-09-28T20:30:00" />
<datetimeedit mode="date" value="2026-09-28" />
<calendar first-day-of-week="1" value="2026-09-28" />
```

Values are optional typed civil values: `CalendarDate`, `TimeOfDay`, and `LocalDateTime`,
from `<eepp/system/datetime.hpp>`. They do not carry time zones. Use `setDate()`, `setTime()`,
or `setDateTime()` and the corresponding optional getters. `clear()` removes a value when
`allow-empty` is true (the default). Canonical setters clamp to the active mode's bounds and
emit `OnValueChange` only when the typed value changes. The `value` property exposes ISO
serialization for ordinary data binding. Hidden seconds and milliseconds remain part of the
value; serialization preserves nonzero milliseconds even if they are not displayed.

## Serialization and Unix timestamps

`DateTimeFormatter` in `<eepp/system/datetimeformat.hpp>` formats ISO values independently of
the display locale and parses them with an explicit pattern:

```cpp
using namespace EE;
using namespace EE::System;

const CalendarDate date{ 2026, 9, 28 };
const std::string storedDate = DateTimeFormatter::toISODate( date ); // "2026-09-28"
const auto restoredDate = DateTimeFormatter::parseDate( storedDate, DateTimeFormat( "yyyy-MM-dd" ) );

const LocalDateTime value{ date, { 20, 14, 32, 125 } };
const std::string storedValue = DateTimeFormatter::toISOLocalDateTime( value, true );
const auto restoredValue = DateTimeFormatter::parseDateTime(
	storedValue, DateTimeFormat( "yyyy-MM-dd'T'HH:mm:ss.SSS" ) );

// The application knows this value uses UTC-03:00; the types do not store a timezone.
const Int32 utcOffsetSeconds = -3 * 60 * 60;
const auto timestamp = value.toUnixTimestampMilliseconds( utcOffsetSeconds );
if ( timestamp ) {
	const auto restored = LocalDateTime::fromUnixTimestampMilliseconds(
		*timestamp, utcOffsetSeconds );
}
```

Unix timestamps count seconds or milliseconds since `1970-01-01T00:00:00Z`, without leap
seconds. `CalendarDate::toUnixTimestamp()` uses midnight UTC; `fromUnixTimestamp()` extracts
the UTC date containing that instant. `LocalDateTime` conversions default to UTC and accept an offset
in seconds, where `local = UTC + offset`. The offset is not inferred from the
system timezone and does not resolve daylight-saving gaps or ambiguous times. Applications
using named timezones must resolve the offset for the particular date/time before conversion.

The seconds methods discard milliseconds, rounding down even before the epoch. The
`toUnixTimestampMilliseconds()` / `fromUnixTimestampMilliseconds()` methods preserve them.
Conversions return `std::optional`: invalid civil values, an out-of-range year, or an
unrepresentable Int64 millisecond timestamp produce `std::nullopt`. The full Int32 year range
fits in Int64 seconds, but only part of it fits in Int64 milliseconds. These conversions are
independent of locale, `time_t` size, and the machine's timezone. For a date-only database
column, ISO date text or typed date fields preserve its civil meaning directly.

## Editing

Click a section to select it. Left/Right navigate sections; Home/End select the first/last.
Up/Down step the active section. Numeric entry automatically advances when a complete value
has been entered. Pending digits are shown with underscores, commit on navigation, Enter,
focus loss, or timeout, and can be canceled with Escape. Configure the timeout in C++ with
`setPendingDigitTimeout()` (default: one second). Delete/Backspace clear the value when allowed.
Copy and select-all use normal input commands. Paste replaces the whole value, including when
only one section is selected, and accepts ISO regardless of the display format. Invalid input
preserves the previous valid value. Wheel edits require both focus and `wheel-editing: true`;
wheel editing is disabled by default. `allow-editing: false` prevents user mutation.

Month/year edits clamp the day at month ends. Time-only arithmetic wraps within the day;
local date-time arithmetic carries across date boundaries. AM/PM entry changes the period
without advancing the date. Utility arithmetic outside the representable Int32 year range
leaves the value unchanged.

## Locale and properties

Display defaults come from `DateTimeLocale::system()` (Windows regional settings on Windows), with a centralized locale-name fallback
for information unavailable through the standard library. `setLocale()` accepts explicit date
order/separator, hour cycle, month and weekday names, AM/PM labels, and first weekday; this also
supports locales unavailable on the host. `fromLocaleName()` resolves ordering and cycle
fallbacks; applications can supply translated names in the returned structure.

| Property | Meaning |
| --- | --- |
| `mode` | `date`, `time`, or `datetime` on the shared editor |
| `date-format`, `time-format` | `locale` (default), or quoted token pattern |
| `hour-cycle` | `locale`, `12`, or `24` |
| `show-seconds` | Show seconds without milliseconds; default `false` |
| `show-milliseconds` | Show milliseconds and seconds; default `false` |
| `hour-step`, `minute-step`, `second-step` | Positive integer steps |
| `min-date`, `max-date` | Date-mode ISO bounds |
| `min-time`, `max-time` | Time-mode ISO bounds |
| `min-datetime`, `max-datetime` | Local date-time ISO bounds |
| `allow-empty`, `wheel-editing` | Empty-value and focused wheel editing policies |
| `active-section` | `none`, `day`, `month`, `year`, `hour`, `minute`, `second`, `millisecond`, `am-pm` |
| `popup-to-root` | Parent picker popup to scene root instead of its window container |
| `popup-open` | Inspectable picker open state |
| `first-day-of-week` | Calendar weekday: Sunday `0` through Saturday `6` |
| `calendar-view` | Calendar `days`, `months`, or `years` |
| `displayed-month`, `focused-date` | Calendar ISO navigation state |

Date patterns require day, month, and year. Time patterns require one hour token and minutes;
12-hour patterns also require `a`. Tokens: `d`/`dd`, `M`/`MM`, `yy`/`yyyy`, `H`/`HH`, `h`/`hh`,
`m`/`mm`, `s`/`ss`, `S`/`SS`/`SSS`, and `a`. Quote literal text with single quotes inside the
pattern. Two-digit years parse into 2000–2099. Standard serialized dates use `yyyy-MM-dd`,
times `HH:mm:ss[.SSS]`, and local date-times `yyyy-MM-ddTHH:mm:ss[.SSS]`.

Setting a minimum above the maximum moves the maximum to that minimum; setting a maximum
below the minimum moves the minimum to that maximum. Empty bound properties remove constraints.

Seconds and milliseconds can be toggled separately with `setShowSeconds()` and
`setShowMilliseconds()` or their CSS/XML properties. Both default to hidden. Enabling
milliseconds includes seconds; hiding milliseconds leaves seconds enabled. Hiding seconds
hides both. These options change display precision without discarding stored precision, and
also update the date-time popup footer. An explicit `time-format` controls its own sections.

## Calendar popup

Click the trailing button, press F4, or press Alt+Down. Opening an empty picker displays today
without assigning it. Date-only selection updates the value, closes the popup, and restores
editor focus. In a date-time picker, selecting a date preserves the time and keeps the popup
open so the footer can edit it. Selecting a date into an empty value uses midnight, then
applies its constraints.

The footer uses the same segmented editing as `UITimePicker`, with SVG up/down buttons for
hour, minute, and any displayed seconds, milliseconds, or AM/PM sections. It follows the
field's locale, format, steps, and boundary-date time constraints. Changing these settings while
the popup is open updates its controls immediately. Setting the owner read-only closes the popup.
Committed edits update the field immediately and preserve hidden precision. `getPopupTimePicker()` returns the owned
footer editor after the first opening. Enter in the footer closes the popup; Escape cancels
pending digits and closes it while preserving already committed changes. Focus leaving the
field and its entire popup also closes it. The popup measures both calendar and time footer
before choosing its placement, flips above when space permits, and clamps to scene bounds.
`UIPopUp::findBestPosition()` supplies the shared world-pixel placement calculation used by
field popups and `UIMenu::findBestMenuPos()` for trigger and cursor menus. Cascading menus
retain their menu-specific rules for avoiding the previous menu.

Calendar selection and navigation are separate. Arrows move focus by day/week, Home/End move
to week boundaries, PageUp/PageDown move by month, and Ctrl+PageUp/PageDown move by year.
Enter explicitly selects focus. Click the title to choose a month, then click it again to choose
a year. Previous/Next page through those views. Today navigates to today without selecting it.
`OnItemSelected` reports explicit selection, including re-selection of the same date;
`OnValueChange` reports a changed selection. The 42 day widgets are reused across navigation.

## Theme and example

Breeze provides light/dark palette styling. Theme tags include `datetimeedit`, `datepicker`,
`timepicker`, `datetimepicker`, `datepicker::button`, `datetimepicker::button`, `calendar`,
and `calendar::header/previous/next/title/weekdays/weekday/grid/day/today`.
The combined popup exposes `datetimepicker::popup`, `datetimepicker::time`, and
`datetimepicker::time-up/time-down`; arrow classes identify `hour`, `minute`, `second`,
`millisecond`, and `am-pm`. CSS controls the footer font, padding, minimum height, and SVG
arrow size in `dp`. Picker buttons use the standard arrow cursor; editable text uses the I-beam.
Calendars size to their text metrics by default, with a theme minimum of `160dp`
set by the `Calendar` rule in `bin/assets/ui/breeze.css` and `bin/assets/ui/uitheme.css`.
Override `min-width` / `min-height` to change the preferred size while retaining automatic
text fitting.
Changing fonts automatically updates the popup size, header, weekday row, and day grid.
Explicit fixed width/height remain application overrides; choose enough space for the selected font.
Picker buttons use an inline SVG in `foreground-image`, with `foreground-size: 12dp 12dp`,
centered positioning, and theme-controlled `foreground-tint`. The SVG uses a white base fill
so the native image tint supplies the light/dark and hover colors. No font glyph is required.
Calendar previous/next buttons likewise use CSS-supplied left/right SVGs, centered at `12dp`.
Read-only picker buttons use `[allow-editing=false]` selectors to mute the calendar icon and
remove its hover highlight, while leaving the input text readable and selectable.
Day classes are `selected`, `today`, `outside-month`, and `focused`; constrained cells use
normal disabled state. Today uses a border, selection a fill.

Build `eepp-ui-date-time-picker` and run `bin/eepp-ui-date-time-picker`; `--us` switches to a
Sunday-first, 12-hour locale. `--light`, `--small-font`, `--large-font`, and `--narrow`
exercise theme and sizing variants. F11 opens the inspector. Set `EEPP_PIXEL_DENSITY=2` to check
scaling, and use the inspector to vary font size, width, and placement.

## Follow-up scope

Range selection, named timezone support, and time-list popups remain separate follow-ups.

HTML `input` types `date`, `time`, and `datetime-local` now map to these native controls through
`UIHTMLInput`. HTML values are sanitized and serialized independently of the display locale:
`yyyy-MM-dd`, `HH:mm[:ss[.SSS]]`, and `yyyy-MM-ddTHH:mm[:ss[.SSS]]`. Local date-time values
have no timezone. Valid date/time value attributes retain their ISO spelling; local date-time
values normalize their separator to `T`, omit zero seconds, and trim trailing fractional zeros.

The adapter supports `min`, `max`, `step` (including `any` and fractional seconds), `required`,
`readonly`, and `disabled`. Range violations remain unchanged and report invalidity; native
controls retain their default clamping policy. Time ranges can cross midnight. Step validation
uses the minimum, otherwise the value attribute, otherwise the Unix civil epoch as its base.
Date steps use days; time and local date-time steps use seconds, defaulting to 60.

Use `UIHTMLInput::getValidity()` / `checkValidity()` and `UIHTMLForm::checkValidity()` /
`requestSubmit()` for the implemented temporal constraints. Clicking a submit control validates;
direct `UIHTMLForm::submit()` bypasses validation. Anonymous subparts use the light user-agent
defaults in `getHTMLDocumentDefaultsCSS()` in `uiwidgetcreator.cpp`, independently of the application
theme. The HTML
adapter marks private control content with the existing `UI_IGNORE_GLOBAL_CSS`; stylesheet matching
keeps the document's low-priority UA rules and excludes author rules for that content. Popups remain
in the document scene and fit its visible viewport. Date/time editors measure their intrinsic width
from the displayed text or placeholder plus padding and the calendar button, so author CSS is not
required to give an input a usable size.
General HTML validity for other input types, validation messages, and
`novalidate` / `formnovalidate` are outside this temporal mapping.
