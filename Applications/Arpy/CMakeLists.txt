# Arpy Application
file(GLOB_RECURSE ARPY_SOURCES "*.cpp" "*.c")
file(GLOB_RECURSE ARPY_HEADERS "*.h")

add_library(ArpyAPP ${ARPY_SOURCES} ${ARPY_HEADERS})

target_include_directories(ArpyAPP PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ArpyAPP PUBLIC
    MatrixOSInterface
    MatrixOSAPPInterface
    BrightnessControlUI
)

# Register the application class
RegisterApplicationClass(Arpy)
