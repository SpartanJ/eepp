--
-- androidmk_api.lua
-- API for Android.mk generator
-- Author : Bastien Brunnenstein
--

local api = premake.api


-- More info: https://developer.android.com/ndk/guides/abis.html
api.register {
  name = "ndkabi",
  scope = "config",
  kind = "string",
  allowed = function(value)
    local usedAbis = {}
    local validAbis = {
      "default",
      "armeabi",
      "armeabi-v7a",
      "arm64-v8a",
      "x86",
      "x86_64",
      "mips",
      "mips64",
      "all"
    }

    for _, v in ipairs(string.explode(value, ' ')) do
      if v ~= "" then
        if not table.contains(validAbis, v) then
          error("ndkabi : "..v.." is not a valid abi")
        end
        table.insert(usedAbis, v)
      end
    end

    if #usedAbis == 0 then
      error("ndkabi : Invalid parameter")
    end

    if table.contains(usedAbis, "all") then
      return "all"
    else
      return table.implode(usedAbis, '', '', ' ')
    end
  end
}

-- More info: https://developer.android.com/ndk/guides/stable_apis.html
api.register {
  name = "ndkplatform",
  scope = "config",
  kind = "string",
  allowed = {
    "default",
    "android-3",
    "android-4",
    "android-5",
    "android-8",
    "android-9",
    "android-12",
    "android-13",
    "android-14",
    "android-15",
    "android-16",
    "android-17",
    "android-18",
    "android-19",
    "android-21",
    "android-22",
    "android-23",
    "android-24",
    "android-25",
    "android-26",
    "android-27",
    "android-28",
  },
}

-- More info: https://developer.android.com/ndk/guides/cpp-support.html#runtimes
api.register {
  name = "ndkstl",
  scope = "config",
  kind = "string",
  allowed = {
    "default",
    "libstdc++",
    "gabi++_static",
    "gabi++_shared",
    "stlport_static",
    "stlport_shared",
    "gnustl_static",
    "gnustl_shared",
    "c++_static",
    "c++_shared",
  },
}

api.register {
  name = "ndktoolchainversion",
  scope = "config",
  kind = "string",
  allowed = {
    "default",
    "4.8",
    "4.9",
    "clang",
    "clang3.4",
    "clang3.5",
  },
}

-- Allows to add existing Android.mk projects
api.register {
  name = "amk_includes",
  scope = "project",
  kind = "list:file",
  tokens = true,
}

api.register {
  name = "amk_importmodules",
  scope = "project",
  kind = "list:string",
  tokens = true,
}

-- Links from includes and imports
api.register {
  name = "amk_staticlinks",
  scope = "config",
  kind = "list:string",
}

api.register {
  name = "amk_sharedlinks",
  scope = "config",
  kind = "list:string",
}
