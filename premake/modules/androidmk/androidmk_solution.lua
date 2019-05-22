--
-- androidmk_solution.lua
-- Generator for Application.mk and Android.mk solution files
-- Author : Bastien Brunnenstein
--

local androidmk = premake.extensions.androidmk
local solution = premake.solution
local project = premake.project
local make = premake.make

local p = premake


function androidmk.generate_applicationmk(sln)
  premake.eol("\n")

  androidmk.slnBuildScript(sln)

  p.w('PM5_HELP := true')

  for cfg in solution.eachconfig(sln) do
    p.w('')
    p.x('ifeq ($(%s),%s)', androidmk.CONFIG_OPTION, cfg.shortname)

    androidmk.slnOptim(sln, cfg)
    androidmk.slnAbi(sln, cfg)
    androidmk.slnPlatform(sln, cfg)
    androidmk.slnStl(sln, cfg)
    androidmk.slnToolchainVersion(sln, cfg)

    p.w('  PM5_HELP := false')
    p.w('endif')
  end

  androidmk.slnPremakeHelp(sln)
end

function androidmk.generate_androidmk(sln)
  premake.eol("\n")
  local curpath = 'SOLUTION_'..sln.name:upper()..'_PATH'
  p.w('%s := $(call my-dir)', curpath)
  p.w('')

  for prj in solution.eachproject(sln) do
    local prjpath = premake.filename(prj, androidmk.prjFile(prj))
    local prjrelpath = path.getrelative(sln.location, prjpath)
    p.x('include $(%s)/%s', curpath, prjrelpath)
  end

  for cfg in solution.eachconfig(sln) do
    local existingmklist = {}
    local moduleslist = {}

    -- Agregate existing Android.mk files for each project per configuration
    for prj in solution.eachproject(sln) do
      for prjcfg in project.eachconfig(prj) do
        if prjcfg.shortname == cfg.shortname then
          for _, mkpath in ipairs(prj.amk_includes) do
            local mkrelpath = path.getrelative(sln.location, mkpath)
            if not table.contains(existingmklist, mkrelpath) then
              table.insert(existingmklist, mkrelpath)
            end
          end
          for _, mod in ipairs(prj.amk_importmodules) do
            if not table.contains(moduleslist, mod) then
              table.insert(moduleslist, mod)
            end
          end
        end
      end
    end

    if #existingmklist > 0 or #moduleslist > 0 then
      p.w('')
      p.x('ifeq ($(%s),%s)', androidmk.CONFIG_OPTION, cfg.shortname)
      for _, mkpath in ipairs(existingmklist) do
        p.x('  include $(%s)/%s', curpath, mkpath)
      end
      for _, mod in ipairs(moduleslist) do
        p.x('  $(call import-module,%s)', mod)
      end
      p.w('endif')
    end
  end
end


function androidmk.slnBuildScript(sln)
  p.x('APP_BUILD_SCRIPT := $(call my-dir)/%s', androidmk.slnAndroidFile(sln))
end

function androidmk.slnPremakeHelp(sln)
  p.w('')
  p.w('ifeq ($(PM5_HELP),true)')

  p.w('  $(info )')
  p.w('  $(info Usage:)')
  p.w('  $(info $()  ndk-build %s=<config>)', androidmk.CONFIG_OPTION)
  p.w('  $(info )')
  p.w('  $(info CONFIGURATIONS:)')
  for cfg in solution.eachconfig(sln) do
    p.w('  $(info $()  %s)', cfg.shortname)
  end

  p.w('  $(info )')
  p.w('  $(info For more ndk-build options, see https://developer.android.com/ndk/guides/ndk-build.html)')
  p.w('  $(info )')

  p.w('  $(error Set %s and try again)', androidmk.CONFIG_OPTION)
  p.w('endif')
end


-- Function to make sure that an option in a given config is the same for every project
-- Additionnaly, replace "default" with "nil"
local function agregateOption(sln, cfg, option)
  local first = true
  local val
  for prj in solution.eachproject(sln) do
    for prjcfg in project.eachconfig(prj) do
      if prjcfg.shortname == cfg.shortname then
        if first then
          first = false
          val = prjcfg[option]
        else
          if prjcfg[option] ~= val then
            error("Value for "..option.." must be the same on every project for configuration "..cfg.longname.." in solution "..sln.name)
          end
        end
      end
    end
  end
  if val == "default" then
    return nil
  end
  return val
end

function androidmk.slnOptim(sln, cfg)
  local optim = agregateOption(sln, cfg, "optimize")
  if optim == p.OFF or optim == "Debug" then
    p.w('  APP_OPTIM := debug')
  elseif optim ~= nil then
    p.w('  APP_OPTIM := release')
  end
end

function androidmk.slnAbi(sln, cfg)
  local abi = agregateOption(sln, cfg, "ndkabi")
  if abi then
    p.w('  APP_ABI := %s', abi)
  end
end

function androidmk.slnPlatform(sln, cfg)
  local plat = agregateOption(sln, cfg, "ndkplatform")
  if plat then
    p.w('  APP_PLATFORM := %s', plat)
  end
end

function androidmk.slnStl(sln, cfg)
  local stl = agregateOption(sln, cfg, "ndkstl")
  if stl then
    p.w('  APP_STL := %s', stl)
  end
end

function androidmk.slnToolchainVersion(sln, cfg)
  local toolchain = agregateOption(sln, cfg, "ndktoolchainversion")
  if toolchain then
    p.w('  NDK_TOOLCHAIN_VERSION := %s', toolchain)
  end
end
