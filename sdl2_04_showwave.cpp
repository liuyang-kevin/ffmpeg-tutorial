/*** 
 * 在ffmpeg中,进行反交错需要用到avfilter,即图像过滤器,ffmpeg中有很多过滤器,很强大,反交错的过滤器是yadif.
 * 基本的过滤器使用流程是:
 *      解码后的画面--->buffer过滤器---->其他过滤器---->buffersink过滤器--->处理完的画面
 * 所有的过滤器形成了过滤器链,一定要的两个过滤器是buffer过滤器和buffersink过滤器,
 * 前者的作用是将解码后的画面加载到过滤器链中,
 * 后者的作用是将处理好的画面从过滤器链中读取出来.
 * 那么进行反交错的过滤器链应该是这样的: buffer过滤器--->yadif过滤器--->buffersink过滤器
 *      过滤器相关的结构体:
 *          AVFilterGraph: 管理所有的过滤器图像
 *          AVFilterContext: 过滤器上下文
 *          AVFilter: 过滤器
 * 
 * CLI
 * clear && make && ./bin/sdl2_04_showwave.out ./test.mp4
 */

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswresample/swresample.h"
#include "libavfilter/generate_wave_table.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/opt.h"
}
using namespace std;

#include <fstream>


static AVFormatContext *fmt_ctx;
static AVCodecContext *dec_ctx;
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;
static int audio_stream_index = -1;

static int open_input_file(const char *filename)
{
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the audio stream */
    ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream in the input file\n");
        return ret;
    }
    audio_stream_index = ret;

    /* create decoding context */
    dec_ctx = avcodec_alloc_context3(dec);
    if (!dec_ctx)
        return AVERROR(ENOMEM);
    avcodec_parameters_to_context(dec_ctx, fmt_ctx->streams[audio_stream_index]->codecpar);

    /* init the audio decoder */
    if ((ret = avcodec_open2(dec_ctx, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret = 0;
    // showwaves
    // showwavespic  ff_avf_showwavespic
    const AVFilter *abuffersrc  = avfilter_get_by_name("abuffer");
    const AVFilter *abuffersink = avfilter_get_by_name("abuffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs  = avfilter_inout_alloc();

    static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE };
    // static const enum AVSampleFormat out_sample_fmts[] = { AV_SAMPLE_FMT_U8, AV_SAMPLE_FMT_NONE };
    static const int64_t out_channel_layouts[] = { AV_CH_LAYOUT_MONO, -1 };
    static const int out_sample_rates[] = { 8000, -1 };
    const AVFilterLink *outlink;
    // AVRational  「num，den」Numerator | Denominator分子和分母
    // 时间的基本单位 以秒为单位 
    AVRational time_base = fmt_ctx->streams[audio_stream_index]->time_base;

    filter_graph = avfilter_graph_alloc();
    if (!outputs || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    printf("\n\ntime_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64"\n\n", 
            time_base.num, time_base.den, dec_ctx->sample_rate,
            av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);

    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    if (!dec_ctx->channel_layout)
        dec_ctx->channel_layout = av_get_default_channel_layout(dec_ctx->channels);
    snprintf(args, sizeof(args),
            "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
             time_base.num, time_base.den, dec_ctx->sample_rate,
             av_get_sample_fmt_name(dec_ctx->sample_fmt), dec_ctx->channel_layout);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, abuffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        goto end;
    }

    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
        goto end;
    }

    

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs->name       = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name       = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx    = 0;
    inputs->next       = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        goto end;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        goto end;

    /* Print summary of the sink buffer
     * Note: args buffer is reused to store channel layout string */
    outlink = buffersink_ctx->inputs[0];
    av_get_channel_layout_string(args, sizeof(args), -1, outlink->channel_layout);
    av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
           (int)outlink->sample_rate,
           (char *)av_x_if_null(av_get_sample_fmt_name(static_cast<AVSampleFormat>(outlink->format)), "?"),
           args);

end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs);

    return ret;
}

