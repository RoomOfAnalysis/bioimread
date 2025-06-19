cmake_minimum_required(VERSION 3.16)

project(qpwrapper VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)

find_package(OpenCV REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui WebEngineWidgets WebChannel)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(UseJava)
set(JAVA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/java)
file(MAKE_DIRECTORY ${JAVA_OUTPUT_DIR})
file(GLOB JAVA_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.java
)
set(bfwrapper_dir ${CMAKE_CURRENT_SOURCE_DIR}/../bfwrapper)
file(GLOB JARS LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/qupath_jars/*.jar
)
add_jar(${PROJECT_NAME}
    SOURCES ${JAVA_SOURCES}
    OUTPUT_DIR ${JAVA_OUTPUT_DIR}
    INCLUDE_JARS  ${JARS}
    ENTRY_POINT ${PROJECT_NAME}
)

add_executable(${PROJECT_NAME}_test
    ${bfwrapper_dir}/jvmwrapper.cpp ${bfwrapper_dir}/jvmwrapper.hpp
    ${bfwrapper_dir}/stopwatch.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
add_dependencies(${PROJECT_NAME}_test
    ${PROJECT_NAME}
)

target_include_directories(${PROJECT_NAME}_test
    PRIVATE ${JNI_INCLUDE_DIRS}
    PRIVATE ${bfwrapper_dir}
)
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE ${JNI_LIBRARIES}
    PRIVATE ${OpenCV_LIBS}
)

add_library(qpreader
    STATIC
    ${bfwrapper_dir}/jvmwrapper.cpp ${bfwrapper_dir}/jvmwrapper.hpp
    reader.cpp reader.hpp
)
add_dependencies(qpreader
    ${PROJECT_NAME}
)
target_include_directories(qpreader
    PRIVATE ${JNI_INCLUDE_DIRS}
    PRIVATE ${bfwrapper_dir}
)
target_link_libraries(qpreader
    PUBLIC ${JNI_LIBRARIES}
)

add_executable(qpreader_test
    ${CMAKE_CURRENT_SOURCE_DIR}/reader_test.cpp
)
target_link_libraries(qpreader_test
    PRIVATE qpreader
)

add_library(deepzoom
    STATIC
    deepzoom.cpp deepzoom.hpp
)
add_dependencies(deepzoom
    ${PROJECT_NAME}
)
target_link_libraries(deepzoom
    PUBLIC qpreader
)

add_executable(deepzoom_test
    ${CMAKE_CURRENT_SOURCE_DIR}/deepzoom_test.cpp
)
target_link_libraries(deepzoom_test
    PRIVATE deepzoom
)

add_executable(tilesviewer
    ${CMAKE_CURRENT_SOURCE_DIR}/tilesviewer.cpp
)
target_link_libraries(tilesviewer
    PRIVATE Qt${QT_VERSION_MAJOR}::Core
    PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
    PRIVATE Qt${QT_VERSION_MAJOR}::Gui
    PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets
    PRIVATE Qt${QT_VERSION_MAJOR}::WebChannel
    PRIVATE deepzoom
)
