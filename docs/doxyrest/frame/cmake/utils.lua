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

g_cmakeUtilsIncluded = true

dofile(g_frameDir .. "/../common/string.lua")
dofile(g_frameDir .. "/../common/table.lua")
dofile(g_frameDir .. "/../common/item.lua")
dofile(g_frameDir .. "/../common/doc.lua")
dofile(g_frameDir .. "/../common/toc.lua")

LANGUAGE = "cmake"

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
	local s = ""

	local count = #paramArray
	if count > 0 then
		s = " " .. getParamName(paramArray[1])

		for i = 2, count do
			s = s .. " " .. getParamName(paramArray[i])
		end
	end

	return s .. ")"
end

function getParamArrayString_ml(paramArray, indent)
	if not indent then
		indent = ""
	end

	local s = ""
	local nl = "\n" .. indent .. "\t"

	local count = #paramArray
	if count > 0 then
		s = nl .. getParamName(paramArray[1])

		for i = 2, count do
			s = s .. nl .. getParamName(paramArray[i])
		end
	end

	return s .. nl .. ")"
end

function getFunctionDeclString(func, nameTemplate, indent)
	local functionKind
	if func.memberKind == "define" then
		functionKind = "macro"
	else
		functionKind = "function"
	end

	local s = functionKind .. g_preParamSpace .. "("
	local name = fillItemNameTemplate(nameTemplate, func.name, func.id)
	local paramString

	if ML_PARAM_LIST_COUNT_THRESHOLD and
		#func.paramArray > ML_PARAM_LIST_COUNT_THRESHOLD then
		s = s .. "\n" .. indent
		paramString = getParamArrayString_ml(func.paramArray, indent)
	else
		paramString = getParamArrayString_sl(func.paramArray)

		if ML_PARAM_LIST_LENGTH_THRESHOLD then
			local decl = functionKind .. g_preParamSpace .. func.name .. paramString
			decl = replaceRolesWithPlainText(decl)

			if string.len(decl) > ML_PARAM_LIST_LENGTH_THRESHOLD then
				s = s .. "\n" .. indent
				paramString = getParamArrayString_ml(func.paramArray, indent)
			end
		end
	end

	return s .. name .. paramString
end

-------------------------------------------------------------------------------

-- compound prep

function itemLocationFilter(item)
	return not (item.location and string.match(item.location.file, EXCLUDE_LOCATION_PATTERN))
end

function prepareCompound(compound)
	if compound.stats then
		return compound.stats
	end

	local stats = {}

	if EXCLUDE_LOCATION_PATTERN then
		filterArray(compound.variableArray, itemLocationFilter)
		filterArray(compound.functionArray, itemLocationFilter)
		filterArray(compound.defineArray, itemLocationFilter)
	end

	stats.hasItems =
		#compound.variableArray ~= 0 or
		#compound.functionArray ~= 0 or
		#compound.defineArray ~= 0

	stats.hasBriefDocumentation = not isDocumentationEmpty(compound.briefDescription)
	stats.hasDetailedDocumentation = not isDocumentationEmpty(compound.detailedDescription)
	stats.hasDocumentedVariables = prepareItemArrayDocumentation(compound.variableArray, compound)
	stats.hasDocumentedFunctions = prepareItemArrayDocumentation(compound.functionArray, compound)
	stats.hasDocumentedDefines = prepareItemArrayDocumentation(compound.defineArray, compound)
	stats.hasDocumentedItems = stats.hasDocumentedVariables or stats.hasDocumentedFunctions

	if EXCLUDE_UNDOCUMENTED_ITEMS then
		filterArray(compound.variableArray, hasItemDocumentation)
		filterArray(compound.functionArray, hasItemDocumentation)
		filterArray(compound.defineArray, hasItemDocumentation)
	end

	table.sort(compound.groupArray, cmpGroups)

	if SORT_GLOBAL_MEMBERS then
		table.sort(compound.variableArray, cmpNames)
		table.sort(compound.functionArray, cmpNames)
		table.sort(compound.defineArray, cmpNames)
	end

	compound.stats = stats

	return stats
end

-------------------------------------------------------------------------------
