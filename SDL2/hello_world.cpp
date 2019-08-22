/**
 * 将学习简单的将一张图片绘制到屏幕上的方法
 * 
 * CLI
 * cd ./SDL2
 * clear && make && ./bin/hello_world.out ../test.mp4
*/
#include <iostream>
#include "SDL2/SDL.h"
// using namespace std;

int main(int argc, char** argv){
    printf("hello world\n");
    if (SDL_Init(SDL_INIT_EVERYTHING) == -1){
        std::cout << SDL_GetError() << std::endl;
        return 1;
    }
}