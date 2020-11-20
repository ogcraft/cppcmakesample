cmake_minimum_required(VERSION 3.1...3.19)

project(
  cppcmakesample
  VERSION 1.1
  LANGUAGES CXX)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_subdirectory(src)
