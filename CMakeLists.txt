cmake_minimum_required(VERSION 3.18)
project(esp CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CI клонирует ImGui и передаёт -DIMGUI_DIR; локально по умолчанию ./imgui
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/imgui CACHE PATH "path to full Dear ImGui checkout")
if(NOT EXISTS ${IMGUI_DIR}/imgui.cpp)
    message(FATAL_ERROR "no imgui.cpp in ${IMGUI_DIR}")
endif()

add_executable(esp
    main.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# your overlay window / touch code
if(EXISTS ${CMAKE_SOURCE_DIR}/surface.cpp)
    target_sources(esp PRIVATE surface.cpp)
else()
    message(WARNING "surface.cpp not found: link will fail on create_overlay_window/poll_touch")
endif()

# IMGUI_DIR первым: заголовки из репо-папки imgui/ в include не попадают
target_include_directories(esp PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends ${CMAKE_SOURCE_DIR})
target_compile_definitions(esp PRIVATE IMGUI_IMPL_OPENGL_ES3)
target_link_libraries(esp EGL GLESv3 android log)