ofstream fileCh1, fileCh2;
static void print_frame(const AVFrame *frame)
{
    

    const int n = frame->nb_samples * av_get_channel_layout_nb_channels(frame->channel_layout);
    const uint16_t *p     = (uint16_t*)frame->data[0];
    const uint16_t *p_end = p + n;

    // printf("%d, %d, %d, %x, %d\n", int8_t(*p >> 8), av_get_channel_layout_nb_channels(frame->channel_layout), 
    //             frame->nb_samples, &*p, int16_t(*p)/255);
    fileCh1 << "\n";
    fileCh2 << "\n";
    int8_t d = 0;
    while (p < p_end) {
        int8_t tmp = int8_t(*p >> 8);
        if(abs(tmp)>abs(d))
            d = tmp;
        // printf("%x, %x, %d | %d\n", &*p, &*p_end, n, (&*p_end - &*p));
        // printf("%d, %d, %d, %x\n", int16_t(*p), av_get_channel_layout_nb_channels(frame->channel_layout), 
        //         frame->nb_samples, &*p);
                
        // fputc(*p    & 0xff, stdout);
        // fputc(*p>>8 & 0xff, stdout);
        p++;
    }
    // fflush(stdout); //fflush()会强迫将缓冲区内的数据写回参数stream 指定的文件中. 如果参数stream 为NULL,fflush()会将所有打开的文件数据更新.
    printf("%d\n", d);
}

static int8_t maxValue(const AVFrame *frame)
{
    const int n = frame->nb_samples * av_get_channel_layout_nb_channels(frame->channel_layout);
    const uint16_t *p     = (uint16_t*)frame->data[0];
    const uint16_t *p_end = p + n;

    int8_t d = 0;
    while (p < p_end) {
        int8_t tmp = int8_t(*p >> 8);
        if(abs(tmp)>abs(d))
            d = tmp;
        p++;
    }
    // printf("%d\n", d);
    return d;
}


int main(int argc, char *argv[])
{

    fileCh1.open("ch1.csv");
    fileCh2.open("ch2.csv");

    const char *player = "ffplay -f s16le -ar 8000 -ac 1 -";
    const char *filter_descr = "aresample=8000,aformat=sample_fmts=s16:channel_layouts=mono";
    // const char *filter_descr = "aresample=8000,aformat=sample_fmts=u16:channel_layouts=mono";

    const char *input_filename = argv[1];
    int ret;
    AVPacket packet;
    AVFrame *frame = av_frame_alloc();       // 解码帧
    AVFrame *filt_frame = av_frame_alloc();  // 复用的容器

    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file | %s\n", argv[0], player);
        exit(1);
    }
    int8_t *vs = NULL;
    int vs_count = 0;

    if ((ret = open_input_file(argv[1])) < 0)
        goto end;
    if ((ret = init_filters(filter_descr)) < 0)
        goto end;

    

    /* read all packets */
    while (1) {
        if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
            break;

        if (packet.stream_index == audio_stream_index) {
            ret = avcodec_send_packet(dec_ctx, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error while sending a packet to the decoder\n");
                break;
            }

            while (ret >= 0) {
                ret = avcodec_receive_frame(dec_ctx, frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                    break;
                } else if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while receiving a frame from the decoder\n");
                    goto end;
                }

                if (ret >= 0) {
                    // 
                    /* push the audio data from decoded frame into the filtergraph */
                    if (av_buffersrc_add_frame_flags(buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0) {
                        av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
                        break;
                    }

                    /* pull filtered audio from the filtergraph */
                    while (1) {
                        ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                            break;
                        if (ret < 0)
                            goto end;
                        
                        // printf("%d, %d, %d\n", filt_frame->channel_layout, filt_frame->nb_samples, filt_frame->sample_rate);
                        // print_frame(filt_frame);
                        int8_t v = maxValue(filt_frame);
                        vs = (int8_t *)(realloc(vs, (vs_count+1)*sizeof(int8_t)));
                        vs[vs_count] = v;
                        vs_count++;
                        av_frame_unref(filt_frame);
                    }





                    av_frame_unref(frame);
                }
            }
        }
        av_packet_unref(&packet);

    }
end:
    fileCh1.close();
    fileCh2.close();

    avfilter_graph_free(&filter_graph);
    avcodec_free_context(&dec_ctx);
    avformat_close_input(&fmt_ctx);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }


        printf("%d, %d", vs[0],vs[50]);

    exit(0);
}