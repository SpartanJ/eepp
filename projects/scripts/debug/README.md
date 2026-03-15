# eepp Debugger Scripts

This directory contains Python scripts for GDB and LLDB to make debugging the `eepp` framework easier.

Specifically, these scripts provide "pretty-printers" for custom, highly-optimized data structures like `EE::SmallVector`. Because `EE::SmallVector` uses advanced bit-packing and tagged pointers to minimize stack overhead, it can look like unreadable memory in a raw debugger. These scripts unmask the data so it reads exactly like a standard `std::vector`.

## GDB (GNU Debugger)

To make GDB automatically format `EE::SmallVector` objects, you need to load the `eepp_gdb.py` script.

### Option 1: Auto-Load for all eepp projects (Recommended)
Add the following line to your `~/.gdbinit` file to load the pretty-printers automatically every time you start GDB. Make sure to replace `/path/to/eepp` with your actual local path:

```
python exec(open("/path/to/eepp/projects/scripts/debug/eepp_gdb.py").read())
```

### Option 2: Manual Load (Per Session)

If you only want to load it for a specific debugging session, run this command inside the GDB prompt:

```
(gdb) source /path/to/eepp/projects/scripts/debug/eepp_gdb.py
```

## LLDB (Clang)

LLDB uses a different Python API for formatting variables. Use the eepp_lldb.py script to get clean summaries and expandable array elements.

### Option 1: Auto-Load for all eepp projects (Recommended)

Add this line to your `~/.lldbinit` file so LLDB automatically imports the formatting rules:

```
command script import /path/to/eepp/projects/scripts/debug/eepp_lldb.py
```

### Option 2: Manual Load (Per Session)

To load the script manually while LLDB is running, use this command in the LLDB prompt:

```
(lldb) command script import /path/to/eepp/projects/scripts/debug/eepp_lldb.py
```

## Verification

Once loaded, an `EE::SmallVector` containing elements will display cleanly in your watch window or terminal output:

### Without the script:

```
clientsCopy = { m_data = { _M_elems = "\x07\x00\x00..." } }
```

### With the script:

`clientsCopy = [Direct] size=3`

`[0] = 0x00007fffffff1230`

`[1] = 0x00007fffffff1280`

`[2] = 0x00007fffffff12a0`
