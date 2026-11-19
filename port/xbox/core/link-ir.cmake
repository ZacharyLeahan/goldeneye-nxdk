# llvm-link on Windows accepts long object lists through a response file.
# Relative forward-slash paths also work when MSYS2 invokes native LLVM.
file(STRINGS "core/ir-objects.txt" objects)
set(response "")
foreach(object IN LISTS objects)
  file(RELATIVE_PATH relative "${CMAKE_CURRENT_BINARY_DIR}" "${object}")
  string(APPEND response "\"${relative}\"\n")
endforeach()
file(WRITE "core/ir-objects.rsp" "${response}")
execute_process(COMMAND "${linker}" @core/ir-objects.rsp -o core/all.bc
  COMMAND_ERROR_IS_FATAL ANY)
