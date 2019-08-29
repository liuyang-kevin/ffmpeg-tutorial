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

    // 创建一个窗口
    SDL_Window *win = nullptr;
    win = SDL_CreateWindow("Hello World!", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << SDL_GetError() << std::endl;
        return 1;
    }

    // renderer（渲染器）是用SDL_CreateRenderer这个函数创建
    SDL_Renderer *ren = nullptr;
    ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr){
        std::cout << SDL_GetError() << std::endl;
        return 1;
    }
    // SDL_RENDERER_ACCELERATED，因为我们想使用硬件加速的renderer，换句话说就是想利用显卡的力量。
    // SDL_RENDERER_PRESENTVSYNC标志，因为我们想要使用SDL_RendererPresent这个函数，这个函数将会以显示器的刷新率来更新画面。


    SDL_Surface *bmp = nullptr;
    bmp = SDL_LoadBMP("../../docs/images/hello.bmp");
    if (bmp == nullptr){
        std::cout << SDL_GetError() << std::endl;
        return 1;
    }
    // OpenGL 时代的 Texture 是一种名词
    // D3D10+ 时代的 Texture 是另一种名词
    // Vulkan 里的 Texture 是什么？
    // 在 GPU 内部 Texture 既是名词也是动词
    // 要有效地利用硬件加速来绘制，我们必须把SDL_Surface转化为SDL_Texture，这样renderer才能够绘制
    SDL_Texture *tex = nullptr;
    tex = SDL_CreateTextureFromSurface(ren, bmp);
    SDL_FreeSurface(bmp);  // SDL_Surface释放掉，因为以后就用不着它了。


    SDL_RenderClear(ren); //使用SDL_RenderClear来清空屏幕
    SDL_RenderCopy(ren, tex, NULL, NULL); //把texture画上去。
    SDL_RenderPresent(ren);  //更新屏幕的画面。

    // SDL_RenderCopy传了两个NULL值。第一个NULL是一个指向源矩形的指针，
    // 也就是说，从图像上裁剪下的一块矩形；而另一个是指向目标矩形的指针。
    // 我们将NULL传入这两个参数，
    // 是告诉SDL绘制整个源图像（第一个NULL），并把它画在屏幕上（0，0 ）的位置，
    // 并拉伸这个图像让它填满整个窗口（第二个NULL）


    // SDL_Delay(5000); // 防止立刻退出 // Mac上好像不行

    // Mac上添加事件循环才可以出来
    SDL_Event e;
    while (true)
    {
        while (SDL_PollEvent(&e)) 
        {
            if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
            {
                // 释放内存
                SDL_DestroyTexture(tex);
                SDL_DestroyRenderer(ren);
                SDL_DestroyWindow(win);

                SDL_Quit();
                goto exit;
                break;
            }
            // Handle events
        }
    }
    
exit:
    return 0;
}