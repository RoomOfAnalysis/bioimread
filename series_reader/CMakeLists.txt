cmake_minimum_required(VERSION 3.16)

project(series_reader VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Xml Widgets)

add_library(${PROJECT_NAME}
    STATIC
    series_reader.cpp series_reader.hpp
)
target_link_libraries(${PROJECT_NAME}
    PUBLIC ${OpenCV_LIBS}
    PUBLIC Qt6::Xml
)

add_executable(${PROJECT_NAME}_test
    main.cpp
)
target_link_libraries(${PROJECT_NAME}_test
    PRIVATE ${PROJECT_NAME}
    PRIVATE Qt6::Widgets
)
