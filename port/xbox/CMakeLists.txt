cmake_minimum_required(VERSION 3.20)
project(ge007_xbox LANGUAGES C CXX)
if(NOT NXDK)
  message(FATAL_ERROR "Configure with the nxdk CMake toolchain (see DOWNSTREAM.md).")
endif()
get_filename_component(GE "../.." ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SDK "$ENV{NXDK_DIR}")
find_program(LLVM_OPT opt REQUIRED)
set(common_options -O2 -fno-strict-aliasing -funsigned-char -fwrapv -idirafter "${GE}/include")
add_executable(ge007 main.cpp title_symbols.c nv2a/gfx_nv2a.cpp
  "${GE}/port/fast3d/gfx_pc.cpp" "${GE}/port/fast3d/gfx_cc.cpp")
target_compile_features(ge007 PRIVATE cxx_std_20)
target_include_directories(ge007 PRIVATE shim "${GE}" "${GE}/port/include" "${GE}/port/fast3d")

# Import the cartridge's retail US definitions on each reconfigure. An upstream
# Makefile restructuring must fail here before it can select a different game path.
set(cartridge_makefile "${GE}/Makefile")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${cartridge_makefile}")
file(STRINGS "${cartridge_makefile}" region_rules REGEX "^[ \t]*(ifeq|endif|LCDEFS)")
set(in_us_region FALSE)
foreach(rule IN LISTS region_rules)
  string(STRIP "${rule}" rule)
  if(rule STREQUAL "ifeq ($(VERSION), US)")
    set(in_us_region TRUE)
  elseif(rule MATCHES "^(ifeq|endif)")
    set(in_us_region FALSE)
  elseif(in_us_region AND rule MATCHES "^LCDEFS[ \t]*:=[ \t]*(.+)$")
    set(us_lcdefs "${CMAKE_MATCH_1}")
    break()
  endif()
endforeach()
if(NOT us_lcdefs)
  message(FATAL_ERROR "Could not read NTSC-U LCDEFS from the cartridge Makefile.")
endif()
separate_arguments(region_flags UNIX_COMMAND "${us_lcdefs}")
list(TRANSFORM region_flags REPLACE "^-D" "")

# The shared RSP receives native i386 pointers here, with no desktop DRAM alias.
target_compile_definitions(ge007 PRIVATE ${region_flags} AVOID_UB=1 PAL=0 PORT=1 _LANGUAGE_C=1
  PORT_DRAM_SIZE=0 Z_SOLO)
target_compile_options(ge007 PRIVATE ${common_options} -include port/xbox/shim/platform.h
  -ffunction-sections -fdata-sections $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)

set(game_sources "${GE}/assets/font_dl.c")
foreach(unit IN ITEMS lookat perspective rotate mtxutil normalize)
  list(APPEND game_sources "${GE}/src/libultra/gu/${unit}.c")
endforeach()
set(game_options -std=gnu11 "-I${GE}/src" "-I${GE}/src/game" "-idirafter${GE}/include/PR"
  -include port/xbox/game_headers.h
  -Wno-microsoft-anon-tag -Wno-int-conversion -Wno-incompatible-pointer-types -Wno-implicit-function-declaration)
set_source_files_properties(${game_sources} PROPERTIES COMPILE_OPTIONS "${game_options}")

# lld-link resolves externals before COFF section GC. Trim the two full game
# translation units to the entry points this milestone actually invokes.
foreach(unit IN ITEMS title blood_animation)
  if(unit STREQUAL "title")
    set(exports "setupRarewareLogoData,retrieve_display_rareware_logo,isGunBarrelInMode2,matrixBufferRareLogo0,matrixBufferRareLogo1,matrixBufferRareLogo2")
  else()
    set(exports clear_framebuffer_black)
  endif()
  add_library(${unit}_ir OBJECT "${GE}/src/game/${unit}.c")
  target_include_directories(${unit}_ir PRIVATE $<TARGET_PROPERTY:ge007,INCLUDE_DIRECTORIES>)
  target_compile_definitions(${unit}_ir PRIVATE $<TARGET_PROPERTY:ge007,COMPILE_DEFINITIONS>)
  target_compile_options(${unit}_ir PRIVATE ${common_options} ${game_options} -emit-llvm)
  set(obj "${CMAKE_CURRENT_BINARY_DIR}/${unit}.obj")
  add_custom_command(OUTPUT "${obj}"
    COMMAND "${LLVM_OPT}" -passes=internalize,globaldce "-internalize-public-api-list=${exports}"
      $<TARGET_OBJECTS:${unit}_ir> -o "${unit}.bc"
    COMMAND "${CMAKE_C_COMPILER}" -O2 -c "${unit}.bc" -o "${obj}"
    DEPENDS ${unit}_ir $<TARGET_OBJECTS:${unit}_ir> VERBATIM)
  target_sources(ge007 PRIVATE "${obj}")
endforeach()
target_sources(ge007 PRIVATE ${game_sources})

# Emit COFF's leading underscore on the existing region-specific cart symbols.
set(rom_symbols "${GE}/port/src/romassets_u.s")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${rom_symbols}")
file(STRINGS "${rom_symbols}" symbols REGEX "^\\.set _rarewarelogoSegment")
set(cart_source "")
foreach(symbol IN LISTS symbols)
  string(REGEX REPLACE "^\\.set ([A-Za-z_0-9]+), (0x[0-9A-Fa-f]+).*"
    "__asm__(\".globl _\\1\\\\n.set _\\1, \\2\");" definition "${symbol}")
  string(APPEND cart_source "${definition}\n")
endforeach()
file(CONFIGURE OUTPUT cart_symbols.c CONTENT "${cart_source}" @ONLY)
target_sources(ge007 PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/cart_symbols.c")
target_link_libraries(ge007 PRIVATE "${SDK}/lib/libpbkit.lib"
  "${SDK}/lib/libnxdk_automount_d.lib" "${SDK}/lib/libzlib.lib")
target_link_options(ge007 PRIVATE -stack:524288 -opt:ref -map -include:_automount_d_drive)
add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/default.xbe"
  COMMAND "${SDK}/tools/cxbe/cxbe" "-OUT:${CMAKE_CURRENT_BINARY_DIR}/default.xbe"
    "-TITLE:GoldenEye 007" "$<TARGET_FILE:ge007>"
  DEPENDS ge007 VERBATIM)
add_custom_target(xbe ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/default.xbe")
