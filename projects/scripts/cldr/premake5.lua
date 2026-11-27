-- Standalone maintainer action. The root build compiles the checked-in tables.
newoption {
	trigger = "cldr",
	value = "DIRECTORY",
	description = "Directory containing the unpacked cldr-json/ packages"
}

newoption {
	trigger = "out",
	value = "DIRECTORY",
	description = "Existing directory for localedata.generated.cpp"
}

newoption {
	trigger = "all-number-locales",
	description = "Require the complete cldr-numbers-full package"
}

newaction {
	trigger = "generate-cldr",
	description = "Generate eepp's locale tables from the pinned Unicode CLDR release",
	execute = function()
		assert(_OPTIONS.cldr and _OPTIONS.out and _OPTIONS["all-number-locales"],
			"Provide --cldr, --out and --all-number-locales")
		dofile(path.join(_MAIN_SCRIPT_DIR, "generate.lua")).generate(
			path.getabsolute(_OPTIONS.cldr), path.getabsolute(_OPTIONS.out))
	end
}
