add_executable(hurricane_rt1060
    hal_rt1060.c
    startup.c
)

target_link_libraries(hurricane_rt1060
    hurricane_core
)

target_include_directories(hurricane_rt1060 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/drivers
)

# Linker script and flags
target_link_options(hurricane_rt1060 PRIVATE
    "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld"
)
