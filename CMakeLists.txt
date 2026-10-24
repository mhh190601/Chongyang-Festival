cmake_minimum_required(VERSION 3.20)
project(ChongYang LANGUAGES CXX)

# EasyX 的预编译库 EasyXw.lib 是以「静态 Release CRT」编译的
# （MT_StaticRelease / _ITERATOR_DEBUG_LEVEL=0）。
# 若 Debug 配置用 MTd_StaticDebug，链接时会报 LNK2038 运行库不匹配，
# 因此这里固定使用静态 Release CRT，Debug / Release 配置都能正常链接。
# 代价：Debug 下没有 STL 迭代器调试检查，对本项目无影响。
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/utf-8 /W3 /EHsc)
    # NOMINMAX：禁止 windows.h 把 min/max 定义成宏，否则 std::min( 会被
    # 展开成 std::( 而报 C2589（本项目已确认没有裸用 min()/max() 的代码）
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)
endif()

if(WIN32)
    link_directories("C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/VS/lib/x64")
    link_directories("C:/Program Files/Microsoft Visual Studio/18/Community/VC/Auxiliary/VS/lib/x86")
endif()

# 包含了所有源文件
add_executable(ChongYang
    src/main.cpp
    src/core/Console.cpp
    src/core/Input.cpp
    src/core/Bazi.cpp
    src/core/Audio.cpp
    src/entity/Petal.cpp
    src/entity/FloatingPoem.cpp
    src/scene/MenuScene.cpp
    src/scene/BlessScene.cpp
    src/scene/DivinationScene.cpp
    src/scene/OpeningScene.cpp
    src/scene/InteractiveScene.cpp
)

if(WIN32)
    target_link_libraries(ChongYang PRIVATE
        user32 gdi32 ole32 oleaut32 winmm msimg32 shell32
    )
endif()

set_target_properties(ChongYang PROPERTIES WIN32_EXECUTABLE FALSE)
