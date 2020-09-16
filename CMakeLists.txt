cmake_minimum_required (VERSION 3.9)

project (dephier)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(GDAL REQUIRED)

add_subdirectory(submodules/richdem EXCLUDE_FROM_ALL)

include_directories(submodules)

add_library(dephier INTERFACE)
target_include_directories(dephier INTERFACE include/)
target_compile_features(dephier INTERFACE cxx_auto_type cxx_std_17)


add_executable(dephier.exe src/main.cpp)
target_link_libraries(dephier.exe PRIVATE dephier richdem)

add_executable(dephier_paper_tests.exe src/dephier_paper_tests.cpp)
target_link_libraries(dephier_paper_tests.exe PRIVATE dephier richdem)
target_compile_features(dephier_paper_tests.exe PUBLIC cxx_auto_type cxx_std_17)

install(TARGETS dephier.exe DESTINATION bin)
install(TARGETS dephier_paper_tests.exe DESTINATION bin)