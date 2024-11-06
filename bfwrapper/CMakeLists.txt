cmake_minimum_required(VERSION 3.5)

project(bfwrapper VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Java REQUIRED)
find_package(JNI REQUIRED)

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

add_executable(showinfo
    ${CMAKE_CURRENT_SOURCE_DIR}/showinfo.cpp
)
add_dependencies(showinfo
    ${PROJECT_NAME}
)

target_include_directories(showinfo
    PRIVATE ${JNI_INCLUDE_DIRS}
)
target_link_libraries(showinfo
    PRIVATE ${JNI_LIBRARIES}
)
