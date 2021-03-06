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
extern "C"{
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswresample/swresample.h"

    #include "libswscale/swscale.h"
    #include "libavutil/opt.h"
    // #include "libswscale/swscale_internal.h"
}
#include <queue>
#include <SDL2/SDL.h>
using namespace std;



#define SDL_AUDIO_BUFFER_SIZE            (1152)  // ??
#define AVCODEC_MAX_AUDIO_FRAME_SIZE    (192000) // ??

// -------- 库 ----------
typedef struct video_thread_params
{
    SwsContext *sws_context;
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    AVCodecContext *vid_codec_context;
    SDL_mutex *video_mutex;
} video_thread_params;

int video_thread_proc(void *data);  // 用于开启视频流处理线程


//创建一个全局的结构体变量以便于我们从文件中得到的声音包有地方存
//放同时也保证SDL中的声音回调函数audio_callback 能从这个地方得到声音数据
typedef struct PacketQueue{
    AVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    SDL_mutex *mutex; //互斥锁; 因为SDL 是在一个独立的线程中来进行音频处理的。如果我们没有正确的锁定这个队列，我们有可能把数据搞乱。
    SDL_cond *cond;  //条件变量
} PacketQueue;

void packet_queue_init(PacketQueue *pq){
    memset(pq, 0, sizeof(PacketQueue));
    pq->mutex = SDL_CreateMutex();
    pq->cond = SDL_CreateCond();
}

int packet_queue_put(PacketQueue *q, AVPacket *pkt){
    AVPacketList *pkt1;
    /*(
    if (av_dup_packet(pkt) < 0){
        printf("error");
        return -1;
    }
    */

    pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
    if (!pkt1){
        printf("error");
        return -1;
    }

    av_copy_packet(&pkt1->pkt, pkt);
    av_free_packet(pkt);
    pkt1->next = NULL;

    //函数SDL_LockMutex()锁定队列的互斥量以便于我们向队列中添加东西，然后函
    //数SDL_CondSignal()通过我们的条件变量为一个接 收函数（如果它在等待）发
    //出一个信号来告诉它现在已经有数据了，接着就会解锁互斥量并让队列可以自由
    //访问。
    SDL_LockMutex(q->mutex);

    if (!q->last_pkt)//队列为空
        q->first_pkt = pkt1;
    else//队列不为空
        q->last_pkt->next = pkt1;
    q->last_pkt = pkt1;
    q->nb_packets++;
    q->size += pkt1->pkt.size;
    SDL_CondSignal(q->cond);

    SDL_UnlockMutex(q->mutex);

    return 0;
}
// -------- 业务 ----------
static int quit = 0;  // app 退出
static int video_quit = 0;  // 


static int64_t audio_pts = 0;
static int64_t audio_dts = 0;

PacketQueue audioq;
PacketQueue videoq;
queue<AVFrame *> frameq; // C++的std queue 渲染视频使用队列

static AVFrame *g_pFrameYUV;

SDL_Thread *g_pVideoThread;
SDL_mutex *g_pVideoMutex;


// 声音

static int packet_queue_get(PacketQueue *q, AVPacket *pkt, int block){
    AVPacketList *pkt1;
    int ret;

    SDL_LockMutex(q->mutex);

    for (;;){
        if (quit){
            ret = -1;
            break;
        }

        pkt1 = q->first_pkt;
        if (pkt1){
            q->first_pkt = pkt1->next;
            if (!q->first_pkt)
                q->last_pkt = NULL;
            q->nb_packets--;
            q->size -= pkt1->pkt.size;
            //*pkt = pkt1->pkt;
            av_copy_packet(pkt, &pkt1->pkt);
            av_free_packet(&pkt1->pkt);
            av_free(pkt1);
            ret = 1;
            break;
        }
        else if (!block){
            ret = 0;
            break;
        }
        else{
            SDL_CondWait(q->cond, q->mutex);
        }
    }
    SDL_UnlockMutex(q->mutex);
    return ret;
}


