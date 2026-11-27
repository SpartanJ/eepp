#!/usr/bin/env python3
"""Check eepp against CLDR 49 samples, Babel's rule evaluator and ICU's formatter."""

import argparse
from decimal import Decimal, getcontext
import json
from pathlib import Path
import re
import subprocess

from babel.plural import PluralRule


CATEGORIES = ("zero", "one", "two", "few", "many", "other")
PLAIN_DECIMAL = re.compile(r"\d+(?:\.\d+)?", re.ASCII)
getcontext().prec = 96


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def official_samples(definition):
    for sample_group in definition.split("@")[1:]:
        for sample in sample_group.split(" ", 1)[1].split(","):
            bounds = [value.strip() for value in sample.split("~")]
            if not all(PLAIN_DECIMAL.fullmatch(value) for value in bounds):
                continue  # Ellipses and compact-exponent examples are outside ordinary decimals.
            if len(bounds) == 1:
                yield bounds[0]
                continue
            start, end = map(Decimal, bounds)
            step = Decimal(1).scaleb(start.as_tuple().exponent)
            if start.as_tuple().exponent != end.as_tuple().exponent or end - start > 1000 * step:
                yield from bounds
                continue
            while start <= end:
                yield format(start, "f")
                start += step


def plural_checks(core, inputs, expected):
    source = read_json(core / "supplemental/plurals.json")["supplemental"]
    assert source["version"]["_cldrVersion"] == "49"
    records = source["plurals-type-cardinal"]
    assert len(records) == 225
    samples = [str(value) for value in range(251)] + [
        "999", "1000", "10000", "100000", "999999", "1000000", "1000001", "2000000",
        "1000000.00", "1000000.5", "-1000000", "-1", "-1.0",
        "184467440737095516160000000000001", "1.000000000000000000000000000001",
    ]
    samples += [f"{whole}.{fraction}" for whole in (0, 1, 2, 10, 11, 20, 21, 99)
                for fraction in ("0", "00", "1", "01", "010", "11", "110", "5", "50", "99")]
    official_count = 0
    for locale, record in sorted(records.items()):
        rules = {}
        examples = {}
        for key, definition in record.items():
            category = key.removeprefix("pluralRule-count-")
            expression = definition.split("@", 1)[0].strip()
            if expression:
                rules[category] = expression
            examples.update((sample, category) for sample in official_samples(definition))

        reference = PluralRule(rules)
        for text in dict.fromkeys(samples + list(examples)):
            category = reference(Decimal(text))
            if text in examples and category != examples[text]:
                raise AssertionError(f"CLDR sample {locale} {text}: {examples[text]} vs Babel {category}")
            inputs.append(f"plural\t{locale}\t{text}")
            expected.append(str(CATEGORIES.index(category)))
        official_count += len(examples)
    print(f"{len(inputs):,} plural decisions across {len(records)} locale entries "
          f"({official_count:,} official ordinary-decimal samples)")


def number_checks(core, numbers, inputs):
    definitions = read_json(core / "supplemental/numberingSystems.json")
    digits = definitions["supplemental"]["numberingSystems"]
    candidates = sorted((numbers / "main").glob("*/numbers.json"))
    assert len(candidates) == 657
    count = len(inputs)
    for file in candidates:
        locale = file.parent.name
        info = read_json(file)["main"][locale]["numbers"]
        default = info["defaultNumberingSystem"]
        profiles = [(default, "default")]
        if default != "latn":
            profiles.append(("latn", "latin"))
        for system, name in profiles:
            symbols = info[f"symbols-numberSystem-{system}"]
            for style in ("decimal", "percent"):
                pattern = info[f"{style}Formats-numberSystem-{system}"]["standard"]
                for value, precision in (
                    ("0", "-"), ("-0.0001", "-"), ("1.245", "2"), ("1.255", "2"),
                    ("-1.245", "2"), ("-123.456", "1"), ("999.9995", "-"),
                    ("1234.5", "-"), ("12345.6789", "-"), ("-12345678.89", "-"),
                    ("18446744073709551615", "-"), ("0.125", "1"),
                ):
                    inputs.append("\t".join([
                        "number", locale, name, value, style, precision, pattern,
                        *(symbols[key] for key in ("decimal", "group", "plusSign", "minusSign", "percentSign")),
                        digits[system]["_digits"], str(info["minimumGroupingDigits"]),
                    ]))
    print(f"{len(inputs) - count:,} decimal/percent comparisons across {len(candidates)} locales "
          "using ICU with pinned CLDR 49 symbols and patterns")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cldr", required=True, type=Path, help="parent of the unpacked cldr-json directory")
    parser.add_argument("--binary", required=True, type=Path, help="compiled reference.cpp driver")
    args = parser.parse_args()

    root = args.cldr / "cldr-json"
    core = root / "cldr-core"
    numbers = root / "cldr-numbers-full"
    for package in (core, numbers):
        assert read_json(package / "package.json")["version"] == "49.0.0"
    inputs, expected = [], []
    plural_checks(core, inputs, expected)
    number_checks(core, numbers, inputs)
    process = subprocess.run([str(args.binary.resolve())], input="\n".join(inputs) + "\n",
                             check=True, encoding="utf-8", capture_output=True)
    outputs = process.stdout.split("\n")[:-1]
    assert len(outputs) == len(inputs), (len(outputs), len(inputs))
    failures = 0
    for index, (query, result) in enumerate(zip(inputs, outputs)):
        if index < len(expected):
            actual, reference = result, expected[index]
        else:
            actual, reference = (result.split("\t", 1) + [""])[:2]
        if actual != reference:
            failures += 1
            if failures <= 30:
                print(f"{query.split(chr(9))[:6]}: eepp={actual!r}, reference={reference!r}")
    if failures:
        raise SystemExit(f"{failures:,} mismatches")
    print("All comparisons passed.")


if __name__ == "__main__":
    main()
