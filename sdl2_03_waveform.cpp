/***
 * 主逻辑依赖 c++ 的d std::queue;
 * 同步视频 音频的包队列
 * 
 * main函数中解析文件 提取
 * av codec ctx
 * av codec parameter
 * av codec
 * 开启while循环 提取 packet 保存到queue
 * 
 * CLI
 * clear && make && ./bin/sdl2_02_audio_player.out ./test.mp4
 */

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
}
using namespace std;

#include <fstream>
// ofstream         //文件写操作 内存写入存储设备
// ifstream         //文件读操作，存储设备读区到内存中
// fstream          //读写操作，对打开的文件可进行读写操作

#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000
#define FF_INPUT_BUFFER_PADDING_SIZE 8
#define MAX_AUDIO_FRAME_SIZE 192000


int main(int argc, char *argv[])
{
    // const char* input_filename="/home/user/Music/Clip.mp3";
    const char *input_filename = argv[1];
    AVFormatContext *pFormatCtx;
    pFormatCtx = avformat_alloc_context();
    if (avformat_open_input(&pFormatCtx, input_filename, NULL, NULL) != 0)
    {
        printf("Can't open the file\n");
        return -1;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Couldn't find stream information.\n");
        return -1;
    }

    printf("%s %s\n", "播放文件", "信息------");
    av_dump_format(pFormatCtx, 0, input_filename, 0);
    printf("%s %s\n", "-------", "------");

    int audioIndex = -1;
    int i;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audioIndex < 0)
        {
            audioIndex = i;
            break;
        }
    }
    if (audioIndex == -1)
    {
        printf("Could not find Audio Stream \n");
        return -1;
    }
    

    // AVDictionary *metadata = pFormatCtx->metadata;
    AVCodecParameters *pCodecParam = pFormatCtx->streams[audioIndex]->codecpar;
    AVCodec *codec = avcodec_find_decoder(pCodecParam->codec_id);
    AVCodecContext *ctx = avcodec_alloc_context3(codec); //需要使用avcodec_free_context释放
    int rescode = avcodec_parameters_to_context(ctx, pCodecParam);
    if (rescode < 0){
        printf("上下文生成错误 avcodec_parameters_to_context(ctx, pCodecParam)\n");
    }

    if (codec == NULL)
    {
        printf("cannot find codec! 不知道什么类型的码\n");
        return -1;
    }

    if (avcodec_open2(ctx, codec, NULL) < 0)
    {
        printf("Codec cannot be found 解码程序没有 \n");
        return -1;
    }

    AVPacket *packet;
    AVFrame *frame;
    packet = av_packet_alloc();  //初始化一个packet
    frame = av_frame_alloc();    //初始化一个Frame
    int plane_size;

    ofstream fileCh1, fileCh2;
    fileCh1.open("ch1.csv");
    fileCh2.open("ch2.csv");


    // // int out_sample_rate = 44100;   //输出的采样率
    // int out_sample_rate = 48000;   //输出的采样率
    // enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16; //输出的声音格式
    // uint64_t out_chn_layout = AV_CH_LAYOUT_STEREO;  //输出的通道布局 双声道
    // int out_nb_samples = -1;        //输出的音频采样
    // int out_channels = -1;        //输出的通道数
    // int out_buffer_size = -1;   //输出buff大小
    // unsigned char *outBuff = NULL;//输出的Buffer数据
    // uint64_t in_chn_layout = -1;  //输入的通道布局
    // //单个通道中的采样数
    // out_nb_samples = ctx->frame_size;
    // //输出的声道数
    // out_channels = av_get_channel_layout_nb_channels(out_chn_layout);
    // //输出音频的布局
    // in_chn_layout = av_get_default_channel_layout(ctx->channels);


    // /** 计算重采样后的实际数据大小,并分配空间 **/
    // //计算输出的buffer的大小
    // out_buffer_size = av_samples_get_buffer_size(NULL, out_channels, out_nb_samples, out_sample_fmt, 1);
    // //分配输出buffer的空间
    // outBuff = (unsigned char *) av_malloc(MAX_AUDIO_FRAME_SIZE * 2); //双声道

    AVSampleFormat sfmt = ctx->sample_fmt;

    while (av_read_frame(pFormatCtx, packet) >= 0)
    {
        // printf("%llu \n", packet->buf);

        if (packet->stream_index == audioIndex)
        {
            // printf("packet->stream_index %llu \n", packet->stream_index);
            int ret = avcodec_send_packet(ctx, packet);
            printf("ret %d \n", ret);
            if (ret < 0)
            {
                fprintf(stderr, "Error submitting the packet to the decoder\n");
                exit(1);
            }

            while(avcodec_receive_frame(ctx, frame) == 0)
            {
                // printf("%d \n", frame->linesize[0]);
                // printf("%d \n", frame->nb_samples);
                // printf("%d \n", frame->channels);
                // printf("%d \n", frame->channel_layout);
                
                // printf("%d \n", frame->);
                // for (int ch = 0; ch < frame->channels; ch++)
                // {
                //     if (ch == 0)
                //         fileCh1 << ((int16_t *)frame->extended_data[ch]) << "\n";
                //     else if (ch == 1)
                //         fileCh2 << ((int16_t *)frame->extended_data[ch]) << "\n";
                // }



                            //         for (int nb = 0; nb < plane_size / sizeof(uint16_t); nb++)
            //         {
            //             for (int ch = 0; ch < ctx->channels; ch++)
            //             {
            //                 if (ch == 0)
            //                     fileCh1 << ((int16_t *)frame->extended_data[ch])[nb] << "\n";
            //                 else if (ch == 1)
            //                     fileCh2 << ((int16_t *)frame->extended_data[ch])[nb] << "\n";
            //             }
            //         }
            }

            
            


            // int data_size = av_samples_get_buffer_size(&plane_size, ctx->channels,
            //                                            frame->nb_samples,
            //                                            ctx->sample_fmt, 1);

            // printf("av_samples_get_buffer_size sfmt | plane_size %d |data_size %d|\n", plane_size, data_size);

            // while (ret >= 0)
            // {
            //     printf("ret %d |\n", ret);
            //     ret = avcodec_receive_frame(ctx, frame);
            //     if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            //         return ret;
            //     else if (ret < 0)
            //     {
            //         fprintf(stderr, "Error during decoding\n");
            //         exit(1);
            //     }

            //     // 枚举类型带P的代表非平面类型，带P的代表平面类型。
            //     // 所谓平面与非平面是针对在编解码的时候，数据存储的方式来判定的，
            //     //
            //     switch (sfmt)
            //     {
            //     case AV_SAMPLE_FMT_FLTP:
            //         printf("AVSampleFormat sfmt = %s\n", "AV_SAMPLE_FMT_FLTP");

            //         for (int nb = 0; nb < plane_size / sizeof(uint16_t); nb++)
            //         {
            //             for (int ch = 0; ch < ctx->channels; ch++)
            //             {
            //                 if (ch == 0)
            //                     fileCh1 << ((int16_t *)frame->extended_data[ch])[nb] << "\n";
            //                 else if (ch == 1)
            //                     fileCh2 << ((int16_t *)frame->extended_data[ch])[nb] << "\n";
            //             }
            //         }
            //         break;

            //     case AV_SAMPLE_FMT_S16P:
            //         printf("AVSampleFormat sfmt = %s\n", "AV_SAMPLE_FMT_S16P");
            //         for (int nb = 0; nb < plane_size / sizeof(uint16_t); nb++)
            //         {
            //             for (int ch = 0; ch < ctx->channels; ch++)
            //             {
            //                 if (ch == 0)
            //                     fileCh1 << ((int16_t *)frame->extended_data[ch])[nb] << "\n";
            //                 else if (ch == 1)
            //                     fileCh2 << ((int16_t *)frame->extended_data[ch])[nb] << "\n";
            //             }
            //         }

            //         break;
            //     default:
            //         break;
            //     }
            // }

            
        }

        av_packet_unref(packet);
    }
    fileCh1.close();
    fileCh2.close();
    avcodec_close(ctx);
    avformat_close_input(&pFormatCtx);
    // delete buffer;
    return 0;
}