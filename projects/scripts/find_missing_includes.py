#!/usr/bin/env python3
import os
import re

# Catches anything inside <...> or "..."
INCLUDE_REGEX = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]')


def extract_includes_from_file(filepath):
    """Helper function to read a file and return a set of its includes."""
    includes = set()
    if not os.path.exists(filepath):
        return includes

    try:
        with open(filepath, "r", encoding="utf-8-sig") as f:
            for line in f:
                match = INCLUDE_REGEX.search(line)
                if match:
                    # Add the exact path inside the quotes/brackets to the set
                    includes.add(match.group(1))
    except Exception as e:
        print(f"Error reading {filepath}: {e}")

    return includes


def find_missing_includes():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, "..", ".."))

    include_base_dir = os.path.join(repo_root, "include")
    eepp_dir = os.path.join(include_base_dir, "eepp")

    if not os.path.exists(eepp_dir):
        print(f"Error: Could not find {eepp_dir}")
        return

    print("Scanning eepp modules for missing includes...\n")

    total_missing = 0

    # 1. Find all master module headers (e.g., ui.hpp, core.hpp, scene.hpp)
    for item in sorted(os.listdir(eepp_dir)):
        if not item.endswith(".hpp") or item == "ee.hpp" or item == "version.hpp":
            continue

        module_name = item[:-4]  # strip .hpp (e.g., 'core')
        module_dir = os.path.join(eepp_dir, module_name)

        # Only process if there is a matching directory (e.g., include/eepp/core/)
        if os.path.isdir(module_dir):
            master_header_path = os.path.join(eepp_dir, item)

            # 2. Extract includes from the main module header
            existing_includes = extract_includes_from_file(master_header_path)

            # --- THE CORE EXCEPTION ---
            # If this is the core module, also grab includes from eepp/core/core.hpp
            if module_name == "core":
                core_inner_path = os.path.join(module_dir, "core.hpp")
                existing_includes.update(extract_includes_from_file(core_inner_path))

            # 3. Walk the module's directory recursively to find all .hpp files
            missing_in_module = []
            for root, dirs, files in os.walk(module_dir):
                for file in files:
                    if file.endswith(".hpp"):
                        full_path = os.path.join(root, file)

                        rel_path = os.path.relpath(full_path, include_base_dir)
                        rel_path = rel_path.replace(os.sep, "/")

                        # Skip the inner core.hpp file itself so it doesn't get flagged
                        if module_name == "core" and rel_path == "eepp/core/core.hpp":
                            continue

                        # 4. Check if the file is missing from the known includes
                        if rel_path not in existing_includes:
                            missing_in_module.append(f"#include <{rel_path}>")

            # 5. Print results nicely
            if missing_in_module:
                print(f"--- Missing in {item} ---")
                for missing in sorted(missing_in_module):
                    print(missing)
                print("")  # Spacing
                total_missing += len(missing_in_module)

    if total_missing == 0:
        print("All module headers are perfectly up to date!")
    else:
        print(f"Found {total_missing} potentially missing includes.")
        print("Note: Some of these might be internal/detail headers you intentionally left out.")


if __name__ == "__main__":
    find_missing_includes()
