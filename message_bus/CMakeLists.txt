project( message_bus )
cmake_minimum_required( VERSION 2.8 )

# set default build type
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
# CMAKE_CXX_FLAGS_DEBUG: 这是一个CMake内置变量，用于表示C++编译器在Debug模式下的编译选项。
# 当CMake在Debug模式下生成构建文件时，它会将该变量的值作为编译器的命令行参数。

# $ENV{CXXFLAGS}: 这部分表示从环境变量CXXFLAGS中获取的值。如果环境变量CXXFLAGS已设置，那么它的值会被添加到CMAKE_CXX_FLAGS_DEBUG变量中。
# -O0: 这是一个编译器选项，表示关闭所有优化。在Debug模式下，通常不需要优化，以便更容易调试。
# -Wall: 这是一个编译器选项，表示打开所有警告信息
# -g: 这是一个编译器选项，表示生成调试信息
# -ggdb: 这是一个编译器选项，表示生成适用于GNU Debugger (GDB) 的调试信息。这可以使GDB在调试程序时提供更丰富的功能。

    set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O2 -Wall")
endif()

# 包含一个名为 CheckCXXCompilerFlag 的模块
# CheckCXXCompilerFlag 模块提供了一个名为 check_cxx_compiler_flag 的函数，它可以用来检查 C++ 编译器是否支持给定的编译选项。
# 通过包含此模块，就可以在 CMakeLists.txt 文件中使用 check_cxx_compiler_flag 函数。这样，可以确保使用的编译选项在当前编译器中是可用的。
include( CheckCXXCompilerFlag )
# 检查编译器是否支持C++11，如果支持，就把COMPILER_SUPPORTS_CXX11设置为true
check_cxx_compiler_flag( "-std=c++11"   COMPILER_SUPPORTS_CXX11 )
if( COMPILER_SUPPORTS_CXX11 ) # 支持C++11
    if( CMAKE_COMPILER_IS_GNUCXX ) # 是否使用 GNU C++ 编译器 (g++)
    # -std=gnu++11 支持C++11并启用GNU扩展
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    endif()
endif()

#########################
# file 命令，通过文件名的通配符和递归搜索，将当前目录及其子目录下的所有具有 .h 或 .hpp 扩展名的头文件收集到一个名为 HEADER_FILES 的变量中
file(GLOB_RECURSE HEADER_FILES *.h *.hpp)
# HEADER_FILES 变量中收集到的文件分组到名为 "Include" 的源文件组
source_group("Include" FILES ${HEADER_FILES})
# aux_source_directory 命令，将当前目录（表示为 "."）中的所有源文件（通常是 .c 或 .cpp 文件）收集到一个名为 DIR_SRCS 的变量中
aux_source_directory(. DIR_SRCS)
# 将当前项目源目录的上一级目录下的 include 目录添加到编译器的头文件搜索路径中
include_directories(${PROJECT_SOURCE_DIR}/../include)
# 将当前项目源目录下的 include 目录添加到编译器的头文件搜索路径中
include_directories(${PROJECT_SOURCE_DIR}/include)
# 将当前项目源目录下的 lib 目录添加到编译器的库文件搜索路径中
link_directories(${PROJECT_SOURCE_DIR}/lib)
# 将当前项目源目录的上一级目录下的 lib 目录添加到编译器的库文件搜索路径中
link_directories(${PROJECT_SOURCE_DIR}/../lib)
# 将当前项目源目录的上一级目录下的 lib 目录添加到编译器的库文件搜索路径中
# 默认情况下，这将创建一个静态库（如 .lib 或 .a 文件），但可以通过指定关键字 SHARED、STATIC 或 MODULE 更改库类型
ADD_LIBRARY( ${PROJECT_NAME} ${DIR_SRCS})

########################
# 执行 make install才会执行
#  CMake 构建系统的配置文件片段，用于指定库的安装路径
INSTALL(TARGETS ${PROJECT_NAME} # 使用 CMake 的 INSTALL 命令来指定目标（在这里指库）的安装路径。这里的目标是由 ${PROJECT_NAME} 变量表示的库
    # 指定目标的可执行文件（如动态库 .dll 和 .so 文件）的安装路径。它们将被安装到当前项目源目录的上一级目录下的 bin 目录。
    RUNTIME DESTINATION ${PROJECT_SOURCE_DIR}/../bin
    # 指定动态库（如 .dll 和 .so 文件）的安装路径。它们将被安装到当前项目源目录的上一级目录下的 lib 目录
    LIBRARY DESTINATION ${PROJECT_SOURCE_DIR}/../lib
    # 指定目标的静态库（如 .lib 和 .a 文件）和导入库（用于链接动态库的 .lib 文件）的安装路径。它们将被安装到当前项目源目录的上一级目录下的 lib 目录。
    ARCHIVE DESTINATION ${PROJECT_SOURCE_DIR}/../lib
)