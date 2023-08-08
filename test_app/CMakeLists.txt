project(test_app)
cmake_minimum_required( VERSION 2.8 )

# set default build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")
endif()

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-std=c++11"   COMPILER_SUPPORTS_CXX11 )
if( COMPILER_SUPPORTS_CXX11 )
    if( CMAKE_COMPILER_IS_GNUCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    endif()
endif()

#########################
# aux_source_directory 命令，将当前目录（表示为 "."）中的所有源文件（通常是 .c 或 .cpp 文件）收集到一个名为 DIR_SRCS 的变量中
aux_source_directory(. DIR_SRCS)
# 将当前项目源目录下的 lib 目录添加到编译器的库文件搜索路径中
link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(${PROJECT_SOURCE_DIR}/../lib)
# 将当前项目源目录的上一级目录下的 message_bus 目录添加到编译器的头文件搜索路径中
include_directories(${PROJECT_SOURCE_DIR}/../message_bus)
# 创建一个可执行程序，DIR_SRCS包含了用于构建可执行程序的源文件列表
add_executable(${PROJECT_NAME} ${DIR_SRCS})


if(CMAKE_SYSTEM_NAME MATCHES "Linux")
    MESSAGE(STATUS "current platform: Linux")  
    # 它的作用是将指定的库链接到目标（在这里是一个可执行程序）上
    target_link_libraries(${PROJECT_NAME}
        message_bus
        pthread
    )

elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
    target_link_libraries(${PROJECT_NAME}
        message_bus 
    )
endif()
########################

# set the output dir
INSTALL(TARGETS ${PROJECT_NAME} 
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/../bin
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/../lib
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/../lib
)