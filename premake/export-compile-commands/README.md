## Generate compile_commands.json for premake projects

This module implements [JSON Compilation Database Format
Specification](http://clang.llvm.org/docs/JSONCompilationDatabase.html) for
premake projects.

Install this module somewhere premake can find it, for example:

```
git clone https://github.com/tarruda/premake-export-compile-commands export-compile-commands
```

Then put this at the top of your system script(eg: ~/.premake/premake-system.lua):

```lua
require "export-compile-commands"
```

Note that while possible, it is not recommended to put the `require` line in
project-specific premake configuration because the "export-compile-commands"
module will need to be installed everywhere your project is built.

After the above steps, the "export-compile-commands" action will be available
for your projects:

```
premake5 export-compile-commands
```

The `export-compile-commands` action will generate one json file per
config/platform combination in each workspace, all under the `compile_commands`
subdirectory. For example, say you have defined `debug` and `release`
configurations with `x32` and `x64` platforms, the output will be something
like:

```
Generated WORKSPACE_BUILD_DIR/compile_commands/debug_x32.json...
Generated WORKSPACE_BUILD_DIR/compile_commands/debug_x64.json...
Generated WORKSPACE_BUILD_DIR/compile_commands/release_x32.json...
Generated WORKSPACE_BUILD_DIR/compile_commands/release_x64.json...
```

where each file contain the compilation commands for the corresponding
config/platform combo.
