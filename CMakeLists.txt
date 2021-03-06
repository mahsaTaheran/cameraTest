# TODO cleanup cmake file
cmake_minimum_required(VERSION 3.15)
#add_compile_definitions(_GLIBCXX_USE_CXX11_ABI=0)
#add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)
project(FrameGrabber_Handler)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH  ${CMAKE_MODULE_PATH} "/opt/siso/cmake/modules")
#useful if we have multiple build-versions of boost. not sure really about the setting now
set (Boost_USE_STATIC_LIBS ON)
set(Boost_USE_DEBUG_LIBS OFF)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_RUNTIME OFF)
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
set(CMAKE_CXX_FLAGS "-lboost_program_options")
# set the directories as environment variables
set(ENV{SISODIR5} ${SISO_DIR})
set(ENV{FGLIB5} ${FGLIB_DIR})
set(ENV{ClSerSis} ${FGLIB_DIR})
#set(ENV{BOOST_ROOT} "/usr/local")
set(Boost_PO_LIBRARY_DIR /usr/local/lib)
#find will check the env variable $ENV{BOOST_ROOT}. this can be set before calling find_package
include(SisoPlatform)
find_package(FgLib5 REQUIRED)
find_package(ClSerSis REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Boost 1.73.0 COMPONENTS program_options REQUIRED)

set(PCO_INCLUDES /opt/pco_camera/pco_common/pco_include /opt/pco_camera/pco_common/pco_classes /opt/pco_camera/pco_me4/pco_classes)
set(PCO_LIBDIRS /opt/pco_camera/pco_common/pco_lib /opt/siso/lib)
# we can change this to add_library later if the project uses cmake instead of gnu make
if (Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR})
	add_library(FrameGrabber_Handler FrameGrabber.cpp FrameGrabber.h FrameGrabberHelper.hpp)
	add_executable(newTest newTest.cpp grabberTest.cpp Configuration.cpp)
	set_target_properties(FrameGrabber_Handler PROPERTIES COMPILE_FLAGS "-D USE_CLALLSERIAL -Wall -Wpedantic -Wno-write-strings -Wno-unknown-pragmas")
	target_include_directories(FrameGrabber_Handler PUBLIC ${PROJECT_SOURCE_DIR} ${PCO_INCLUDES} ${clsersis_INCLUDE_DIR} ${FgLib5_INCLUDE_DIR} ${OpenCV_INCLUDE})
	target_link_directories(FrameGrabber_Handler PUBLIC ${PCO_LIBDIRS})
	target_link_libraries(FrameGrabber_Handler PUBLIC ${FgLib5_LIBRARY} pcocom_me4 pcolog clsersis ${OpenCV_LIBS} pthread)
	#target_include_directories(configTest PUBLIC ${PROJECT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
	target_link_directories(newTest PUBLIC ${Boost_PO_LIBRARY_DIR})
	target_link_libraries(newTest PRIVATE FrameGrabber_Handler PUBLIC ${Boost_PROGRAM_OPTIONS_LIBRARY})
endif()
