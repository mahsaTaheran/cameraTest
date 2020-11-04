# TODO cleanup cmake file
cmake_minimum_required(VERSION 3.15)
project(FrameGrabber_Handler)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} "/opt/siso/cmake/modules")
# Set this to 1 to automatically update the docs on reload of CMake
set(BUILD_DOCS 1)
if(BUILD_DOCS)
    execute_process(COMMAND doxygen Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    execute_process(COMMAND make WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/docs/latex)
endif(BUILD_DOCS)
# Make install directory setting available to user
set(SISO_DIR "/opt/siso" CACHE PATH "The install directory of the Silicon Software Runtime")
set(FGLIB_DIR "/opt/siso" CACHE PATH "The install directory of the FrameGrabber libs same as the runtime")

execute_process(COMMAND ${SISO_DIR}/setup-env.sh)

# set the directories as environment variables
set(ENV{SISODIR5} ${SISO_DIR})
set(ENV{FGLIB5} ${FGLIB_DIR})
set(ENV{ClSerSis} ${FGLIB_DIR})

include(SisoPlatform)
find_package(FgLib5 REQUIRED)
find_package(ClSerSis REQUIRED)
find_package(OpenCV REQUIRED)

set(PCO_INCLUDES /opt/pco_camera/pco_common/pco_include /opt/pco_camera/pco_common/pco_classes /opt/pco_camera/pco_me4/pco_classes)
set(PCO_LIBDIRS /opt/pco_camera/pco_common/pco_lib /opt/siso/lib)
# we can change this to add_library later if the project uses cmake instead of gnu make
add_library(FrameGrabber_Handler FrameGrabber.cpp FrameGrabber.h FrameGrabberHelper.hpp)
add_executable(FrameGrabberTest FramegrabberTest.cpp)
set_target_properties(FrameGrabber_Handler PROPERTIES COMPILE_FLAGS "-D USE_CLALLSERIAL -Wall -Wpedantic -Wno-write-strings -Wno-unknown-pragmas")
target_include_directories(FrameGrabber_Handler PUBLIC ${PCO_INCLUDES} ${clsersis_INCLUDE_DIR} ${FgLib5_INCLUDE_DIR} ${OpenCV_INCLUDE})
target_link_directories(FrameGrabber_Handler PUBLIC ${PCO_LIBDIRS})
target_link_libraries(FrameGrabber_Handler PUBLIC ${FgLib5_LIBRARY} pcocom_me4 pcolog clsersis ${OpenCV_LIBS} pthread)
target_link_libraries(FrameGrabberTest PRIVATE FrameGrabber_Handler)