import os
import re
import sys
import argparse

INCLUDE_REGEX = re.compile(r'^\s*#\s*include\s*(?:<((?:eepp/)[^>]+)>|"([^"]+)")')
PRAGMA_ONCE_REGEX = re.compile(r"^\s*#\s*pragma\s+once\s*")

visited_files = set()


def amalgamate(file_path, search_paths, current_file_dir):
    # 1. Check relative to current file, 2. Check global search paths
    potential_paths = [os.path.join(current_file_dir, file_path)] + [
        os.path.join(sp, file_path) for sp in search_paths
    ]

    resolved_path = None
    for p in potential_paths:
        abs_p = os.path.abspath(p)
        if os.path.exists(abs_p):
            resolved_path = abs_p
            break

    if not resolved_path:
        return None

    if resolved_path in visited_files:
        return f"// [Already included: {file_path}]\n"

    visited_files.add(resolved_path)
    print(f"Embedding: {resolved_path}")

    output = []
    new_current_dir = os.path.dirname(resolved_path)

    try:
        # utf-8-sig automatically strips invisible BOM characters!
        with open(resolved_path, "r", encoding="utf-8-sig") as f:
            lines = f.readlines()
    except Exception as e:
        return f"// ERROR reading {file_path}: {str(e)}\n"

    for line in lines:
        if PRAGMA_ONCE_REGEX.match(line):
            continue  # Cleanly strip pragma once

        match = INCLUDE_REGEX.match(line)
        if match:
            included_file = match.group(1) or match.group(2)
            content = amalgamate(included_file, search_paths, new_current_dir)

            if content is not None:
                output.append(f"\n// >>> Begin: {included_file} >>>\n")
                if not content.endswith("\n"):
                    content += "\n"
                output.append(content)
                output.append(f"// <<< End: {included_file} <<<\n\n")
            else:
                output.append(line)  # Keep system header
        else:
            output.append(line)

    return "".join(output)


if __name__ == "__main__":
    # Dynamically resolve paths
    script_dir = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.abspath(os.path.join(script_dir, "..", ".."))
    include_dir = os.path.join(repo_root, "include")

    # Default output path
    default_out = os.path.join(repo_root, "eepp.hpp")

    # Setup argparse for CLI options
    parser = argparse.ArgumentParser(
        description="Amalgamate eepp C++ headers into a single, redistributable header file."
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        default=default_out,
        help=f"Path to the output file (default: {default_out})",
    )

    args = parser.parse_args()

    root_file = "eepp/ee.hpp"

    print("--- Starting Amalgamation ---")
    result = amalgamate(root_file, [include_dir], include_dir)

    if result is None:
        print(f"Error: Could not find the root file '{root_file}' in '{include_dir}'.")
        sys.exit(1)

    final_code = "#ifndef EEPP_AMALGAMATED_HPP\n#define EEPP_AMALGAMATED_HPP\n\n"
    final_code += result
    final_code += "\n#endif // EEPP_AMALGAMATED_HPP\n"

    # Resolve output path and ensure the directory exists
    out_file = os.path.abspath(args.output)
    out_dir = os.path.dirname(out_file)
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(out_file, "w", encoding="utf-8") as out:
        out.write(final_code)

    print(f"--- Success! Created {out_file} ---")
