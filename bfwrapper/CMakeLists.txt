cmake_minimum_required(VERSION 3.16)

project(bfwrapper VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)

find_package(OpenCV REQUIRED)

include(UseJava)
set(JAVA_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/java)
file(MAKE_DIRECTORY ${JAVA_OUTPUT_DIR})
file(GLOB JAVA_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.java
)
file(GLOB JARS LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/bioformats_jars/*.jar
)
add_jar(${PROJECT_NAME}
    SOURCES ${JAVA_SOURCES}
    OUTPUT_DIR ${JAVA_OUTPUT_DIR}
    INCLUDE_JARS  ${JARS}
    ENTRY_POINT ${PROJECT_NAME}
)

add_executable(${PROJECT_NAME}_test
    jvmwrapper.cpp jvmwrapper.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)
add_dependencies(${PROJECT_NAME}_test
    ${PROJECT_NAME}
)

target_include_directories(${PROJECT_NAME}_test
    PRIVATE ${JNI_INCLUDE_DIRS}
)
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE ${JNI_LIBRARIES}
    PRIVATE ${OpenCV_LIBS}
)

add_library(reader
    STATIC
    jvmwrapper.cpp jvmwrapper.hpp
    reader.cpp reader.hpp
)
target_include_directories(reader
    PRIVATE ${JNI_INCLUDE_DIRS}
)
target_link_libraries(reader
    PUBLIC ${JNI_LIBRARIES}
)
