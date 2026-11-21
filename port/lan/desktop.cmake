include("${CMAKE_CURRENT_LIST_DIR}/build-id.cmake")
find_package(Python3 COMPONENTS Interpreter REQUIRED)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lan")

set(GE_SCHED_ROOT "${CMAKE_SOURCE_DIR}")
include("${CMAKE_CURRENT_LIST_DIR}/scheduler.cmake")
list(REMOVE_ITEM SRC src/sched.c "${scheduler_source}")
list(APPEND SRC "${port_scheduler}")

include("${CMAKE_CURRENT_LIST_DIR}/simulants.cmake")
list(APPEND SRC ${bot_sources})
foreach(unit radar chraction boss lv bg model bondview2 gun propobj chrprop mpmenu)
  set(src "${CMAKE_SOURCE_DIR}/src/game/${unit}.c")
  if(unit STREQUAL "boss")
    set(src "${CMAKE_SOURCE_DIR}/src/boss.c")
    list(REMOVE_ITEM SRC src/boss.c)
  endif()
  set(dst "${CMAKE_CURRENT_BINARY_DIR}/lan/${unit}.c")
  add_custom_command(OUTPUT "${dst}" COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_LIST_DIR}/adapt.py" "${src}" "${dst}"
    DEPENDS "${src}" "${CMAKE_CURRENT_LIST_DIR}/adapt.py" VERBATIM)
  list(REMOVE_ITEM SRC "${src}")
  list(APPEND SRC "${dst}")
endforeach()
set(front_source "${CMAKE_SOURCE_DIR}/src/game/front.c")
set(front_copy "${CMAKE_CURRENT_BINARY_DIR}/lan/front.c")
add_custom_command(OUTPUT "${front_copy}" COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_LIST_DIR}/mode_menu.py" "${front_source}" "${front_copy}"
  DEPENDS "${front_source}" "${CMAKE_CURRENT_LIST_DIR}/mode_menu.py" VERBATIM)
list(REMOVE_ITEM SRC "${front_source}")
list(APPEND SRC "${front_copy}")

# The Windows host selects an authored guest spawn for repeatable profiling.
set(guest_spawn_source "${CMAKE_SOURCE_DIR}/src/game/bondview_r.c")
set(guest_spawn_copy "${CMAKE_CURRENT_BINARY_DIR}/lan/bondview_r.c")
add_custom_command(OUTPUT "${guest_spawn_copy}" COMMAND "${Python3_EXECUTABLE}"
  "${CMAKE_CURRENT_LIST_DIR}/tests/egypt_camera.py" "${guest_spawn_source}" "${guest_spawn_copy}"
  DEPENDS "${guest_spawn_source}" "${CMAKE_CURRENT_LIST_DIR}/tests/egypt_camera.py" VERBATIM)
list(REMOVE_ITEM SRC "${guest_spawn_source}")
list(APPEND SRC "${guest_spawn_copy}"
  "${CMAKE_SOURCE_DIR}/port/controls/finals_controls.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/finals_desktop.cpp"
  "${CMAKE_CURRENT_LIST_DIR}/frame_trace.c" "${CMAKE_CURRENT_LIST_DIR}/lan.c" "${CMAKE_CURRENT_LIST_DIR}/desktop.c")
list(REMOVE_ITEM SRC "${CMAKE_SOURCE_DIR}/port/src/pc_netorder.c")
list(APPEND LIBS ws2_32 iphlpapi gdi32)
add_compile_definitions(GE_LAN_HOST=1)

add_executable(lan_discovery_test EXCLUDE_FROM_ALL
  "${CMAKE_CURRENT_LIST_DIR}/tests/discovery.c"
  "${CMAKE_CURRENT_LIST_DIR}/lan.c")
target_link_libraries(lan_discovery_test ${LIBS} ws2_32)
