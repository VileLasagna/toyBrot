project(ToybrotCUDASDL LANGUAGES CXX CUDA)

add_executable(${PROJECT_NAME}  "main.cpp"
                                "FracGenWindow.hpp"
                                "FracGenWindow.cpp"
                                "FracGen.cu"
                                "FracGen.hpp")

if(NOT WIN32)
    target_link_libraries(${PROJECT_NAME} pthread)
endif()

target_link_libraries(${PROJECT_NAME} SDL2::SDL2)

