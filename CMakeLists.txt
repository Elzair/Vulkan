cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
cmake_policy(VERSION 2.8)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include("cmake/defaults.cmake")
set(NAME vulkanExamples)

project(${NAME})

add_custom_target(SetupRelease ALL ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin)
set_target_properties(SetupRelease PROPERTIES FOLDER "CMakeTargets")
add_custom_target(SetupDebug ALL ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/bin_debug)
set_target_properties(SetupDebug PROPERTIES FOLDER "CMakeTargets")

find_package(Vulkan REQUIRED)
link_libraries(${VULKAN_LIBRARY})
include_directories(${VULKAN_INCLUDE_DIR})

if(WIN32)
    add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
else()
    add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
    find_package(XCB REQUIRED)
    link_libraries(${XCB_LIBRARIES})
endif()

if (WIN32)
    add_dependency_external_projects(glfw3)
    list(APPEND EXTERNALS glfw3)
    include_directories(${GLFW3_INCLUDE_DIR})
    link_libraries(${GLFW3_LIBRARY})

    add_dependency_external_projects(zlib)
    list(APPEND EXTERNALS zlib)
    include_directories(${ZLIB_INCLUDE_DIRS})
    link_libraries(${ZLIB_LIBRARIES})

    add_dependency_external_projects(assimp)
    list(APPEND EXTERNALS assimp)
    include_directories(${ASSIMP_INCLUDE_DIRS})
    link_libraries(${ASSIMP_LIBRARIES})
else()
    pkg_check_modules(GLFW3 REQUIRED glfw3>=3.2)
    link_libraries(${GLFW3_LIBRARIES})
    include_directories(${GLFW3_INCLUDEDIR})
    link_directories(${GLFW3_LIBRARY_DIRS})
    
    find_package(assimp)
    link_libraries(${ASSIMP_LIBRARIES})
    include_directories(${ASSIMP_INCLUDEDIR})
    link_directories(${ASSIMP_LIBRARY_DIRS})
endif()

add_dependency_external_projects(glm)
list(APPEND EXTERNALS glm)
include_directories(${GLM_INCLUDE_DIRS})

add_dependency_external_projects(vkcpp)
list(APPEND EXTERNALS vkcpp)
include_directories(${VKCPP_INCLUDE_DIRS})

add_dependency_external_projects(gli)
list(APPEND EXTERNALS gli)
include_directories(${GLI_INCLUDE_DIRS})

add_dependency_external_projects(glslang)
list(APPEND EXTERNALS glslang)
include_directories(${GLSLANG_INCLUDE_DIRS})
link_libraries(${GLSLANG_LIBRARIES})

add_subdirectory(base)
link_libraries(base)
include_directories(base)

foreach(EXTERNAL ${EXTERNALS})
    add_dependencies(base ${EXTERNAL})
endforeach()

if (NOT WIN32)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    link_libraries(${CMAKE_THREAD_LIBS_INIT})
endif()

add_subdirectory(examples)

