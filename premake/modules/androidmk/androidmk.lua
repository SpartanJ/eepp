--
-- androidmk.lua
-- Generator for ndk-build makefiles
-- Author : Bastien Brunnenstein
--

premake.extensions.androidmk = premake.extensions.androidmk or {}
local androidmk = premake.extensions.androidmk

include "_preload.lua"


function androidmk.slnApplicationFile(sln)
  return sln.name .."_Application.mk"
end

function androidmk.slnAndroidFile(sln)
  return sln.name .."_Android.mk"
end


function androidmk.prjFile(prj)
  return prj.name ..".mk"
end


androidmk.ldflags = {
  flags = {
    --LinkTimeOptimization = "-flto",
  }
}

androidmk.cflags = {
  flags = {
    FatalCompileWarnings = "-Werror",
    ShadowedVariables = "-Wshadow",
    UndefinedIdentifiers = "-Wundef",
    --LinkTimeOptimization = "-flto",
  },
  warnings = {
    Extra = "-Wall -Wextra",
    Off = "-w",
  }
}

androidmk.cppflags = {
  flags = {
    ["C++11"] = "-std=c++11",
    ["C++14"] = "-std=c++14",
  }
}


include "androidmk_api.lua"
include "androidmk_solution.lua"
include "androidmk_project.lua"
