//ffmpeg_audio_player
#include <stdlib.h>
#include <string.h>
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
//SDL
#include "sdl/SDL.h"
#include "sdl/SDL_thread.h"
};

	static  Uint8  *audio_chunk; 
	static  Uint32  audio_len; 
	static  Uint8  *audio_pos; 
//-----------------
	/*  The audio function callback takes the following parameters: 
	stream: A pointer to the audio buffer to be filled 
	len: The length (in bytes) of the audio buffer (���ǹ̶���4096��)
	�ص�����
	ע�⣺mp3Ϊʲô���Ų�˳����
	len=4096;audio_len=4608;�������512��Ϊ����512�������ٵ���һ�λص�����������
	m4a,aac�Ͳ����ڴ�����(����4096)��
	*/ 
	void  fill_audio(void *udata,Uint8 *stream,int len){ 
		/*  Only  play  if  we  have  data  left  */ 
	if(audio_len==0) 
			return; 
		/*  Mix  as  much  data  as  possible  */ 
	len=(len>audio_len?audio_len:len); 
	SDL_MixAudio(stream,audio_pos,len,SDL_MIX_MAXVOLUME);
	audio_pos += len; 
	audio_len -= len; 
	} 
//-----------------
	
int decode_audio(const char* url)
{
	AVFormatContext	*pFormatCtx;
	int				i, audioStream;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;

	//Register all available file formats and codecs
	av_register_all();

	//��ʼ��
	pFormatCtx = avformat_alloc_context();
	if(avformat_open_input(&pFormatCtx,url,NULL,NULL)!=0){
		printf("Couldn't open file.\n");
		return -1;
	}
	
	// Retrieve stream information
	if(av_find_stream_info(pFormatCtx)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	// Dump valid information onto standard error
	av_dump_format(pFormatCtx, 0, url, false);

	// Find the first audio stream
	audioStream=-1;
	for(i=0; i < pFormatCtx->nb_streams; i++)
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
		{
			audioStream=i;
			break;
		}

	if(audioStream==-1)
	{
		printf("Didn't find a audio stream.\n");
		return -1;
	}

	// Get a pointer to the codec context for the audio stream
	pCodecCtx=pFormatCtx->streams[audioStream]->codec;

	// Find the decoder for the audio stream
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}

	// Open codec
	if (avcodec_open(pCodecCtx, pCodec)<0)
//	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	/********* For output file ******************/
	FILE *pFile;
	pFile=fopen("output.pcm", "wb");

	// Open the time stamp file
	FILE *pTSFile;
	pTSFile=fopen("audio_time_stamp.txt", "wb");
	if(pTSFile==NULL)
	{
		printf("Could not open output file.\n");
		return -1;
	}
	fprintf(pTSFile, "Time Base: %d/%d\n", pCodecCtx->time_base.num, pCodecCtx->time_base.den);

	/*** Write audio into file ******/
	//�ѽṹ���Ϊָ��
	AVPacket *packet=(AVPacket *)malloc(sizeof(AVPacket));
	av_init_packet(packet);

	//��Ƶ����Ƶ�������ͳһ��
	//�¼�
	AVFrame	*pFrame;
	pFrame=avcodec_alloc_frame();

	//---------SDL--------------------------------------
	//��ʼ��
	if(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
		printf( "Could not initialize SDL - %s\n", SDL_GetError()); 
		exit(1);
	}

	//�ṹ�壬����PCM���ݵ������Ϣ
	SDL_AudioSpec wanted_spec;
	wanted_spec.freq = pCodecCtx->sample_rate; 
	wanted_spec.format = AUDIO_S16SYS; 
	wanted_spec.channels = pCodecCtx->channels; 
	wanted_spec.silence = 0; 
	//wanted_spec.samples = 1024; //����AAC��M4a���������Ĵ�С
	wanted_spec.samples = 1152; //����MP3��WMAʱ����
	printf("%d", pFrame->nb_samples);
	wanted_spec.callback = fill_audio; 
	wanted_spec.userdata = pCodecCtx; 

	if (SDL_OpenAudio(&wanted_spec, NULL)<0)//���裨2������Ƶ�豸 
	{ 
		printf("can't open audio.\n"); 
		return 0; 
	} 
	//-----------------------------------------------------
	printf("������ %3d\n", pFormatCtx->bit_rate);
	printf("���������� %s\n", pCodecCtx->codec->long_name);
	printf("time_base  %d \n", pCodecCtx->time_base);
	printf("������  %d \n", pCodecCtx->channels);
	printf("sample per second  %d \n", pCodecCtx->sample_rate);

	uint32_t ret,len = 0;
	int got_picture;
	int index = 0;
	while(av_read_frame(pFormatCtx, packet)>=0)
	{
		if(packet->stream_index==audioStream)
		{
			ret = avcodec_decode_audio4( pCodecCtx, pFrame,
				&got_picture, packet);
			if ( ret < 0 ) // if error len = -1
			{
                printf("Error in decoding audio frame.\n");
                exit(0);
            }
			if ( got_picture > 0 )
			{
#if 1
				printf("index %3d\n", index);
				printf("pts %5d\n", packet->pts);
				printf("dts %5d\n", packet->dts);
				printf("packet_size %5d\n", packet->size);
				
#endif
				//ֱ��д��
				//ע�⣺������data��0����������linesize��0��
#if 1
				// write to PCM file
				fwrite(pFrame->data[0], 1, pFrame->linesize[0], pFile);
				//fwrite(pFrame, 1, got_picture, pFile);
				//len+=got_picture;
				index++;
#endif
			}
#if 1
			//---------------------------------------
			//printf("begin....\n"); 
			audio_chunk = (Uint8*) pFrame->data[0]; 
			//������Ƶ���ݳ���
			audio_len = pFrame->linesize[0];
			//����mp3��ʱ���Ϊaudio_len = 4096
			//���Ƚ���������������������MP3һ֡����4608
			//ʹ��һ�λص�������4096�ֽڻ��壩���Ų��꣬���Ի�Ҫʹ��һ�λص����������²��Ż���������
			//���ó�ʼ����λ��
			audio_pos = audio_chunk;
			//�ط���Ƶ���� 
			SDL_PauseAudio(0);
			//printf("don't close, audio playing...\n"); 
			while(audio_len>0)//�ȴ�ֱ����Ƶ���ݲ������! 
				SDL_Delay(1); 
			//---------------------------------------
#endif
		}
		// Free the packet that was allocated by av_read_frame
		av_free_packet(packet);
	}

	SDL_CloseAudio();//�ر���Ƶ�豸 
	// Close file
	fclose(pFile);
	// Close the codec
	avcodec_close(pCodecCtx);
	// Close the video file
	av_close_input_file(pFormatCtx);

	return 0;
}


int main(int argc, char* argv[])
{
	char filename[] = "./testaudios/voice.mp3";

	if (decode_audio(filename) == 0)
		printf("Decode audio successfully.\n");

	return 0;
}
