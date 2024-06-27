


set (SOURCES 
	"solis.h"
	"solis_scanner.h"
	"solis_scanner.c"
 "solis_common.h" "solis_common.c" "solis_compiler.h" "solis_chunk.h" "solis_chunk.c" "solis_value.h" "solis_value.c" "solis_vm.c" "solis_compiler.c" "solis_hashtable.c" "solis_object.c")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

add_library(SolisLang ${SOURCES})

target_include_directories(SolisLang PUBLIC "/")