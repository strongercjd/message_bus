project(message_bus)
cmake_minimum_required( VERSION 2.8 )

# add_subdirectory:将指定的子目录添加到当前项目的构建过程中
# 这个命令告诉 CMake 在处理当前项目时，还需要处理子目录中的 CMakeLists.txt 文件。
add_subdirectory(message_bus)
add_subdirectory(test_app)