/**
 * 
 * 
*/
int AudioResampling(AVCodecContext * audio_dec_ctx,
                    AVFrame * pAudioDecodeFrame,
                    int out_sample_fmt,
                    int out_channels,
                    int out_sample_rate,
                    uint8_t* out_buf)
{
    SwrContext * swr_ctx = NULL; //SwrContext重采样结构体
    int data_size = 0;
    int ret = 0;
    int64_t src_ch_layout = audio_dec_ctx->channel_layout;
    int64_t dst_ch_layout = AV_CH_LAYOUT_STEREO;
    int dst_nb_channels = 0;
    int dst_linesize = 0;
    int src_nb_samples = 0;
    int dst_nb_samples = 0;
    int max_dst_nb_samples = 0;
    uint8_t **dst_data = NULL;
    int resampled_data_size = 0;

    swr_ctx = swr_alloc();
    if (!swr_ctx)
    {
        printf("swr_alloc error \n");
        return -1;
    }

    src_ch_layout = (audio_dec_ctx->channels ==
                     av_get_channel_layout_nb_channels(audio_dec_ctx->channel_layout)) ?
                     audio_dec_ctx->channel_layout :
                     av_get_default_channel_layout(audio_dec_ctx->channels);

    if (out_channels == 1)
    {
        dst_ch_layout = AV_CH_LAYOUT_MONO;
        //printf("dst_ch_layout: AV_CH_LAYOUT_MONO\n");
    }
    else if (out_channels == 2)
    {
        dst_ch_layout = AV_CH_LAYOUT_STEREO;
        //printf("dst_ch_layout: AV_CH_LAYOUT_STEREO\n");
    }
    else
    {
        dst_ch_layout = AV_CH_LAYOUT_SURROUND;
        //printf("dst_ch_layout: AV_CH_LAYOUT_SURROUND\n");
    }

    if (src_ch_layout <= 0)
    {
        printf("src_ch_layout error \n");
        return -1;
    }

    src_nb_samples = pAudioDecodeFrame->nb_samples;
    if (src_nb_samples <= 0)
    {
        printf("src_nb_samples error \n");
        return -1;
    }
}


/**
 * 把数据存储在一个中间缓冲中，企图将字节转变为流，当我们数据不够的时候提供给我们，
 * 当数据塞满时帮我们保存数据以使我们以后再用。
*/
int audio_decode_frame(AVCodecContext *aCodecCtx, uint8_t *audio_buf, int buf_size){
    static AVPacket pkt;
    static uint8_t *audio_pkt_data = NULL;
    static int audio_pkt_size = 0;

    int len1, data_size, ret = 0;

    static AVFrame *pFrame;
    pFrame = av_frame_alloc();

    for (;;){
        while (audio_pkt_size > 0){
            data_size = buf_size;
            len1 = avcodec_decode_audio4(aCodecCtx, pFrame, &ret, &pkt);

            //len1 = avcodec_decode_audio3(aCodecCtx, (int16_t *)audio_buf,
            //    &data_size, &pkt);
            if (len1 < 0){//if error, skip frame
                printf("error\n");
                audio_pkt_size = 0;
                break;
            }
            data_size = AudioResampling(aCodecCtx, pFrame, AV_SAMPLE_FMT_S16, 2, 44100, audio_buf);
            audio_pkt_data += len1;
            audio_pkt_size -= len1;
            if (data_size <= 0)//No data yet, get more frames
                continue;
            return data_size;
        }
        //if (pkt.data)
            av_free_packet(&pkt);
        if (quit)
            return -1;
        if (packet_queue_get(&audioq, &pkt, 1) < 0){//从这里开始，取得main线程放入队列的包
            printf("error, can't get packet from the queue");
            return -1;
        }

        //SDL_LockMutex(g_pVideoMutex);
        audio_pts = pkt.pts;
        audio_dts = pkt.dts;
        //SDL_UnlockMutex(g_pVideoMutex);

        audio_pkt_data = pkt.data;
        audio_pkt_size = pkt.size;
    }
}


