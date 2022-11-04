# CMakeList.txt : CMake project for MiniCrawlSim, include source and define
# project specific logic here.
#


include(FetchContent) 
FetchContent_Declare(mahi-gui GIT_REPOSITORY https://github.com/mahilab/mahi-gui.git) 
FetchContent_MakeAvailable(mahi-gui)


# Add source to this project's executable.
add_executable (MiniCrawlSim "MiniCrawlSim.cpp" "MiniCrawlSim.h")
target_link_libraries(MiniCrawlSim mahi::gui)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET MiniCrawlSim PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
