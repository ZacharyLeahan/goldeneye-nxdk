find_package(Python3 COMPONENTS Interpreter REQUIRED)
set(bot_root "${CMAKE_CURRENT_LIST_DIR}/../..")
set(bot_nav "${CMAKE_CURRENT_BINARY_DIR}/simulant_facility_nav.h")
add_custom_command(OUTPUT "${bot_nav}"
  COMMAND "${Python3_EXECUTABLE}" "${bot_root}/tools_pc/gen_simulant_nav.py" "${bot_nav}"
  DEPENDS "${bot_root}/tools_pc/gen_simulant_nav.py"
    "${bot_root}/assets/obseg/setup/UsetuparkZ.c" "${bot_root}/assets/obseg/setup/Ump_setuparkZ.c"
  VERBATIM)
set(bot_sources "${bot_nav}")
foreach(unit mp_combat mp_roster mp_simulants simulant_probe)
  list(APPEND bot_sources "${bot_root}/port/src/${unit}.c")
endforeach()
set_source_files_properties("${bot_root}/port/src/simulant_probe.c" PROPERTIES INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}")