/**
 * 声音回调函数
 * userdata是输入，stream是输出，len是输入，len的值一般为4096（调试中发现的）
 * audio_callback函数的功能是调用audio_decode_frame函数，把解码后数据块audio_buf追加在stream的后面，
 * 通过SDL库对audio_callback的不断调用，不断解码数据，然后放到stream的末尾，
 * SDL库认为stream中数据够播放一帧音频了，就播放它, 
 * 第三个参数len是向stream中写数据的内存分配尺度，是分配给audio_callback函数写入缓存大小。
*/
void audio_callback(void *userdata, Uint8 *stream, int len){
    // //SDL_memset(stream, 0, len);
    AVCodecContext *aCodecCtx = (AVCodecContext*)userdata; //??
    int len1, audio_size;

    // //audio_buf 的大小为 1.5 倍的声音帧的大    小以便于有一个比较好的缓冲
    static uint8_t audio_buf[(AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2];
    static unsigned int audio_buf_size = 0;
    static unsigned int audio_buf_index = 0;

    while (len > 0){
        if (audio_buf_index >= audio_buf_size){ //already send all our data, get more
            audio_size = audio_decode_frame(aCodecCtx, audio_buf, sizeof(audio_buf));
            if (audio_size < 0){//error, output silence
                printf("error, output silence\n");
                audio_buf_size = SDL_AUDIO_BUFFER_SIZE;
                memset(audio_buf, 0, audio_buf_size);
            }
            else
                audio_buf_size = audio_size;
            audio_buf_index = 0;
        }
        len1 = audio_buf_size - audio_buf_index;
        if (len1>len){
            len1 = len;
        }
        memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
        len -= len1;
        stream += len1;
        audio_buf_index += len1;
    }
}


// 视频
/**
 * 视频流处理线程
 * 这里对齐是以音频流为准的
 * 所以如果音频流（相对于视频流）过于稀疏的话，就会失去作用，效果不好
*/
int video_thread_proc(void *data)
{
    printf("video_thread_proc  %lu\n", frameq.size());

    video_thread_params *params = (video_thread_params *)data;
    AVFrame *pFrame = NULL;
    AVFrame *pNextFrame = NULL;
    AVPacket packet = {0};
    AVPacket nextpacket;

    // 注意，以下代码比较的都是 DTS（解压缩时间戳）而不是 PTS（显示时间戳）！！！
    // 实际视频显示是以 PTS 为准的，但是 PTS 大多没有规律（只要是在解压之后就好），难以处理
    // 所幸在相当一部分视频中 DTS 与 PTS 相差较小，所以这里就用 DTS 了

    while (!video_quit)
    {
        while (!frameq.empty())
        {
            printf("frameq.size %lu\n", frameq.size());
            if (pFrame == NULL)
            {
                SDL_LockMutex(params->video_mutex);

                // 这里采用了“连续读取”的方法，也即如果下一帧的 DTS 小于当前基准（目前采用音频流），则循环直至找到第一个 DTS 比基准大的
                // 然后播放小于基准而且离基准最近的帧，并缓存下一帧
                // 由于处理时间较长，而且使用了互斥体（看，音频部分也用到了），所以可能有极少数的音频帧有异常（噪声啊之类的）
                // 当然也可以将这一段注释掉，采用 pFrame = frameq.count(); frameq.pop();，同时解除 if (packet_queue_get()) 的注释（和下面的一对大括号）
                // 但是无跳过的方法会导致视频严重滞后

                if (pNextFrame != NULL)
                {
                    pFrame = pNextFrame;
                    SDL_memcpy(&packet, &nextpacket, sizeof(AVPacket));
                    pNextFrame = NULL;
                }
                else
                {
                    pFrame = frameq.front();
                    frameq.pop();
                }
                while (!frameq.empty())
                {
                    pNextFrame = frameq.front();
                    frameq.pop();
                    packet_queue_get(&videoq, &nextpacket, 1);

                    if (nextpacket.dts <= audio_dts)
                    {
                        av_free_packet(&packet);
                        av_frame_free(&pFrame);
                        SDL_memcpy(&packet, &nextpacket, sizeof(AVPacket));
                        pFrame = pNextFrame;
                        pNextFrame = NULL;
                    }
                    else
                    {
                        break;
                    }
                }

                
                //pFrame = frameq.front();
                //frameq.pop();
                

                //cout << "vdts: " << packet.dts << " adts: " << audio_dts << endl;

                //if (packet_queue_get(&videoq, &packet, 1) >= 0)
                //{
                    sws_scale(params->sws_context, (const uint8_t* const*)pFrame->data,
                              pFrame->linesize, 0, params->vid_codec_context->height, g_pFrameYUV->data, g_pFrameYUV->linesize);

                    SDL_UpdateYUVTexture(params->texture, NULL, g_pFrameYUV->data[0], g_pFrameYUV->linesize[0],
                                         g_pFrameYUV->data[1], g_pFrameYUV->linesize[1], g_pFrameYUV->data[2], g_pFrameYUV->linesize[2]);

                    //SDL_RenderClear(params->renderer);
                    SDL_RenderCopy(params->renderer, params->texture, NULL, NULL);
                    SDL_RenderPresent(params->renderer);
                    // 可以使用 av_frame_clone() + 队列 实现“远程”dts 读取
                    //if (params->vid_codec_context->refcounted_frames)
                    //{
                    //    av_frame_unref(pFrame);
                    //}
                //}

                //cout << "--------------------------------------------------" << endl;
                //cout << "vidpts: " << packet.pts << " audpts: " << audio_pts << endl;
                //cout << "viddts: " << packet.dts << " auddts: " << audio_dts << endl;

                SDL_UnlockMutex(params->video_mutex);
            }
            else
            {
                //cout << "vdts: " << packet.dts << " adts: " << audio_dts << endl;

                // 如果当前帧应该用被缓存的帧，那就用，不要重新读取了

                sws_scale(params->sws_context, (const uint8_t* const*)pFrame->data,
                          pFrame->linesize, 0, params->vid_codec_context->height, g_pFrameYUV->data, g_pFrameYUV->linesize);

                SDL_UpdateYUVTexture(params->texture, NULL, g_pFrameYUV->data[0], g_pFrameYUV->linesize[0],
                                     g_pFrameYUV->data[1], g_pFrameYUV->linesize[1], g_pFrameYUV->data[2], g_pFrameYUV->linesize[2]);

                //SDL_RenderClear(params->renderer);
                SDL_RenderCopy(params->renderer, params->texture, NULL, NULL);
                SDL_RenderPresent(params->renderer);
                // 可以使用 av_frame_clone() + 队列 实现“远程”dts 读取
                if (params->vid_codec_context->refcounted_frames)
                {
                    av_frame_unref(pFrame);
                }

                // 如果该帧是在音频帧之前的，那就销毁它，和数据包
                if (packet.dts <= audio_dts)
                {
                    av_frame_free(&pFrame);
                    av_free_packet(&packet);
                    pFrame = NULL;
                }
            }

        }
    }

    return 0;
}

int main(int argc, char *argv[]){
    // setlocale(LC_ALL, "");
    // setlocale(LC_ALL, "zh_CN.UTF-8");        

    printf("%s \n", "SDL2 play audio");
    // av_register_all();    // 4.0以后不用了 //注册了所有的文件格式和编解码的库，它们将被自动的使用在被打开的合适格式的文件上
    AVFormatContext *pFormatCtx;
    pFormatCtx = avformat_alloc_context();

    if (argc < 2) return 0;  // 参数检测
    printf("%s %s\n", "播放文件", argv[1]);
    //Open an input stream and read the header
    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0){
        printf("Can't open the file\n");
        return -1;
    }
    //Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        printf("Couldn't find stream information.\n");
        return -1;
    }
    printf("%s %s\n", "播放文件", "信息------");
    av_dump_format(pFormatCtx, 0, argv[1], 0);
    printf("%s %s\n", "-------", "------");


    int i, videoIndex, audioIndex;
    videoIndex = -1;
    audioIndex = -1;
    // nb_streams 是流个数, 分析流是视频还是音频 保留索引
    // 4.0 codec过时 使用codecpar
    for (i = 0; i < pFormatCtx->nb_streams; i++){//视音频流的个数
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
            && videoIndex < 0){
            videoIndex = i;
        }
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO
            && audioIndex < 0)
            audioIndex = i;
    }
    printf("%s \n", "索引");
    printf("%s %d\n", "videoIndex =", videoIndex);
    printf("%s %d\n", "audioIndex =", audioIndex);


    if (videoIndex == -1)
        return -1;
    if (audioIndex == -1)
        return -1;

    AVCodecParameters *pCodecParam, *paCodecParam; // 参数
    AVCodecContext *pCodecCtx, *paCodecCtx; // 上下文
    AVCodec *pCodec, *paCodec; // 编码
    //Get a pointer to the codec context for the video stream
    //流中关于编解码器的信息就是被我们叫做"codec context"（编解码器上下文）
    //的东西。这里面包含了流中所使用的关于编解码器的所有信
    pCodecParam = pFormatCtx->streams[videoIndex]->codecpar;
    // 帧引用：打开，见 AVFrame 的注释
    // pCodecCtx->refcounted_frames = 1; // 4.0 codec过时 老版本先预加载1帧？然后取codec
    paCodecParam = pFormatCtx->streams[audioIndex]->codecpar;
    //Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecParam->codec_id);
    paCodec = avcodec_find_decoder(paCodecParam->codec_id);

    pCodecCtx = avcodec_alloc_context3(pCodec);//需要使用avcodec_free_context释放
    avcodec_parameters_to_context(pCodecCtx, pCodecParam);

    paCodecCtx = avcodec_alloc_context3(paCodec);//需要使用avcodec_free_context释放
    avcodec_parameters_to_context(paCodecCtx, paCodecParam);



    if (pCodec == NULL || paCodecParam == NULL){  // 从视频角度出发，不合适就不解码
        printf("Unsupported codec!\n");
        return -1;
    }

    // 找视频解码器
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        printf("Could not open video codec.\n");
        return -1;
    }
    // 找音频解码器
    if (avcodec_open2(pCodecCtx, paCodec, NULL) < 0){
        printf("Could not open audio codec.\n");
        return -1;
    }


    //--------------------------------------------------------//
    printf("比特率 %3lld\n", pFormatCtx->bit_rate);
    printf("解码器名称 %s\n", paCodec->long_name);
    printf("time_base  %d \n", paCodecCtx->time_base);
    printf("声道数  %d \n", paCodecParam->channels);
    printf("sample per second  %d \n", paCodecParam->sample_rate);
    //--------------------------------------------------------//
    printf("pCodecParam width  %d \n", pCodecParam->width);
    printf("pCodecCtx   width  %d \n", pCodecCtx->width);
    printf("pCodecCtx->pix_fmt  %d \n", pCodecCtx->pix_fmt);
    printf("pCodecParam->format  %d \n", pCodecParam->format);
    
    //

    //allocate video frame and set its fileds to default value
    AVFrame *pFrame;
    //AVFrame *pFrameYUV;
    pFrame = av_frame_alloc();
    g_pFrameYUV = av_frame_alloc();


    //即使我们申请了一帧的内存，当转换的时候，我们仍然需要一个地方来放置原始
    //的数据。我们使用avpicture_get_size 来获得我们需要的大小， 然后手工申请
    //内存空间：
    uint8_t *out_buffer;
    int numBytes;
    numBytes = avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecParam->width, pCodecParam->height);
    //av_malloc 是ffmpeg 的malloc，用来实现一个简单的malloc 的包装，这样来保
    //证内存地址是对齐的（4 字节对齐或者2 字节对齐）。它并不能保 护你不被内
    //存泄漏，重复释放或者其它malloc 的问题所困扰。
    out_buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
    //Assign appropriate parts of buffer to image planes in pFrameYUV
    //Note that pFrameYUV is an AVFrame, but AVFrame is a superset of AVPicture
    avpicture_fill((AVPicture*)g_pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecParam->width, pCodecParam->height);

    //----------------SDL--------------------------------------//
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
        printf("Could not initialize SDL -%s\n", SDL_GetError());
        exit(1);
    }
    //先设置声音的选项：采样率，声音通道数和其它的参数，
    //然后设置一个回调函数和一些用户数据userdata。当开始播放音频的时候，
    //SDL 将不断地调用这个回调函数并且要求它来向声音缓冲填入一个特定的数量的字节。

    //把这些信息放到SDL_AudioSpec结构体中后，
    //调用函数SDL_OpenAudio()就会打开声音设备并且给我们送回另外一个AudioSpec 结构体。
    //这个结构体是我们实际上用到的－－因为我们不能保证得到我们所要求的。
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = paCodecParam->sample_rate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = paCodecParam->channels;    //声音的通道数
    wanted_spec.silence = 0;                          //用来表示静音的值
    wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;      //声音缓冲区的大小
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = paCodecCtx;

    if (SDL_OpenAudio(&wanted_spec, NULL) < 0){
        printf("SDL_OpenAudio error: %s\n", SDL_GetError());
        return -1;
    }

    packet_queue_init(&audioq);
    packet_queue_init(&videoq); 
    SDL_PauseAudio(0);
    
    SDL_Window *window = NULL;
    window = SDL_CreateWindow("MPlayer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              pCodecParam->width, pCodecParam->height, SDL_WINDOW_SHOWN);
    if (!window){
        printf("Could not initialize SDL_Window -%s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *ren = NULL;
    ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == NULL){
        printf("Could not initialize SDL_Renderer -%s\n", SDL_GetError());
        return -1;
    }

    SDL_Texture *texture = NULL;
    texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_YV12,
                                SDL_TEXTUREACCESS_STREAMING, pCodecParam->width, pCodecParam->height);


    //*************************************************************//
    //通过读取包来读取整个视频流，然后把它解码成帧，最后转换格式并且保存
    int frameFinished;
    //int psize = pCodecCtx->width * pCodecCtx->height;
    AVPacket packet;
    av_new_packet(&packet, numBytes);

    i = 0;
    int ret;
    static struct SwsContext *img_convert_ctx;  // 图像转码
    
    img_convert_ctx = sws_getContext(pCodecParam->width, pCodecParam->height,
                                     (AVPixelFormat)(pCodecParam->format), pCodecParam->width, pCodecParam->height, AV_PIX_FMT_YUV420P,
                                     SWS_BICUBIC, NULL, NULL, NULL);

    printf("pCodecCtx->pix_fmt  %d\n", pCodecCtx->pix_fmt);
    printf("(AVPixelFormat)(pCodecParam->format) %d\n", (AVPixelFormat)(pCodecParam->format));


    SDL_Event ev;
    // 视频渲染
    video_thread_params vtp;
    vtp.renderer = ren;
    vtp.texture = texture;
    vtp.sws_context = img_convert_ctx;
    vtp.vid_codec_context = pCodecCtx;
    vtp.video_mutex = SDL_CreateMutex();
    g_pVideoMutex = vtp.video_mutex;
    g_pVideoThread = SDL_CreateThread(video_thread_proc, "video_thread", &vtp); // 原demo使用了queue 这里注释了代码 执行了空方法


    double v_a_ratio;            // 视频帧数/音频帧数 帧数比率
    int frame_queue_size;

    printf("---------------------------------------\n");


    // 开启播放循环
    //Read the next frame of a stream
    while ((!quit) 
            && (
                av_read_frame(pFormatCtx, &packet) >= 0 // 抽取一包数据
                || (!frameq.empty()) 
                )
    )
    {
        //Is this a packet from the video stream?
        if (packet.stream_index == videoIndex)
        {
            //decode video frame of size packet.size from packet.data into picture
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            if(ret>=0) { //解码出内容
                printf("-解码出内容--------%d\n", ret);
                printf("-frameFinished--------%d\n", frameFinished);
                //Convert the image from its native format to YUV
                if (frameFinished){ 
                    // 用互斥体保持同步，将包送入队列，让另外的线程自己处理
                    SDL_LockMutex(vtp.video_mutex);
                    packet_queue_put(&videoq, &packet);
                    AVFrame *pNewFrame = av_frame_clone(pFrame);
                    frameq.push(pNewFrame);
                    printf("-Pushing vpacket.--------%d\n", frameFinished);
                    SDL_UnlockMutex(vtp.video_mutex);
                }
                // 注意这里也必须要 free packet，否则会导致严重内存泄露
                // 我修改了 packet_queue_put() 函数，它会复制 packet，所以可以放心释放上
                av_free_packet(&packet);
            } else { // 解码失败
                av_free_packet(&packet);
                printf("decode error \n");
                return -1;
            }
        }
        else if (packet.stream_index == audioIndex){
            packet_queue_put(&audioq, &packet);
        }
        else
        {
            av_free_packet(&packet);
        }


    //goto节点
process_sdl_events:
        if (SDL_PollEvent(&ev))
        {
            switch (ev.type){
                case SDL_QUIT:
                {
                    quit = 1;
                    video_quit = 1;
                    SDL_Quit();
                    goto exit_line;
                    break;
                }
                case SDL_KEYDOWN:
                    if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                    {
                        quit = 1;
                        video_quit = 1;
                        SDL_Quit();
                        goto exit_line;
                        break;
                    }
                default:
                    break;
            }
        }

        if (frameq.size() > 50) goto process_sdl_events;
    }

exit_line:

    SDL_DestroyMutex(vtp.video_mutex);

    SDL_DestroyTexture(texture);

    av_frame_free(&pFrame);
    av_frame_free(&g_pFrameYUV);

    avcodec_close(pCodecCtx);
    avcodec_close(paCodecCtx);

    avformat_close_input(&pFormatCtx);

    return 0;

}