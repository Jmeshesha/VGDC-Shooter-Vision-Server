cmake_minimum_required(VERSION 3.27.2)
project(Vision_Server VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

add_executable(${PROJECT_NAME} src/main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
