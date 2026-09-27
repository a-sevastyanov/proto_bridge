cmake_minimum_required(VERSION 3.16)
project(protobridge_receiver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Найти установленную в системе библиотеку protobuf
find_package(Protobuf REQUIRED)

# Указывает компилятору из каких .cpp-файлов собирать
# исполняемый файл, а также называет его.
add_executable(receiver
    ${CMAKE_CURRENT_SOURCE_DIR}/receiver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/user.pb.cc
)

# Где искать заголовочные файлы (.hpp, .pb.h) при компиляции.
# Полезно, если они лежат не в той же папке, что и .cpp.
target_include_directories(receiver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/proto
    ${Protobuf_INCLUDE_DIRS}
)

# Указывает компоновщику подключить эту библиотеку (в нашем
# случае libprotobuf, чтобы разрешились все символы, которые
# user.pb.cc использует из runtime protobuf).
target_link_libraries(receiver PRIVATE
    ${Protobuf_LIBRARIES}
)
