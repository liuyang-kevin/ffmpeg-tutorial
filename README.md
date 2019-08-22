# 学习性代码 偏重文档性质
## 环境说明：
+ Mac OS 10.8
+ FFmpeg 4+
+ SDL2 
+ c/c++ 代码都有

## XOS编译要点
* makefile + vscode 开发测试
* 配置文件保留在`.vscode`文件中
* makefile需要手动调整引用路径
* XOS上貌似sdl不能良好显示图像，需要x11
* 修改后可运行代码都使用SDL2



===================================
ffmpeg-tutorial
===============

* * *
教程实验代码来自[mpenkov/ffmpeg-tutorial](https://github.com/mpenkov/ffmpeg-tutorial)，作者已经停止维护，我在他的基础上，修改了代码学习FFmpeg。

作者的声明如下：

The [original tutorials](http://dranger.com/ffmpeg/) have now been [updated](https://ffmpeg.org/pipermail/libav-user/2015-February/007896.html).
I won't be maintaining this project anymore, and am keeping it here for historical reasons.
* * *

#### 环境说明：
+ Ubuntu 16.04.1 LTS
+ FFmpeg 2.8.7
+ SDL 1.2 

#### 编译运行

+ 首先通过源代码安装[FFmpeg 2.8.7](https://ffmpeg.org/download.html#releases)，然后编译本教程代码，通过如下命令：

	    git clone https://github.com/feixiao/ffmpeg-tutorial.git
    	cd ffmpeg-tutorial
    	make
  
+ 通过如下命令运行程序：
 	
 	 bin/tutorial01.out

#### 实验例子：
+ tutorial01
	+ 将前面的５帧图像分别保存到文件。
+ tutorial02
	+ 使用SDL播放视频帧数据。
+ tutorial03
	+ 播放音视频数据。
+ tutorial04
	+ 增加独立线程处理视频数据。
+ tutorial05
	+ 在tutorial04的基础上面增加pts进行同步。
+ tutorial06
	+ 在tutorial05的基础上面选择不同的音视频同步策略。
+ tutorial07
	+ 在tutorial06的基础上面增加seek处理。
+ tutorial08
	+ 使用rtp打包aac和h264文件。

#### 参考资料
+ [FFMPEG完美入门资料](http://download.csdn.net/download/leeking1989/7111345)
+ [一个广院工科生的视音频技术笔记](http://blog.csdn.net/leixiaohua1020)
