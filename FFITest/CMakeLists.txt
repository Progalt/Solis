
add_library(SolisFFITest SHARED "SolisLib.h" "SolisLib.c")

target_link_libraries(SolisFFITest SolisLang)