// tutorial02.c
// A pedagogical video player that will stream through every video frame as fast as it can.
//
// This tutorial was written by robin (ziyi.001@163.com).
//
// Code based on FFplay, Copyright (c) 2003 Fabrice Bellard, 
// and a tutorial by Martin Bohme (boehme@inb.uni-luebeckREMOVETHIS.de)
// Tested on Gentoo, CVS version 5/01/07 compiled with GCC 4.1.1
//
// Use the Makefile to build all examples.
//
// Run using
// tutorial02 myvideofile.mpg
//
// to play the video stream on your screen.


#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

//#import "SDL.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_thread.h"

#ifdef __MINGW32__
#undef main /* Prevents SDL from overriding main() */
#endif

#include <stdio.h>

int main(int argc, char *argv[]) {
  AVFormatContext *pFormatCtx = NULL;
  int             i, videoStream;
  AVCodecContext  *pCodecCtx = NULL;
  AVCodec         *pCodec = NULL;
  AVFrame         *pFrame = NULL; 
  AVPacket        packet;
  int             frameFinished;
  //float           aspect_ratio;

  AVDictionary    *optionsDict = NULL;
    
  SDL_Window    *m_window = NULL;
  SDL_Renderer *m_renderer = NULL;
    
  SDL_Rect        rect;
  SDL_Event       event;

  if(argc < 2) {
    fprintf(stderr, "Usage: test <file>\n");
    exit(1);
  }
  // Register all formats and codecs
  av_register_all();
  
  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  // Open video file
  if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
    return -1; // Couldn't open file
  
  // Retrieve stream information
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; // Couldn't find stream information
  
  // Dump information about file onto standard error
  av_dump_format(pFormatCtx, 0, argv[1], 0);
  
  // Find the first video stream
  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }
  if(videoStream==-1)
    return -1; // Didn't find a video stream
  
  // Get a pointer to the codec context for the video stream
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  // Find the decoder for the video stream
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL)
  {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; // Codec not found
  }
  
  // Open codec
    if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return -1; // Could not open codec
  
    // Allocate video frame
    pFrame=av_frame_alloc();
    AVFrame*   m_pFrameYUV = av_frame_alloc();
    uint8_t *  out_buffer = (uint8_t *)av_malloc(avpicture_get_size(PIX_FMT_YUV420P, pCodecCtx->coded_width, pCodecCtx->coded_height));
    avpicture_fill((AVPicture *)m_pFrameYUV , out_buffer, PIX_FMT_YUV420P, pCodecCtx->coded_width, pCodecCtx->coded_height);
    struct SwsContext *img_convert_ctx = sws_getContext(pCodecCtx->coded_width, pCodecCtx->coded_height,pCodecCtx->sw_pix_fmt,
                                                     pCodecCtx->coded_width, pCodecCtx->coded_height,AV_PIX_FMT_YUV420P,
                                                     SWS_BICUBIC,
                                                     NULL, NULL, NULL);
    
    // Make a screen to put our video
    m_window = SDL_CreateWindow("test windows", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,pCodecCtx->coded_width, pCodecCtx->coded_height,SDL_WINDOW_SHOWN);
    if(!m_window)
    {
        printf("SDL: could not create window - exiting:%s\n",SDL_GetError());
        return -1;
    }
    
    m_renderer = SDL_CreateRenderer(m_window, -1, 0);
    SDL_RenderClear(m_renderer);
    SDL_Texture *m_pSdlTexture = SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
                                                   pCodecCtx->coded_width, pCodecCtx->coded_height);
    
    rect.x = 0;
    rect.y = 0;
    rect.w = pCodecCtx->coded_width;
    rect.h = pCodecCtx->coded_height;
   
    
  // Read frames and save first five frames to disk
  while(av_read_frame(pFormatCtx, &packet)>=0)
  {
    // Is this a packet from the video stream?
    if(packet.stream_index==videoStream)
    {
      // Decode video frame
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
      
      // Did we get a video frame?
      if(frameFinished)
      {
          // Convert the image into YUV format that SDL uses
          sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->coded_height,
                    m_pFrameYUV->data, m_pFrameYUV->linesize);
          
    
          SDL_UpdateYUVTexture(m_pSdlTexture, &rect,
                         m_pFrameYUV->data[0], m_pFrameYUV->linesize[0],
                         m_pFrameYUV->data[1], m_pFrameYUV->linesize[1],
                         m_pFrameYUV->data[2], m_pFrameYUV->linesize[2]);

          //SDL_RenderClear( m_renderer );//this line seems nothing to do
          SDL_RenderCopy( m_renderer, m_pSdlTexture,  NULL, &rect);

          SDL_RenderPresent(m_renderer);
          
          SDL_Delay(20);
      }
    }
    
    // Free the packet that was allocated by av_read_frame
    av_free_packet(&packet);
    SDL_PollEvent(&event);
    switch(event.type)
      {
        case SDL_QUIT:
          SDL_Quit();
          exit(0);
          break;
        default:
          break;
    }

  }
    
  sws_freeContext(img_convert_ctx);
  av_frame_free(&m_pFrameYUV);
    
  // Free the YUV frame
  av_free(pFrame);
  
  // Close the codec
  avcodec_close(pCodecCtx);
  
  // Close the video file
  avformat_close_input(&pFormatCtx);
  
  SDL_DestroyWindow(m_window);
    
    
  return 0;
}
