

add_executable(SolisCLI "Main.c" "../Solis/terminal.h")

target_link_libraries(SolisCLI SolisLang)