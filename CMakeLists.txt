cmake_minimum_required(VERSION 3.10.0)
project(text_editor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(wxWidgets REQUIRED COMPONENTS net core base stc)
if(wxWidgets_USE_FILE)
    include(${wxWidgets_USE_FILE})
endif()

add_executable(text_editor WIN32 src/main.cpp
    src/app.cpp src/MainFrame.cpp
    src/TextEditor.cpp)


target_include_directories(text_editor PUBLIC include)
target_link_libraries(text_editor ${wxWidgets_LIBRARIES})


include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
