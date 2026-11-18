include("${GE}/port/lan/build-id.cmake")
find_program(LLVM_LINK llvm-link REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(generated "${CMAKE_CURRENT_BINARY_DIR}/core")
set(upstream_build "${GE}/CMakeLists.txt")
set(adapted_units lv spectrum image_bank objecthandler initunk_005450 loadobjectmodel front bondview2)
foreach(unit IN LISTS adapted_units)
  list(APPEND original_sources "${GE}/src/game/${unit}.c")
  list(APPEND adapted_sources "${generated}/${unit}.c")
endforeach()
set(pbkit_source "${SDK}/lib/pbkit/pbkit.c")
set(core_inputs "${pbkit_source}" "${CMAKE_CURRENT_LIST_DIR}/prepare.py" ${original_sources}
  "${GE}/src/game/objecthandler.h"
  "${GE}/port/src/romdata.c" "${GE}/port/src/romassets_u.s"
  "${GE}/assets/animationtable_data.h" "${upstream_build}" "${GE}/port/fast3d/gfx_pc.cpp")
set(finals_dir "${GE}/mods/the-finals")
foreach(file weapon-wheel.c wheel-layout.h wheel-render.h retro-ui.h precision-aim.h jump-pad-render.h jump-pad.h combat.h mantle.h slide.h)
  list(APPEND core_inputs "${finals_dir}/${file}")
endforeach()
list(APPEND core_inputs "${CMAKE_CURRENT_LIST_DIR}/finals_sources.py")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${core_inputs})
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/prepare.py" "${generated}" "${pbkit_source}"
  COMMAND_ERROR_IS_FATAL ANY)
file(READ "${upstream_build}" source_manifest)
string(REGEX REPLACE "#[^\n]*" "" source_manifest "${source_manifest}")
foreach(group SRC_ENGINE SRC_ASSETS SRC_LIBULTRARE_DATA)
  string(REGEX MATCH "set\\(${group}[\n\r][^)]*\\)" source_block "${source_manifest}")
  if(NOT source_block)
    message(FATAL_ERROR "Review Xbox source list after upstream changed ${group}")
  endif()
  string(REGEX MATCHALL "[\n][ \t]+(src|assets)/[A-Za-z0-9_/]+\\.c" paths "${source_block}")
  foreach(path IN LISTS paths)
    string(STRIP "${path}" path)
    list(APPEND core_units "${GE}/${path}")
  endforeach()
endforeach()
file(GLOB_RECURSE game_units CONFIGURE_DEPENDS "${GE}/src/game/*.c")
list(REMOVE_ITEM game_units ${original_sources}
  "${GE}/src/game/zlib.c" "${GE}/src/game/decompress.c")
list(REMOVE_ITEM core_units "${GE}/src/sprintf.c")
file(GLOB core_libs CONFIGURE_DEPENDS "${GE}/src/libultra/audio/*.c"
  "${GE}/src/libultrare/audio/*.c" "${GE}/src/libultra/gu/*.c")
list(REMOVE_ITEM core_libs "${GE}/src/libultra/audio/cspgetstate.c")
foreach(unit random n64stubs ucode pc_netorder rzdecomp gimgfixup utils legacycrc
    drawdistgameplay watchsettings libultra mixer audio)
  list(APPEND core_units "${GE}/port/src/${unit}.c")
endforeach()
add_library(world_ir OBJECT ${game_units} ${core_libs} ${core_units}
  ${adapted_sources} "${generated}/cart_fixups.c" core/resources.c core/sequence.c)
target_include_directories(world_ir PRIVATE "${generated}/finals" shim "${GE}" "${GE}/port/include" "${GE}/src" "${GE}/src/game"
  "${GE}/src/libultra" "${GE}/src/libultra/audio" "${SDK}/lib/sdl/SDL2/include")
target_compile_definitions(world_ir PRIVATE ${region_flags} AVOID_UB=1 PAL=0 PORT=1 _LANGUAGE_C=1
  PORT_DRAM_V1_BASE=0x81800000u PORT_DRAM_K0_BASE=0x81800000u __XBOX__ Z_SOLO)
