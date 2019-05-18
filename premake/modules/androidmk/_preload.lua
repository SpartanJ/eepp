--
-- _preload.lua
-- Generator for ndk-build makefiles
-- Author : Bastien Brunnenstein
--

premake.extensions.androidmk = premake.extensions.androidmk or {}
local androidmk = premake.extensions.androidmk
local make = premake.make


androidmk.CONFIG_OPTION = "PM5_CONFIG"


newaction {
  trigger         = "androidmk",
  shortname       = "Android.mk",
  description     = "Generate Android.mk files for Android NDK",

  valid_kinds     = {
    premake.STATICLIB,
    premake.SHAREDLIB,
  },

  valid_languages = { "C", "C++" },


  onSolution = function(sln)
--    premake.escaper(make.esc)
    premake.generate(sln, androidmk.slnApplicationFile(sln), androidmk.generate_applicationmk)
    premake.generate(sln, androidmk.slnAndroidFile(sln), androidmk.generate_androidmk)
  end,

  onProject = function(prj)
--    premake.escaper(make.esc)
    premake.generate(prj, androidmk.prjFile(prj), androidmk.generate_projectmk)
  end,

  onCleanSolution = function(sln)
    premake.clean.file(sln, androidmk.slnApplicationFile(sln))
    premake.clean.file(sln, androidmk.slnAndroidFile(sln))
  end,

  onCleanProject = function(prj)
    premake.clean.file(prj, androidmk.prjFile(prj))
  end
}
