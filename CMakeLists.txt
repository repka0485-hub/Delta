cmake_minimum_required(VERSION 3.18)
project(esp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(IMGUI ${CMAKE_SOURCE_DIR}/imgui)

add_executable(esp
    main.cpp
    ${IMGUI}/imgui.cpp
    ${IMGUI}/imgui_draw.cpp
    ${IMGUI}/imgui_tables.cpp
    ${IMGUI}/imgui_widgets.cpp
    ${IMGUI}/backends/imgui_impl_opengl3.cpp
)

# your overlay window / touch code
if(EXISTS ${CMAKE_SOURCE_DIR}/surface.cpp)
    target_sources(esp PRIVATE surface.cpp)
endif()

target_include_directories(esp PRIVATE ${IMGUI} ${IMGUI}/backends ${CMAKE_SOURCE_DIR})
target_compile_definitions(esp PRIVATE IMGUI_IMPL_OPENGL_ES3)
target_link_libraries(esp EGL GLESv3 android log)
