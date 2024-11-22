cmake_minimum_required(VERSION 3.5)

if(WIN32)
    set(CMAKE_TOOLCHAIN_FILE "D:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE INTERNAL "" FORCE)
endif()

project(bioimread VERSION 0.1 LANGUAGES CXX)

set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "" FORCE)  # -Wno-dev

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# enable address sanitizer to find potential memory issues
if(CMAKE_BUILD_TYPE STREQUAL "ASAN")
    if (MSVC)
        add_compile_options(
            #/W4 
            /fsanitize=address /Zi)
    else()
        add_compile_options(
            #-Wall -Wextra -Wpedantic
            -fsanitize=address)
        add_link_options(-fsanitize=address)
    endif()
endif()

#set(CMAKE_BUILD_TYPE Release)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Xml)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(DEFINED ENV{JAVA_HOME})
    message("JAVA_HOME: $ENV{JAVA_HOME}")
else()
    message("NOT DEFINED JAVA_HOME ENV")
endif()
find_package(JNI REQUIRED)

add_subdirectory(bfwrapper)
add_subdirectory(series_reader)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB PROJECT_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${SRC_DIR}/*.hpp
    ${SRC_DIR}/*.cpp
    ${SRC_DIR}/*.ui
    ${SRC_DIR}/utils/*.hpp
    ${SRC_DIR}/utils/*.cpp
)

add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE ${JNI_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Core
    PRIVATE Qt6::Widgets
    PRIVATE Qt6::Xml
    #PRIVATE ${JNI_LIBRARIES} # no need to link, jvm.dll should be in ENV path
    reader
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    # WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
