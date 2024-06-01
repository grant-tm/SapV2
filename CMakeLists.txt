cmake_minimum_required(VERSION 3.10)
project("Sap")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find the FLTK package
find_package(FLTK REQUIRED)
find_package(sqlite REQUIRED);

# Include directories
include_directories(${FLTK_INCLUDE_DIRS})

# Add the executable
add_executable(${PROJECT_NAME} src/main.cpp)

# Link FLTK libraries
target_link_libraries(${PROJECT_NAME} ${FLTK_LIBRARIES})
target_link_libraries(${PROJECT_NAME} sqlite)