# Both architectures hash the movement protocol and native integration. Normalize
# checkout EOLs so a normal Windows autocrlf checkout gets the same identity.
set(lan_signature "ntsc-final/complex/2")
foreach(name lan.c lan.h pose.inc inventory.inc drops.inc adapt.py ../xbox/core/finals_game.inc)
  set(path "${CMAKE_CURRENT_LIST_DIR}/${name}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${path}")
  file(READ "${path}" contents)
  string(REPLACE "\r\n" "\n" contents "${contents}")
  string(SHA256 digest "${contents}")
  string(APPEND lan_signature "${digest}")
endforeach()
string(SHA256 digest "${lan_signature}")
string(SUBSTRING "${digest}" 0 8 lan_id)
set_property(SOURCE "${CMAKE_CURRENT_LIST_DIR}/lan.c" APPEND PROPERTY COMPILE_DEFINITIONS LAN_BUILD_ID=0x${lan_id}u)