target_compile_options(world_ir PRIVATE ${common_options} -std=gnu11 -fcommon -fms-extensions
  "-ivfsoverlay" "${generated}/model_headers.json"
  "-idirafter${GE}/include/PR" -include port/xbox/game_headers.h
  -emit-llvm -Wno-everything)
set_source_files_properties("${generated}/lv.c" PROPERTIES COMPILE_OPTIONS "-include;port/xbox/core/level.h")
set_source_files_properties("${GE}/src/game/math_asinacos.c" "${GE}/src/game/math_asinfacosf.c"
  PROPERTIES COMPILE_OPTIONS "-include;port/xbox/core/game_trig.h")
# The decompiled wrapper discards dynAllocate's result. Register the original
# typed vertex allocator directly on the Xbox calling convention.
set_source_files_properties("${GE}/src/game/initweaponanigroups.c" PROPERTIES
  COMPILE_DEFINITIONS "get_ptr_allocated_block_for_vertices=dynAllocateVertices")
set(exports "mainproc,mainThread,portKernelInit,osCreateThread,osStartThread,g_VtxBuffers,mixerInit,audioInit,getPlayerCount,xboxMountResources")
set(world_obj "${generated}/world.obj")
add_custom_command(OUTPUT "${world_obj}"
  COMMAND "${LLVM_LINK}" $<TARGET_OBJECTS:world_ir> -o "${generated}/all.bc"
  COMMAND "${LLVM_OPT}" -passes=internalize,globaldce "-internalize-public-api-list=${exports}" "${generated}/all.bc" -o "${generated}/game.bc"
  COMMAND "${CMAKE_C_COMPILER}" -O2 -c "${generated}/game.bc" -o "${world_obj}"
  DEPENDS world_ir $<TARGET_OBJECTS:world_ir> COMMAND_EXPAND_LISTS VERBATIM)
add_library(xbox_present OBJECT "${generated}/pbkit_present.c")
target_include_directories(xbox_present PRIVATE "${SDK}/lib/pbkit")
add_executable(world $<TARGET_OBJECTS:xbox_present> main.cpp "${GE}/port/lan/lan.c" core/settings.c controllers.c finals_controls.cpp "${generated}/cart_symbols.c"
  "${world_obj}" nv2a/gfx_nv2a.cpp "${generated}/gfx_pc.cpp" "${GE}/port/fast3d/gfx_cc.cpp")
target_compile_features(world PRIVATE cxx_std_20)
target_include_directories(world PRIVATE "${generated}/finals" shim "${GE}" "${GE}/port/include" "${GE}/port/fast3d"
  "${GE}/port/xbox" "${SDK}/lib/sdl/SDL2/include")
target_compile_definitions(world PRIVATE ${region_flags} AVOID_UB=1 PAL=0 PORT=1 _LANGUAGE_C=1
  PORT_DRAM_SIZE=0 Z_SOLO __XBOX__)
target_compile_options(world PRIVATE ${common_options} -include port/xbox/shim/platform.h
  -ffunction-sections -fdata-sections $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
target_include_directories(world PRIVATE "${SDK}/lib/net/lwip/src/include" "${SDK}/lib/net/nforceif/include")
target_link_libraries(world PRIVATE "${SDK}/lib/libnxdk_net.lib" "${SDK}/lib/libpbkit.lib" "${SDK}/lib/libnxdk_automount_d.lib"
  "${SDK}/lib/libzlib.lib" "${SDK}/lib/libSDL2.lib" "${SDK}/lib/nxdk_usb.lib")
target_link_options(world PRIVATE -stack:524288 -opt:ref -map -include:_automount_d_drive)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/default.xbe"
  COMMAND "${SDK}/tools/cxbe/cxbe" "-OUT:${CMAKE_CURRENT_BINARY_DIR}/default.xbe" "-TITLE:GoldenEye 007" "$<TARGET_FILE:world>"
  DEPENDS world VERBATIM)
add_custom_target(world_xbe ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/default.xbe")
