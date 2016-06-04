-- GENie script file

solution "dimapp"
  configurations { "Debug", "Release" }
  platforms { "x64" }
  language "C++"
  flags {
    "ExtraWarnings",
    "FatalWarnings",
    "EnableMinimalRebuild",
    "StaticRuntime",
    "Symbols"
  }
  includedirs { 
    "include",
    "vendor/sodium/include" 
  }
  links { 
    "vendor/sodium/$(Platform)/$(Configuration)/$(PlatformToolset)/static/libsodium" 
  }
  targetdir "bin"
  pchheader "pch.h"


configuration "Debug"
  defines { "_DEBUG" }

configuration "Release"
  defines { "NDEBUG" }
  libdirs { "vendor/sodium/x64/Release" }
  flags { 
    "Optimize"
  }

print "hello"

project "dimapp"
  kind "StaticLib"
  location "projects"
  vpaths { 
    ["include"] = { "include/*.h", "include/dim/*.h" },
    ["config"] = "include/dim/compiler/*"
  }
  files { 
    "include/**",
    "src/*"
  }
  pchsource "src/pch.cpp"

project "dimapp-win8"
  kind "StaticLib"
  location "projects"
  vpaths { ["*"] = "src/win8" }
  files { 
    "src/win8/*"
  }
  pchsource "src/win8/pch.cpp"

group "-meta"
project "-meta"
  kind "StaticLib"
  location "projects"
  files { 
    ".clang-format",
    "uncrustify.cfg",
    "*.lua",
    "*.yml",
    "LICENSE",
    "README.md"
  }
  links { "dimapp", "dimapp-win8" }


-- tests
group "tests"
project "cmdline"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/cmdline" }
  files { "tests/cmdline/**" }
  pchsource "tests/cmdline/pch.cpp"
  links { "dimapp", "dimapp-win8" }

project "hpack"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/hpack" }
  files { "tests/hpack/**" }
  pchsource "tests/hpack/pch.cpp"
  links { "dimapp", "dimapp-win8" }

project "http"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/http" }
  files { "tests/http/**" }
  pchsource "tests/http/pch.cpp"
  links { "dimapp", "dimapp-win8" }

project "tls"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/tls" }
  files { "tests/tls/**" }
  pchsource "tests/tls/pch.cpp"
  links { "dimapp", "dimapp-win8" }


-- tools
group "tools"
project "pargen"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tools/pargen" }
  files { "tools/pargen/**" }
  pchsource "tools/pargen/pch.cpp"
  links { "dimapp", "dimapp-win8" }

project "tnet"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tools/tnet" }
  files { "tools/tnet/**" }
  pchsource "tools/tnet/pch.cpp"
  links { "dimapp", "dimapp-win8" }
