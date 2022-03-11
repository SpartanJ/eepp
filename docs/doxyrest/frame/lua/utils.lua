--------------------------------------------------------------------------------
--
--  This file is part of the Doxyrest toolkit.
--
--  Doxyrest is distributed under the MIT license.
--  For details see accompanying license.txt file,
--  the public copy of which is also available at:
--  http://tibbo.com/downloads/archive/doxyrest/license.txt
--
--------------------------------------------------------------------------------

g_luaUtilsIncluded = true

dofile(g_frameDir .. "/../common/string.lua")
dofile(g_frameDir .. "/../common/table.lua")
dofile(g_frameDir .. "/../common/item.lua")
dofile(g_frameDir .. "/../common/doc.lua")
dofile(g_frameDir .. "/../common/toc.lua")

LANGUAGE = "lua"

if not INDEX_TITLE then
	INDEX_TITLE = "My Project Documentation"
end

if not EXTRA_PAGE_LIST then
	EXTRA_PAGE_LIST = {}
end

if not EXTERNAL_CREF_DB then
	EXTERNAL_CREF_DB = {}
end

if PRE_PARAM_LIST_SPACE then
	g_preParamSpace = " "
else
	g_preParamSpace = ""
end

if PRE_BODY_NL then
	g_preBodySpace = "\n\t"
else
	g_preBodySpace = " "
end

if not g_globalNamespace.title then
	g_globalNamespace.title = "Global Scope"
end

-------------------------------------------------------------------------------

-- formatting of function declarations

function getParamName(param)
	if param.declarationName ~= "" then
		return param.declarationName
	elseif param.definitionName ~= "" then
		return param.definitionName
	else
		return param.type.plainText
	end
end

function getParamArrayString_sl(paramArray)
	local s = "("

	local count = #paramArray
	if count > 0 then
		s = s .. getParamName(paramArray[1])

		for i = 2, count do
			s = s .. ", " .. getParamName(paramArray[i])
		end
	end

	return s .. ")"
end

function getParamArrayString_ml(paramArray, indent)
	local s = "("

	if not indent then
		indent = ""
	end

	local nl = "\n" .. indent .. "\t"

	local count = #paramArray
	if count > 0 then
		s = s .. nl .. getParamName(paramArray[1])

		for i = 2, count do
			s = s .. "," .. nl .. getParamName(paramArray[i])
		end
	end

	return s .. nl .. ")"
end

function getFunctionDeclString(func, nameTemplate, indent)
	local funcName
	if func.virtualKind == "non-virtual" then
		funcName = string.gsub(func.path, "/", ".")
	else
		local count
		funcName, count = string.gsub(func.path, "/", ":")
		if count > 1 then
			funcName = string.gsub(funcName, ":", ".", count - 1)
		end
	end

	local s = getItemLocalPrefix(func) .. "function " .. fillItemNameTemplate(nameTemplate, funcName, func.id)
	local paramString

	if ML_PARAM_LIST_COUNT_THRESHOLD and
		#func.paramArray > ML_PARAM_LIST_COUNT_THRESHOLD then
		paramString = getParamArrayString_ml(func.paramArray, indent)
	else
		paramString = getParamArrayString_sl(func.paramArray)

		if ML_PARAM_LIST_LENGTH_THRESHOLD then
			local decl = "function " .. func.name .. g_preParamSpace .. paramString
			decl = replaceRolesWithPlainText(decl)

			if string.len(decl) > ML_PARAM_LIST_LENGTH_THRESHOLD then
				paramString = getParamArrayString_ml(func.paramArray, indent)
			end
		end
	end

	return s .. g_preParamSpace .. paramString
end

function getItemLocalPrefix(item)
	-- lua 'local' translates to C/C++ 'static'

	if string.find(item.flags, "static") then
		return "local "
	else
		return ""
	end
end

-------------------------------------------------------------------------------

-- compound prep

function itemLocationFilter(item)
	return not (item.location and string.match(item.location.file, EXCLUDE_LOCATION_PATTERN))
end

function itemLocalFilter(item)
	-- lua 'local' translates to C/C++ 'static'

	return not string.find(item.flags, "static")
end

function prepareCompound(compound)
	if compound.stats then
		return compound.stats
	end

	local stats = {}

	if EXCLUDE_LOCATION_PATTERN then
		filterArray(compound.namespaceArray, itemLocationFilter)
		filterArray(compound.enumArray, itemLocationFilter)
		filterArray(compound.structArray, itemLocationFilter)
		filterArray(compound.classArray, itemLocationFilter)
		filterArray(compound.variableArray, itemLocationFilter)
		filterArray(compound.functionArray, itemLocationFilter)
	end

	if EXCLUDE_LUA_LOCALS then
		filterArray(compound.variableArray, itemLocalFilter)
		filterArray(compound.functionArray, itemLocalFilter)
	end

	stats.hasItems =
		#compound.namespaceArray ~= 0 or
		#compound.enumArray ~= 0 or
		#compound.structArray ~= 0 or
		#compound.classArray ~= 0 or
		#compound.variableArray ~= 0 or
		#compound.functionArray ~= 0

	stats.hasBriefDocumentation = not isDocumentationEmpty(compound.briefDescription)
	stats.hasDetailedDocumentation = not isDocumentationEmpty(compound.detailedDescription)
	stats.hasDocumentedVariables = prepareItemArrayDocumentation(compound.variableArray, compound)
	stats.hasDocumentedFunctions = prepareItemArrayDocumentation(compound.functionArray, compound)
	stats.hasDocumentedItems = stats.hasDocumentedVariables or stats.hasDocumentedFunctions

	if EXCLUDE_UNDOCUMENTED_ITEMS then
		filterArray(compound.variableArray, hasItemDocumentation)
		filterArray(compound.functionArray, hasItemDocumentation)
	end

	table.sort(compound.groupArray, cmpGroups)
	table.sort(compound.namespaceArray, cmpNames)
	table.sort(compound.enumArray, cmpNames)
	table.sort(compound.structArray, cmpNames)
	table.sort(compound.classArray, cmpNames)

	if SORT_GLOBAL_MEMBERS and compound.compoundKind ~= "struct" and compound.compoundKind ~= "class" then
		table.sort(compound.variableArray, cmpNames)
		table.sort(compound.functionArray, cmpNames)
	end

	compound.stats = stats

	return stats
end

function prepareEnum(enum)
	local stats = {}

	stats.hasDocumentedEnumValues = prepareItemArrayDocumentation(enum.enumValueArray)
	stats.hasBriefDocumentation = not isDocumentationEmpty(enum.briefDescription)
	stats.hasDetailedDocumentation = not isDocumentationEmpty(enum.detailedDescription)

	return stats
end

function getEnumValueString(enumValue)
	return (string.gsub(enumValue.initializer.plainText, "^%s*=%s*", "")) -- remove leading =
end

-------------------------------------------------------------------------------
