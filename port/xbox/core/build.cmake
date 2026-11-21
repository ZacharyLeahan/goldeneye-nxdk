include("${GE}/port/lan/build-id.cmake")
find_program(LLVM_LINK llvm-link REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(generated "${CMAKE_CURRENT_BINARY_DIR}/core")
set(upstream_build "${GE}/CMakeLists.txt")
set(adapted_units radar chraction bondview_r lv bg model spectrum image_bank objecthandler initunk_005450 loadobjectmodel front bondview2 gun propobj chrprop mpmenu)
foreach(unit IN LISTS adapted_units)
  list(APPEND original_sources "${GE}/src/game/${unit}.c")
  list(APPEND adapted_sources "${generated}/${unit}.c")
endforeach()
list(APPEND adapted_sources "${generated}/boss.c")
set(pbkit_source "${SDK}/lib/pbkit/pbkit.c")
set(ohci_source "${SDK}/lib/usb/libusbohci/src_core/ohci.c")
set(core_inputs "${pbkit_source}" "${ohci_source}" "${CMAKE_CURRENT_LIST_DIR}/prepare.py" "${GE}/src/music.c" ${original_sources}
  "${GE}/src/boss.c" "${GE}/port/lan/mode_menu.py" "${GE}/port/lan/adapt.py" "${GE}/port/lan/tests/egypt_camera.py" "${GE}/src/game/objecthandler.h"
  "${GE}/port/src/romdata.c" "${GE}/port/src/romassets_u.s"
  "${GE}/assets/animationtable_data.h" "${upstream_build}" "${GE}/port/fast3d/gfx_pc.cpp")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${core_inputs})
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_LIST_DIR}/prepare.py" "${generated}" "${pbkit_source}" "${ohci_source}"
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
set(GE_SCHED_ROOT "${GE}")
include("${GE}/port/lan/scheduler.cmake")
list(REMOVE_ITEM core_units "${scheduler_source}")
list(APPEND core_units "${port_scheduler}")
list(REMOVE_ITEM core_units "${GE}/src/music.c")
list(APPEND core_units "${generated}/music.c")
file(GLOB_RECURSE game_units CONFIGURE_DEPENDS "${GE}/src/game/*.c")
list(REMOVE_ITEM game_units ${original_sources}
  "${GE}/src/game/zlib.c" "${GE}/src/game/decompress.c")
list(REMOVE_ITEM core_units "${GE}/src/sprintf.c" "${GE}/src/boss.c")
file(GLOB core_libs CONFIGURE_DEPENDS "${GE}/src/libultra/audio/*.c"
  "${GE}/src/libultrare/audio/*.c" "${GE}/src/libultra/gu/*.c")
list(REMOVE_ITEM core_libs "${GE}/src/libultra/audio/cspgetstate.c")
foreach(unit random n64stubs ucode pc_netorder rzdecomp gimgfixup utils legacycrc
    drawdistgameplay watchsettings libultra mixer audio)
  list(APPEND core_units "${GE}/port/src/${unit}.c")
endforeach()
include("${GE}/port/lan/simulants.cmake")
add_library(world_ir OBJECT ${bot_sources} ${game_units} ${core_libs} ${core_units}
  ${adapted_sources} "${generated}/cart_fixups.c" core/resources.c core/sequence.c)
target_include_directories(world_ir PRIVATE shim "${GE}" "${GE}/port/include" "${GE}/src" "${GE}/src/game"
  "${GE}/src/libultra" "${GE}/src/libultra/audio" "${SDK}/lib/sdl/SDL2/include")
target_compile_definitions(world_ir PRIVATE ${region_flags} AVOID_UB=1 PAL=0 PORT=1 _LANGUAGE_C=1
  PORT_DRAM_V1_BASE=0x81800000u PORT_DRAM_K0_BASE=0x81800000u __XBOX__ Z_SOLO)
target_compile_options(world_ir PRIVATE ${common_options} -std=gnu11 -fcommon -fms-extensions
  "-ivfsoverlay" "${generated}/model_headers.json"
  "SHELL:-idirafter \"${GE}/include/PR\"" -include port/xbox/game_headers.h
  -emit-llvm -Wno-everything)
set_source_files_properties("${generated}/lv.c" PROPERTIES COMPILE_OPTIONS "-include;port/xbox/core/level.h")
set_source_files_properties("${GE}/src/game/math_asinacos.c" "${GE}/src/game/math_asinfacosf.c"
  PROPERTIES COMPILE_OPTIONS "-include;port/xbox/core/game_trig.h")
# The decompiled wrapper discards dynAllocate's result. Register the original
# typed vertex allocator directly on the Xbox calling convention.
set_source_files_properties("${GE}/src/game/initweaponanigroups.c" PROPERTIES
  COMPILE_DEFINITIONS "get_ptr_allocated_block_for_vertices=dynAllocateVertices")
set(exports "mainproc,mainThread,portKernelInit,osCreateThread,osStartThread,g_VtxBuffers,mixerInit,audioInit,getPlayerCount,xboxMountResources,xboxProfileGame,xboxProfileKills")
set(world_obj "${generated}/world.obj")
file(GENERATE OUTPUT "${generated}/ir-objects.txt"
  CONTENT "$<JOIN:$<TARGET_OBJECTS:world_ir>,\n>\n")
add_custom_command(OUTPUT "${world_obj}"
  COMMAND "${CMAKE_COMMAND}" "-Dlinker=${LLVM_LINK}" -P "${CMAKE_CURRENT_LIST_DIR}/link-ir.cmake"
  COMMAND "${LLVM_OPT}" -passes=internalize,globaldce "-internalize-public-api-list=${exports}" "${generated}/all.bc" -o "${generated}/game.bc"
  COMMAND "${CMAKE_C_COMPILER}" -O2 -c "${generated}/game.bc" -o "${world_obj}"
  DEPENDS world_ir $<TARGET_OBJECTS:world_ir> "${CMAKE_CURRENT_LIST_DIR}/link-ir.cmake" VERBATIM)
add_library(xbox_present OBJECT "${generated}/pbkit_present.c")
target_include_directories(xbox_present PRIVATE "${SDK}/lib/pbkit" "${GE}")
target_compile_definitions(xbox_present PRIVATE GE_PBKIT_TRACE=1)
add_executable(world $<TARGET_OBJECTS:xbox_present> main.cpp "${GE}/port/lan/lan.c" "${GE}/port/lan/frame_trace.c" core/settings.c controllers.c "${GE}/port/controls/finals_controls.cpp" "${generated}/cart_symbols.c"
  "${world_obj}" nv2a/gfx_nv2a.cpp "${generated}/gfx_pc.cpp" "${GE}/port/fast3d/gfx_cc.cpp")
target_compile_features(world PRIVATE cxx_std_20)
target_sources(world PRIVATE "${generated}/ohci_retry.c" perf_hud.cpp)
set_source_files_properties("${generated}/ohci_retry.c" PROPERTIES
  INCLUDE_DIRECTORIES "${SDK}/lib/usb/libusbohci/inc;${SDK}/lib/usb/libusbohci_xbox"
  COMPILE_DEFINITIONS "USBH_USE_EXTERNAL_CONFIG=\"usbh_config_xbox.h\"")
target_include_directories(world PRIVATE shim "${GE}" "${GE}/port/include" "${GE}/port/fast3d"
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
  COMMAND "${SDK}/tools/cxbe/cxbe" "-OUT:default.xbe" "-TITLE:GoldenEye 007" "$<TARGET_FILE:world>"
  DEPENDS world VERBATIM)
add_custom_target(world_xbe ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/default.xbe")
