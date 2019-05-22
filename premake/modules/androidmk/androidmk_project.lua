--
-- androidmk_project.lua
-- Generator for Android.mk project files
-- Author : Bastien Brunnenstein
--

local androidmk = premake.extensions.androidmk
local make = premake.make
local project = premake.project

local p = premake


function androidmk.generate_projectmk(prj)
  premake.eol("\n")

  androidmk.prjHeader(prj)

  -- Prepare the list of files
  local rootFiles, cfgFiles = androidmk.prepareSrcFiles(prj)
  androidmk.prjSrcFiles(rootFiles)

  for cfg in project.eachconfig(prj) do
    p.w('')
    p.x('ifeq ($(%s),%s)', androidmk.CONFIG_OPTION, cfg.shortname)

    androidmk.prjIncludes(prj, cfg)
    androidmk.prjCppFeatures(prj, cfg)
    androidmk.prjCfgSrcFiles(cfgFiles[cfg])
    androidmk.prjDependencies(prj, cfg)
    androidmk.prjLdFlags(prj, cfg)
    androidmk.prjCFlags(prj, cfg)

    -- Always last
    androidmk.prjKind(prj, cfg)

    p.w('endif')
  end
end


function androidmk.prjHeader(prj)
  p.w('LOCAL_PATH := $(call my-dir)')
  p.w('include $(CLEAR_VARS)')
  p.w('LOCAL_MODULE := %s', prj.name)
  if prj.targetname then
    p.w('LOCAL_MODULE_FILENAME := %s', prj.targetname)
  end
  p.w('')
end

function androidmk.prjKind(prj, cfg)
  if cfg.kind == premake.STATICLIB  then
    p.w('  include $(BUILD_STATIC_LIBRARY)')

  else -- cfg.kind == premake.SHAREDLIB
    p.w('  include $(BUILD_SHARED_LIBRARY)')

  end
end


function androidmk.prjIncludes(prj, cfg)
  if cfg.includedirs then
    p.w('  LOCAL_C_INCLUDES := %s', 
      table.implode(
        table.translate(
          table.translate(cfg.includedirs,
            function(d)
              return "$(LOCAL_PATH)/"..project.getrelative(prj, d)
            end)
        , p.esc)
      , '', '', ' '))
  end
end

function androidmk.prjCppFeatures(prj, cfg)
  local features = {}

  if cfg.rtti == p.ON then
    table.insert(features, "rtti")
  end

  if cfg.exceptionhandling == p.ON then
    table.insert(features, "exceptions")
  end

  if #features > 0 then
    p.w('  LOCAL_CPP_FEATURES := %s', table.implode(features, '', '', ' '))
  end
end

function androidmk.prepareSrcFiles(prj)
  local root = {}
  local configs = {}
  for cfg in project.eachconfig(prj) do
    configs[cfg] = {}
  end

  local tr = project.getsourcetree(prj)
  premake.tree.traverse(tr, {
    onleaf = function(node, depth)
      -- Figure out what configurations contain this file
      local incfg = {}
      local inall = true
      for cfg in project.eachconfig(prj) do
        local filecfg = premake.fileconfig.getconfig(node, cfg)
        if filecfg and not filecfg.flags.ExcludeFromBuild then
          incfg[cfg] = filecfg
        else
          inall = false
        end
      end

      -- Allow .arm, .neon and .arm.neon files
      if not path.iscppfile(node.abspath) and
          path.getextension(node.abspath) ~= "arm" and
          path.getextension(node.abspath) ~= "neon" then
        return
      end

      local filename = project.getrelative(prj, node.abspath)

      -- If this file exists in all configurations, write it to
      -- the project's list of files, else add to specific cfgs
      if inall then
        table.insert(root, filename)
      else
        for cfg in project.eachconfig(prj) do
          if incfg[cfg] then
            table.insert(configs[cfg], filename)
          end
        end
      end

    end
  })

  return root, configs
end

function androidmk.prjSrcFiles(files)
  p.w('LOCAL_SRC_FILES := %s', table.implode(table.translate(files, p.esc), '', '', ' '))
end

function androidmk.prjCfgSrcFiles(files)
  if #files > 0 then
    p.w('  LOCAL_SRC_FILES += %s', table.implode(table.translate(files, p.esc), '', '', ' '))
  end
end

function androidmk.prjDependencies(prj, cfg)
  local staticdeps = {}
  local shareddeps = {}

  local dependencies = premake.config.getlinks(cfg, "dependencies", "object")
  for _, dep in ipairs(dependencies) do
    if dep.kind == premake.STATICLIB then
      table.insert(staticdeps, dep.filename)
    else
      table.insert(shareddeps, dep.filename)
    end
  end

  for _, v in ipairs(cfg.amk_staticlinks) do
    table.insert(staticdeps, v)
  end
  for _, v in ipairs(cfg.amk_sharedlinks) do
    table.insert(shareddeps, v)
  end

  if #staticdeps > 0 then
    p.w('  LOCAL_STATIC_LIBRARIES := %s', table.implode(staticdeps, '', '', ' '))
  end

  if #shareddeps > 0 then
    p.w('  LOCAL_SHARED_LIBRARIES := %s', table.implode(shareddeps, '', '', ' '))
  end
end

function androidmk.prjLdFlags(prj, cfg)
  -- LDLIBS
  local flags = {}

  for _, dir in ipairs(premake.config.getlinks(cfg, "system", "directory")) do
    table.insert(flags, '-L' .. premake.quoted(dir))
  end

  for _, name in ipairs(premake.config.getlinks(cfg, "system", "basename")) do
    table.insert(flags, "-l" .. name)
  end

  if #flags > 0 then
    p.w('  LOCAL_LDLIBS := %s', table.implode(table.translate(flags, p.esc), '', '', ' '))
  end

  --LDFLAGS
  flags = premake.config.mapFlags(cfg, androidmk.ldflags)

  for _, opt in ipairs(cfg.linkoptions) do
    table.insert(flags, opt)
  end

  if #flags > 0 then
    p.w('  LOCAL_LDFLAGS := %s', table.implode(table.translate(flags, p.esc), '', '', ' '))
  end
end

function androidmk.prjCFlags(prj, cfg)
  local flags = premake.config.mapFlags(cfg, androidmk.cflags)

  -- Defines
  for _, def in ipairs(cfg.defines) do
    table.insert(flags, '-D' .. def)
  end

  -- Warnings
  for _, enable in ipairs(cfg.enablewarnings) do
    table.insert(flags, '-W' .. enable)
  end
  for _, disable in ipairs(cfg.disablewarnings) do
    table.insert(flags, '-Wno-' .. disable)
  end
  for _, fatal in ipairs(cfg.fatalwarnings) do
    table.insert(flags, '-Werror=' .. fatal)
  end

  -- Build options
  for _, opt in ipairs(cfg.buildoptions) do
    table.insert(flags, opt)
  end

  if #flags > 0 then
    p.w('  LOCAL_CFLAGS := %s', table.implode(table.translate(flags, p.esc), '', '', ' '))
  end


  local cppflags = premake.config.mapFlags(cfg, androidmk.cppflags)

  if #cppflags > 0 then
    p.w('  LOCAL_CPPFLAGS := %s', table.implode(table.translate(cppflags, p.esc), '', '', ' '))
  end
end
