# CMakeLists.txt - dim app source

cmake_minimum_required (VERSION 3.6)
project (dimapp)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ../bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ../bin)

function(add_srclib_project)
    get_filename_component(dirname "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    set(prjname "dimapp-${dirname}")
    project(${prjname})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")

    if(MSVC)
        set_property(SOURCE "pch.cpp" APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")
    endif()

    file(GLOB_RECURSE sources LIST_DIRECTORIES false *.h *.cpp)
    update_deps_file("${sources}")

    add_library(${prjname} ${sources})
    source_group("" FILES ${sources})
    write_user_file(${prjname})
endfunction()


if(MSVC)
    set_property(SOURCE "pch.cpp" APPEND PROPERTY COMPILE_FLAGS "/Ycpch.h")
endif()

file(GLOB_RECURSE global_srcs LIST_DIRECTORIES false ../include/*)
file(GLOB local_srcs LIST_DIRECTORIES false *.h *.cpp)
set(sources ${global_srcs} ${local_srcs})
file(GLOB allnames *)
foreach(var ${allnames})
    if(IS_DIRECTORY ${var})
        list(APPEND subdirs ${var})
        add_subdirectory(${var})
    endif()
endforeach()
update_deps_file(${sources} ${subdirs})

add_library(dimapp ${sources})
source_group("" FILES ${sources})
source_group("compiler" FILES ../include/dim/compiler/visualc.h)
write_user_file(dimapp)

# Create modifed config.h to reflect build defines
set(cfgname "include/dim/config.h")
file(READ "../${cfgname}" content)
foreach(val ${config_defines})
    string(REGEX REPLACE 
        "\n//#define ${val}" "\n#define ${val}" 
        content "${content}")
endforeach()
update_file("${CMAKE_CURRENT_BINARY_DIR}/${cfgname}" "${content}")

# Special handling required to get PDBs of static libraries installed
# I would hope that there's a better way to do this, but I don't know what 
# it may be...
function(install_static_archive_pdb tgt dstdir)
    foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
        install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${tgt}.dir/${cfg}/${tgt}.pdb" 
            DESTINATION ${dstdir} 
            CONFIGURATIONS ${cfg})
    endforeach()
endfunction()

install(DIRECTORY ../include DESTINATION .)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${cfgname}" 
    DESTINATION include/dim)
if(BUILD_SHARED_LIBS)
    install(TARGETS dimapp
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
    install(FILES "$<TARGET_PDB_FILE:dimapp>" DESTINATION bin)
else()
    install(TARGETS dimapp
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
    install_static_archive_pdb(dimapp lib)
endif()
