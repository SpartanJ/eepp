-- Derived from the eepp lean CLDR proof of concept. Source data: Unicode-3.0.
local M = {}

local version = "49.0.0"
local categories = { "zero", "one", "two", "few", "many" }
local operands = { n = "N", i = "I", v = "V", w = "W", f = "F", t = "T", c = "C", e = "E" }

local function readJson(file)
	local input = assert(io.open(file, "rb"))
	local contents = input:read("*a")
	input:close()
	local decoded = json.decode(contents)
	assert(type(decoded) == "table", "Invalid JSON: " .. file)
	return decoded
end

local function sortedKeys(entries)
	local keys = {}
	for key in pairs(entries) do
		keys[#keys + 1] = key
	end
	table.sort(keys)
	return keys
end

local function split(text, separator)
	local parts, start = {}, 1
	while true do
		local first, last = text:find(separator, start, true)
		parts[#parts + 1] = text:sub(start, first and first - 1)
		if not first then
			return parts
		end
		start = last + 1
	end
end

local function trim(text)
	return (text:gsub("^%s+", ""):gsub("%s+$", ""))
end

local function bounded(value, max, field)
	assert(type(value) == "number" and value >= 0 and value <= max and value % 1 == 0,
		"Value out of range for " .. field .. ": " .. tostring(value))
	return value
end

local function addUnique(rows, indexes, row, max, field)
	local key = table.concat(row, ", ")
	local index = indexes[key]
	if index == nil then
		index = bounded(#rows, max, field)
		rows[#rows + 1] = row
		indexes[key] = index
	end
	return index
end

local function stringPool()
	local strings, offsets, size = {}, {}, 0
	local function intern(text)
		assert(type(text) == "string" and not text:find("\0", 1, true), "Invalid string pool entry")
		if not offsets[text] then
			offsets[text] = size
			size = bounded(size + #text + 1, 65535, "string pool")
			strings[#strings + 1] = text
		end
		return offsets[text]
	end
	intern("")
	return strings, intern
end

local function parseRelation(expression, ranges, relations)
	local operand, rest = expression:match("^([nivwftce])%s*(.*)$")
	assert(operand, "Unsupported plural operand: " .. expression)

	local modulo = 0
	if rest:sub(1, 1) == "%" then
		local divisor
		divisor, rest = rest:match("^%%%s*(%d+)%s*(.*)$")
		modulo = bounded(tonumber(divisor), 4294967295, "plural divisor")
		assert(modulo > 0, "Zero divisor: " .. expression)
	end

	local compare, list = rest:match("^(!?=)%s*(.*)$")
	assert(compare, "Unsupported plural comparator: " .. expression)
	local first = bounded(#ranges, 65535, "first range")
	for _, part in ipairs(split(list, ",")) do
		part = trim(part)
		local low, high = part:match("^(%d+)%.%.(%d+)$")
		if not low then
			low = part:match("^(%d+)$")
			high = low
		end
		low = bounded(tonumber(low), 4294967295, "plural range start")
		high = bounded(tonumber(high), 4294967295, "plural range end")
		assert(low <= high, "Invalid plural range: " .. expression)
		ranges[#ranges + 1] = { low, high }
	end
	relations[#relations + 1] = {
		modulo, first, bounded(#ranges - first, 255, "range count"),
		"Operand::" .. operands[operand], compare == "!=" and "true" or "false"
	}
end

local function buildPlurals(cardinal, parents, intern, data)
	local setByRule, byLocale = {}, {}
	for _, locale in ipairs(sortedKeys(cardinal)) do
		local record, expressions, keyParts = cardinal[locale], {}, {}
		for id, category in ipairs(categories) do
			local definition = record["pluralRule-count-" .. category]
			if definition then
				local expr = trim((definition:match("^(.-)@") or definition))
				assert(expr ~= "", "Missing plural expression: " .. locale .. "/" .. category)
				expressions[id] = expr
				keyParts[#keyParts + 1] = category .. "=" .. expr
			end
		end

		local key = table.concat(keyParts, ";")
		local set = setByRule[key]
		if not set then
			local firstRule = bounded(#data.rules, 65535, "first rule")
			for id = 1, #categories do
				local expr = expressions[id]
				if expr then
					local firstClause = bounded(#data.clauses, 65535, "first clause")
					for _, clause in ipairs(split(expr, " or ")) do
						local first = bounded(#data.relations, 65535, "first relation")
						for _, relation in ipairs(split(clause, " and ")) do
							parseRelation(trim(relation), data.ranges, data.relations)
						end
						data.clauses[#data.clauses + 1] = {
							first, bounded(#data.relations - first, 255, "relation count")
						}
					end
					data.rules[#data.rules + 1] = {
						firstClause, bounded(#data.clauses - firstClause, 255, "clause count"), id - 1
					}
				end
			end
			set = bounded(#data.ruleSets, 255, "rule-set ID")
			data.ruleSets[#data.ruleSets + 1] = {
				firstRule, bounded(#data.rules - firstRule, 255, "rule count")
			}
			setByRule[key] = set
		end
		byLocale[locale] = set
	end

	-- Preserve explicit same-language CLDR parents that select a regional rule (e.g. pt-AO).
	for _, locale in ipairs(sortedKeys(parents)) do
		local parent = parents[locale]
		if not cardinal[locale] and cardinal[parent] and parent:find("-", 1, true) and
			locale:match("^[^-]+") == parent:match("^[^-]+") then
			byLocale[locale] = byLocale[parent]
		end
	end
	for _, locale in ipairs(sortedKeys(byLocale)) do
		data.pluralLocales[#data.pluralLocales + 1] = { intern(locale), byLocale[locale] }
	end
end

local function digitsFor(system, numbering, data)
	local index = data.digitsByName[system]
	if index then
		return index
	end
	local meta = numbering[system]
	assert(meta and meta._type == "numeric", "Unsupported numbering system: " .. tostring(system))
	local digits = {}
	for _, codepoint in utf8.codes(meta._digits) do
		digits[#digits + 1] = string.format("0x%X", codepoint)
	end
	assert(#digits == 10, "Expected ten digits for numbering system: " .. system)
	index = bounded(#data.digitSets, 255, "digit set ID")
	data.digitsByName[system] = index
	data.digitSets[#data.digitSets + 1] = digits
	return index
end

local function parsePattern(pattern, locale, isPercent)
	local parts = split(pattern, ";")
	assert(#parts <= 2, "Unsupported subpatterns: " .. locale .. ": " .. pattern)
	local function parsePart(part)
		local prefix, number, suffix = part:match("^([^#0]*)([#0,.]+)([^#0]*)$")
		assert(prefix, "Missing numeric pattern: " .. locale .. ": " .. part)
		local affixes = prefix .. suffix
		local _, percentCount = affixes:gsub("%%", "")
		assert(percentCount == (isPercent and 1 or 0), "Unexpected percent sign: " .. locale .. ": " .. part)
		for _, cp in utf8.codes(affixes) do
			assert(cp == 0x20 or cp == 0x25 or cp == 0x2B or cp == 0x2D or cp == 0xA0 or
				cp == 0x61C or cp == 0x200E or cp == 0x200F or cp == 0x202F,
				"Unsupported affix: " .. locale .. ": " .. part)
		end
		return prefix, number, suffix
	end
	local prefix, number, suffix = parsePart(parts[1])
	local negativePrefix, negativeSuffix = "-" .. prefix, suffix
	if parts[2] then
		local negPrefix, negativeNumber, negSuffix = parsePart(parts[2])
		assert(negativeNumber == number, "Non-normalized negative subpattern: " .. locale .. ": " .. pattern)
		negativePrefix, negativeSuffix = negPrefix, negSuffix
	end

	local integer, fraction = number:match("^([#,]*0)%.(0*#*)$")
	if not integer then
		integer, fraction = number:match("^([#,]*0)$"), ""
	end
	assert(integer and not integer:find(",,", 1, true) and integer:sub(1, 1) ~= ",",
		"Unsupported digit pattern: " .. locale .. ": " .. pattern)
	local groups = split(integer, ",")
	local primary = #groups > 1 and #groups[#groups] or 0
	local secondary = #groups > 2 and #groups[#groups - 1] or primary
	local required = #(fraction:match("^0*"))
	return prefix, suffix, negativePrefix, negativeSuffix, primary, secondary, required, #fraction
end

local function patternFor(pattern, locale, isPercent, symbols, intern, data)
	local prefix, suffix, negativePrefix, negativeSuffix, primary, secondary, minFraction, maxFraction =
		parsePattern(pattern, locale, isPercent)
	local function localize(affix)
		return intern((affix:gsub("[-+%%]", {
			["-"] = symbols.minusSign, ["+"] = symbols.plusSign, ["%"] = symbols.percentSign
		})))
	end
	local function affixes(first, last)
		return string.format("{ %d, %d }", localize(first), localize(last))
	end
	local plusPrefix, prefixCount = negativePrefix:gsub("-", "+")
	local plusSuffix, suffixCount = negativeSuffix:gsub("-", "+")
	if prefixCount + suffixCount == 0 then
		plusPrefix, plusSuffix = prefix, suffix
	end
	return addUnique(data.patterns, data.patternIndexes, {
		affixes(prefix, suffix), affixes(negativePrefix, negativeSuffix), affixes(plusPrefix, plusSuffix),
		bounded(primary, 255, "primary grouping"), bounded(secondary, 255, "secondary grouping"),
		bounded(minFraction, 255, "minimum fraction"), bounded(maxFraction, 255, "maximum fraction")
	}, 255, "pattern ID")
end

-- Mirrors LocaleId::fallback() for normalized tags: insert a likely script, else drop a subtag.
function M.fallback(tag, scriptAliasByName)
	local language, region = tag:match("^([a-z]+)%-([A-Z][A-Z])$")
	if not language then
		language, region = tag:match("^([a-z]+)%-(%d%d%d)$")
	end
	if not language then
		language, region = tag:match("^([a-z]+)%-([A-Z][A-Z])%-")
	end
	if not language then
		language, region = tag:match("^([a-z]+)%-(%d%d%d)%-")
	end
	local script = language and scriptAliasByName[language .. "-" .. region]
	if script then
		return language .. "-" .. script .. tag:sub(#language + 1)
	end
	return tag:match("^(.*)%-[^-]+$")
end

-- Regional IDs whose likely script differs from the bare language's (zh-TW uses Hant, zh uses
-- Hans). Same-script pairs are omitted because removing the region reaches the same data.
local function buildScriptAliases(likelySubtags, intern, data)
	data.scriptAliasByName = {}
	for _, locale in ipairs(sortedKeys(likelySubtags)) do
		local language, region = locale:match("^([a-z][a-z][a-z]?)%-([A-Z][A-Z])$")
		if not language then
			language, region = locale:match("^([a-z][a-z][a-z]?)%-(%d%d%d)$")
		end
		local bare = language and likelySubtags[language]
		if bare then
			local script, territory = likelySubtags[locale]:match("^[a-z]+%-([A-Z][a-z][a-z][a-z])%-([A-Z0-9]+)$")
			local bareScript = bare:match("^[a-z]+%-([A-Z][a-z][a-z][a-z])%-")
			assert(script and bareScript, "Unsupported likely subtags: " .. locale)
			if script ~= bareScript then
				data.scriptAliasByName[locale] = script
				data.scriptAliases[#data.scriptAliases + 1] = { intern(locale), intern(script) }
			end
		end
	end
end

local function buildNumbers(package, numbering, likelySubtags, intern, data)
	local locales, count = {}, 0
	local main = path.join(package, "main")
	for _, dir in ipairs(os.matchdirs(path.join(main, "*"))) do
		local locale = path.getname(dir)
		assert(locale:match("^[a-z][a-zA-Z0-9%-]*$") and not locales[locale], "Invalid locale: " .. locale)
		locales[locale] = dir
		count = count + 1
	end
	assert(count == 657, "Expected all 657 CLDR number locales, found " .. count)

	local entries = {}
	for _, locale in ipairs(sortedKeys(locales)) do
		local source = readJson(path.join(locales[locale], "numbers.json"))
		local numbers = assert(source.main[locale] and source.main[locale].numbers, "Missing numbers: " .. locale)
		local function profile(system)
			local symbols = assert(numbers["symbols-numberSystem-" .. system], "Missing symbols: " .. locale .. "/" .. system)
			local function pattern(style, percent)
				local formats = numbers[style .. "Formats-numberSystem-" .. system]
				local standard = assert(formats and formats.standard, "Missing " .. style .. " pattern: " .. locale .. "/" .. system)
				assert(symbols.minusSign and symbols.plusSign and symbols.percentSign, "Missing affix symbols: " .. locale .. "/" .. system)
				return patternFor(standard, locale, percent, symbols, intern, data)
			end
			local grouping = bounded(tonumber(numbers.minimumGroupingDigits), 255, "minimum grouping")
			assert(grouping > 0, "Invalid minimum grouping: " .. locale)
			return addUnique(data.numberProfiles, data.profileIndexes, {
				intern(symbols.decimal), intern(symbols.group), intern(symbols.nan), intern(symbols.infinity),
				digitsFor(system, numbering, data), pattern("decimal", false), pattern("percent", true), grouping
			}, 255, "number profile ID")
		end
		local system = assert(numbers.defaultNumberingSystem, "Missing default numbering system: " .. locale)
		local default = profile(system)
		entries[locale] = { default, system == "latn" and default or profile("latn") }
	end

	-- LocaleId::fallback() inserts these scripts, so number lookups need no alias rows. Prove
	-- that the runtime walk selects the profile that the previous alias rows selected.
	local function resolve(tag)
		while tag do
			if entries[tag] then
				return entries[tag]
			end
			tag = M.fallback(tag, data.scriptAliasByName)
		end
		return entries.en
	end
	for locale, maximized in pairs(likelySubtags) do
		local language, region = locale:match("^([a-z]+)%-([A-Z][A-Z])$")
		local base, script, territory = maximized:match("^([a-z]+)%-([A-Z][a-z]+)%-([A-Z][A-Z])$")
		local alias = language and base == language and territory == region and not locales[locale] and
			(entries[maximized] or entries[base .. "-" .. script])
		if alias then
			local found = resolve(locale)
			assert(found[1] == alias[1] and found[2] == alias[2], "Script fallback changes the profile of " .. locale)
		end
	end
	for _, locale in ipairs(sortedKeys(entries)) do
		data.numberLocales[#data.numberLocales + 1] =
			{ intern(locale), entries[locale][1], entries[locale][2] }
	end
	data.defaultNumberProfile = assert(entries.en, "Missing US English profile")[1]
	return count
end

local function cppString(text)
	-- Octal escapes keep source ASCII and are independent of compiler source/execution encoding.
	return '"' .. text:gsub(".", function(c)
		local byte = c:byte()
		return (byte < 32 or byte >= 127 or c == '"' or c == "\\") and
			string.format("\\%03o", byte) or c
	end) .. '\\0"'
end

local function writeSource(directory, strings, data)
	local output = assert(io.open(path.join(directory, "localedata.generated.cpp"), "wb"))
	local function array(typeName, name, nested)
		output:write("const ", typeName, " ", name, "[] = {\n")
		for _, row in ipairs(data[name]) do
			local contents = table.concat(row, ", ")
			output:write(nested and "\t{ { " or "\t{ ", contents, nested and " } },\n" or " },\n")
		end
		output:write("};\n\n")
	end
	output:write("// Generated by projects/scripts/cldr/generate.lua (format version 2).\n",
		"// Unicode CLDR 49.0.0 data, copyright Unicode, Inc. (Unicode-3.0).\n",
		"// Input archive checksums and license: projects/scripts/cldr/README.md.\n",
		"// clang-format off\n",
		'#include "localedata.hpp"\n\nnamespace EE::System::LocaleData {\n\n',
		"const char strings[] =\n")
	for index, value in ipairs(strings) do
		output:write("\t", cppString(value), index == #strings and ";\n\n" or "\n")
	end
	array("Range", "ranges")
	array("Relation", "relations")
	array("Clause", "clauses")
	array("Rule", "rules")
	array("RuleSet", "ruleSets")
	array("PluralLocale", "pluralLocales")
	array("DigitSet", "digitSets", true)
	array("Pattern", "patterns")
	array("NumberProfile", "numberProfiles")
	array("NumberLocale", "numberLocales")
	array("ScriptAlias", "scriptAliases")
	output:write("const std::size_t pluralLocaleCount = sizeof( pluralLocales ) / sizeof( *pluralLocales );\n",
		"const std::size_t numberLocaleCount = sizeof( numberLocales ) / sizeof( *numberLocales );\n",
		"const std::size_t scriptAliasCount = sizeof( scriptAliases ) / sizeof( *scriptAliases );\n",
		"const std::uint8_t defaultNumberProfile = ", data.defaultNumberProfile, ";\n\n",
		"} // namespace EE::System::LocaleData\n")
	assert(output:close())
end

function M.generate(input, output)
	local root = path.join(input, "cldr-json")
	local core = path.join(root, "cldr-core")
	local numbers = path.join(root, "cldr-numbers-full")
	for _, package in ipairs({ core, numbers }) do
		assert(readJson(path.join(package, "package.json")).version == version,
			"Expected CLDR " .. version .. ": " .. package)
	end
	local function supplemental(name)
		local data = readJson(path.join(core, "supplemental", name .. ".json")).supplemental
		assert(data.version._cldrVersion == "49", "Unexpected CLDR supplemental version: " .. name)
		return data
	end
	local cardinal = supplemental("plurals")["plurals-type-cardinal"]
	local parents = supplemental("parentLocales").parentLocales.parentLocale
	local numbering = supplemental("numberingSystems").numberingSystems
	local likely = supplemental("likelySubtags").likelySubtags
	assert(#sortedKeys(cardinal) == 225, "Expected all 225 CLDR cardinal rule entries")
	local strings, intern = stringPool()
	local data = {
		ranges = {}, relations = {}, clauses = {}, rules = {}, ruleSets = {}, pluralLocales = {},
		digitSets = {}, digitsByName = {}, patterns = {}, patternIndexes = {},
		numberProfiles = {}, profileIndexes = {}, numberLocales = {}, scriptAliases = {}
	}
	buildScriptAliases(likely, intern, data)
	buildPlurals(cardinal, parents, intern, data)
	local numberCount = buildNumbers(numbers, numbering, likely, intern, data)
	writeSource(output, strings, data)
	print(string.format("CLDR %s: %d plural entries (%d with parents), %d rule sets",
		version, #sortedKeys(cardinal), #data.pluralLocales, #data.ruleSets))
	print(string.format("%d number locales, %d profiles, %d digit sets, %d script aliases",
		numberCount, #data.numberProfiles, #data.digitSets, #data.scriptAliases))
end

return M
