/**
 * 摘抄
 * 
 * 没调
*/
extern "C"
{
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/frame.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
}
 
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avdevice.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "swresample.lib")
 
 
/* PCM转AAC */
int main()
{
	/* ADTS头 */
	char *padts = (char *)malloc(sizeof(char) * 7);
	int profile = 2;	//AAC LC
	int freqIdx = 4;  //44.1KHz
	int chanCfg = 2;  //MPEG-4 Audio Channel Configuration. 1 Channel front-center,channel_layout.h
	padts[0] = (char)0xFF; // 11111111     = syncword
	padts[1] = (char)0xF1; // 1111 1 00 1  = syncword MPEG-2 Layer CRC
	padts[2] = (char)(((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
	padts[6] = (char)0xFC;
 
	SwrContext *swr_ctx = NULL;
	AVCodecContext *pCodecCtx = NULL;
	AVCodec *pCodec = NULL;
	AVFrame *pFrame;
	AVPacket pkt;
	AVCodecID codec_id = AV_CODEC_ID_AAC;
 
	FILE *fp_in;
	FILE *fp_out;
	char filename_in[] = "audio.pcm";
	char filename_out[] = "audio.aac";
 
	uint8_t **convert_data;			//存储转换后的数据，再编码AAC
	int i, ret, got_output;
	uint8_t* frame_buf;
	int size = 0;
	int y_size;
	int framecnt = 0;
	int framenum = 100000;
 
 
	avcodec_register_all();
 
	pCodec = avcodec_find_encoder(codec_id);
	if (!pCodec) {
		printf("Codec not found\n");
		return -1;
	}
 
	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (!pCodecCtx) {
		printf("Could not allocate video codec context\n");
		return -1;
	}
 
	pCodecCtx->codec_id = codec_id;
	pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
	pCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
	pCodecCtx->sample_rate = 44100;
	pCodecCtx->channel_layout = AV_CH_LAYOUT_STEREO;
	pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
 
 
 
	qDebug() << "pCodecCtx->bit_rate ----------------> " << pCodecCtx->bit_rate;
	qDebug() << "pCodecCtx->bit_rate ----------------> " << pCodecCtx->bit_rate;
	qDebug() << av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
 
 
	if ((ret = avcodec_open2(pCodecCtx, pCodec, NULL)) < 0) {
		qDebug() << "avcodec_open2 error ----> " << ret;
 
		printf("Could not open codec\n");
		return -1;
	}
 
	pFrame = av_frame_alloc();
	pFrame->nb_samples = pCodecCtx->frame_size;	//1024
	pFrame->format = pCodecCtx->sample_fmt;
	pFrame->channels = 2;
	qDebug() << "frame_size(set pFrame->nb_samples) -------------> " << pCodecCtx->frame_size;
 
 
	/* 由AV_SAMPLE_FMT_FLT转为AV_SAMPLE_FMT_FLTP */
	swr_ctx = swr_alloc_set_opts(
		NULL,
		av_get_default_channel_layout(pCodecCtx->channels),
		pCodecCtx->sample_fmt,				    //在编码前，我希望的采样格式
		pCodecCtx->sample_rate,
		av_get_default_channel_layout(pCodecCtx->channels),
		AV_SAMPLE_FMT_FLT,					    //PCM源文件的采样格式
		pCodecCtx->sample_rate,
		0, NULL);
 
	swr_init(swr_ctx);
	/* 分配空间 */
	convert_data = (uint8_t**)calloc(pCodecCtx->channels,
		sizeof(*convert_data));
	av_samples_alloc(convert_data, NULL,
		pCodecCtx->channels, pCodecCtx->frame_size,
		pCodecCtx->sample_fmt, 0);
 
 
 
	size = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size, pCodecCtx->sample_fmt, 0);
	
	frame_buf = (uint8_t *)av_malloc(size);
	/* 此时data[0],data[1]分别指向frame_buf数组起始、中间地址 */
	ret = avcodec_fill_audio_frame(pFrame, pCodecCtx->channels, pCodecCtx->sample_fmt, (const uint8_t*)frame_buf, size, 0);
	
	if (ret < 0)
	{
		qDebug() << "avcodec_fill_audio_frame error ";
		return 0;
	}
 
	//Input raw data
	fp_in = fopen(filename_in, "rb");
	if (!fp_in) {
		printf("Could not open %s\n", filename_in);
		return -1;
	}
 
	//Output bitstream
	fp_out = fopen(filename_out, "wb");
	if (!fp_out) {
		printf("Could not open %s\n", filename_out);
		return -1;
	}
 
	//Encode
	for (i = 0; i < framenum; i++) {
		av_init_packet(&pkt);
		pkt.data = NULL;    // packet data will be allocated by the encoder
		pkt.size = 0;
		//Read raw data
		if (fread(frame_buf, 1, size, fp_in) <= 0) {
			printf("Failed to read raw data! \n");
			return -1;
		}
		else if (feof(fp_in)) {
			break;
		}
		/* 转换数据，令各自声道的音频数据存储在不同的数组（分别由不同指针指向）*/
		swr_convert(swr_ctx, convert_data, pCodecCtx->frame_size,
			(const uint8_t**)pFrame->data, pCodecCtx->frame_size);
 
		/* 将转换后的数据复制给pFrame */
		int length = pCodecCtx->frame_size * av_get_bytes_per_sample(pCodecCtx->sample_fmt);
		for (int k = 0; k < 2; ++k)
		for (int j = 0; j < length; ++j)
		{
			pFrame->data[k][j] = convert_data[k][j];
		}
 
		pFrame->pts = i;
 
		qDebug() << "frame->nb_samples -----> " << pFrame->nb_samples;
		qDebug() << "size ------------------> " << size;
		qDebug() << "frame->linesize[0] ----> " << pFrame->linesize[0];
 
 
 
		ret = avcodec_encode_audio2(pCodecCtx, &pkt, pFrame, &got_output);
 
		if (ret < 0) {
			qDebug() << "error encoding";
			return -1;
		}
 
		if (pkt.data == NULL)
		{
			av_free_packet(&pkt);
			continue;
		}
 
		qDebug() << "got_ouput = " << got_output;
		if (got_output) {
			qDebug() << "Succeed to encode frame : " << framecnt << " size :" << pkt.size;
 
			framecnt++;
 
			padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + pkt.size) >> 11));
			padts[4] = (char)(((7 + pkt.size) & 0x7FF) >> 3);
			padts[5] = (char)((((7 + pkt.size) & 7) << 5) + 0x1F);
			fwrite(padts, 7, 1, fp_out);
			fwrite(pkt.data, 1, pkt.size, fp_out);
 
			av_free_packet(&pkt);
		}
	}
	//Flush Encoder
	for (got_output = 1; got_output; i++) {
		ret = avcodec_encode_audio2(pCodecCtx, &pkt, NULL, &got_output);
		if (ret < 0) {
			printf("Error encoding frame\n");
			return -1;
		}
		if (got_output) {
			printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", pkt.size);
			padts[3] = (char)(((chanCfg & 3) << 6) + ((7 + pkt.size) >> 11));
			padts[4] = (char)(((7 + pkt.size) & 0x7FF) >> 3);
			padts[5] = (char)((((7 + pkt.size) & 7) << 5) + 0x1F);
 
			fwrite(padts, 7, 1, fp_out);
			fwrite(pkt.data, 1, pkt.size, fp_out);
			av_free_packet(&pkt);
		}
	}
 
	fclose(fp_out);
	avcodec_close(pCodecCtx);
	av_free(pCodecCtx);
	av_freep(&pFrame->data[0]);
	av_frame_free(&pFrame);
 
	av_freep(&convert_data[0]);	
	free(convert_data);			
	//////////////////////////////////////////////////////////////////////////////////
 
	return 0;
}