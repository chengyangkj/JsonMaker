cmake_minimum_required(VERSION 3.14.0 FATAL_ERROR)
project(JsonMaker LANGUAGES CXX)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/rapidjson
                    ${CMAKE_CURRENT_SOURCE_DIR}
                    )
add_library(${PROJECT_NAME} INTERFACE)
