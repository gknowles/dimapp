-- GENie script file

solution "dimapp"
  configurations { "Debug", "Release" }
  platforms { "x64" }
  language "C++"
  flags {
    "ExtraWarnings",
    "FatalWarnings",
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
  debugdir "$(TargetDir)"
  buildoptions { "/std:c++latest" }


configuration "Debug"
  defines { "_DEBUG" }

configuration "Release"
  defines { "NDEBUG" }
  libdirs { "vendor/sodium/x64/Release" }
  flags { 
    "Optimize"
  }

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

project "dimapp-cli"
  kind "StaticLib"
  location "projects"
  vpaths { ["*"] = "src/cli" }
  files { 
    "src/cli/*"
  }
  pchsource "src/cli/pch.cpp"

project "dimapp-win8"
  kind "StaticLib"
  location "projects"
  vpaths { ["*"] = "src/win8" }
  files { 
    "src/win8/*"
  }
  pchsource "src/win8/pch.cpp"

project "dimapp-xml"
  kind "StaticLib"
  location "projects"
  vpaths { ["*"] = "src/xml" }
  files { 
    "src/xml/*"
  }
  pchsource "src/xml/pch.cpp"

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
  links { "dimapp", "dimapp-win8", "dimapp-cli" }


-- tests
group "tests"
project "hpack"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/hpack" }
  files { "tests/hpack/**" }
  pchsource "tests/hpack/pch.cpp"
  links { "dimapp", "dimapp-win8", "dimapp-cli" }

project "http"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/http" }
  files { "tests/http/**" }
  pchsource "tests/http/pch.cpp"
  links { "dimapp", "dimapp-win8", "dimapp-cli" }

project "tls"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/tls" }
  files { "tests/tls/**" }
  pchsource "tests/tls/pch.cpp"
  links { "dimapp", "dimapp-win8", "dimapp-cli" }

project "xml"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tests/xml" }
  files { "tests/xml/**" }
  pchsource "tests/xml/pch.cpp"
  links { "dimapp", "dimapp-win8", "dimapp-cli", "dimapp-xml" }


-- tools
group "tools"
project "pargen"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tools/pargen" }
  files { "tools/pargen/**" }
  pchsource "tools/pargen/pch.cpp"
  links { "dimapp", "dimapp-win8", "dimapp-cli" }

project "tnet"
  kind "ConsoleApp"
  location "projects"
  vpaths { ["*"] = "tools/tnet" }
  files { "tools/tnet/**" }
  pchsource "tools/tnet/pch.cpp"
  links { "dimapp", "dimapp-win8", "dimapp-cli" }

