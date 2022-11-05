cmake_minimum_required(VERSION 3.0.0)
project(MiniCrawlSim VERSION 0.1.0)

include(FetchContent) 
FetchContent_Declare(mahi-gui GIT_REPOSITORY https://github.com/mahilab/mahi-gui.git) 
FetchContent_MakeAvailable(mahi-gui)

include(CTest)
enable_testing()

add_executable(MiniCrawlSim main.cpp)
target_link_libraries(MiniCrawlSim mahi::gui)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
