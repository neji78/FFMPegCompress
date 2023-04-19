extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
}
#include <string>
#include <iostream>
struct codec_information
{
    AVCodec *audio_codec;
    AVCodecParameters *audio_codec_parameters;
    AVCodecContext *audio_codec_context;
    AVCodec *video_codec;
    AVCodecParameters *video_codec_parameters;
    AVCodecContext *video_codec_context;
} ;
void debug(std::string message,int result);
void set_encoder_codec_contex(AVCodecContext *video_avcc,AVCodecContext *decoder_ctx,AVCodec *video_avc);
AVFormatContext * openMedia(char *inputFilePath);
void set_decoder(codec_information &decoder, AVFormatContext *inputFormatContext);
void set_encoder(codec_information decoder,codec_information &encoder);
int main()
{
    std::string inputFilePath = "C:/Users/pars/Desktop/compress/compress_1/test.mp4";
    std::string outputFilePath = "C:/Users/pars/Desktop/compress/compress_1/output.mp4";
    AVFormatContext* inputFormatContext = nullptr;
    int result,result_ = -1;

    inputFormatContext = openMedia((char *)inputFilePath.c_str());
    codec_information decoder;
    codec_information encoder;
    set_decoder(decoder,inputFormatContext);

    AVFormatContext* outputFormatContext = nullptr;
    result = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFilePath.c_str());
    if ( result< 0)
    {
        // error handling
        debug("avformat_alloc_output_context2",result);
    }

    set_encoder(decoder,encoder);
    //encoder
/*
    AVStream* stream = avformat_new_stream(outputFormatContext, video_avc);
    if (stream == nullptr)
    {
        // error handling
        debug("avformat_new_stream",-1);
    }

    result = avcodec_parameters_from_context(stream->codecpar, video_avcc);
    if(result < 0)
    {
        debug("avcodec_parameters_from_context",result);
    }

    if (avio_open(&outputFormatContext->pb, outputFilePath.c_str(), AVIO_FLAG_WRITE) < 0)
    {
        // error handling
        debug("avio_open",-1);
    }

    if (avformat_write_header(outputFormatContext, nullptr) < 0)
    {
        // error handling
        debug("avformat_write_header",-1);
    }
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;

    AVFrame* frame = av_frame_alloc();
    if (frame == nullptr)
    {
        // error handling
        std::cout<<"av_frame_alloc";
        exit(23);
    }
    AVFrame *input_frame = av_frame_alloc();
    AVPacket *input_packet = av_packet_alloc();

// used function

    while (av_read_frame(inputFormatContext, &packet) >= 0)
    {
        if (packet.stream_index == videoStreamIndex)
        {
            AVFrame* decodedFrame = av_frame_alloc();
            if (decodedFrame == nullptr)
            {
                // error handling
                debug("decodedFrame == nullpt",-1);
            }

            int frameFinished = 0;
            result = avcodec_send_packet(codecContext, &packet);
            result_ = avcodec_receive_frame(codecContext, decodedFrame);
            if ( result== 0 &&  result_== 0)
            {
                /*std::cout<<"\ndecoder frame:";
                std::cout<<"\nFrame "<<av_get_picture_type_char(decodedFrame->pict_type);
                std::cout<<" "<<codecContext->frame_number<<" pts "<<decodedFrame->pts;
                std::cout<<" dts "<<decodedFrame->pkt_dts<<" key_frame "<<decodedFrame->key_frame<<" [coded_picture_number ";
                std::cout<<decodedFrame->coded_picture_number<<" , display_picture_number "<<decodedFrame->display_picture_number<<"]"<<decodedFrame->width<<"x"<<decodedFrame->height<<" "<<decodedFrame->format;
                */
                //save_gray_frame(decodedFrame->data[0], decodedFrame->linesize[0], decodedFrame->width, decodedFrame->height, (char *)frame_filename.c_str());
                //av_frame_unref(frame);
                //av_frame_move_ref(frame, decodedFrame);
                //av_packet_unref(&packet);
                //frame.
                /*std::cout<<"\nencoder frame:";
                std::cout<<"\nFrame "<<av_get_picture_type_char(frame->pict_type);
                std::cout<<" "<<video_avcc->frame_number<<" pts "<<frame->pts;
                std::cout<<" dts "<<frame->pkt_dts<<" key_frame "<<frame->key_frame<<" [coded_picture_number ";
                std::cout<<frame->coded_picture_number<<" , display_picture_number "<<frame->display_picture_number<<"]"<<frame->width<<"x"<<frame->height<<" "<<frame->format;

                AVPacket encodedPacket;
                av_init_packet(&encodedPacket);
                encodedPacket.data = nullptr;
                encodedPacket.size = 0;
                result = avcodec_send_frame(video_avcc, frame);
                result_ = avcodec_receive_packet(video_avcc, &encodedPacket);
                if ( result== 0 &&  result_== 0)
                {
                    encodedPacket.stream_index = stream->index;
                    result = av_write_frame(outputFormatContext, &encodedPacket);

                    if ( result< 0)
                    {
                        // error handling
                        debug("decodedFrame == nullpt",result);
                    }

                    av_packet_unref(&encodedPacket);
                }
                else
                {

                    std::cout<<"\nSEND_FRAME_ERROR: ";
                    debug("avcodec_send_frame",result);
                }
            }
            else
            {
                std::cout<<"\nSEND_PACKET_ERROR: ";
                debug("avcodec_send_packet",result);
            }

            av_frame_free(&decodedFrame);
        }
    }*/
}
void set_encoder(codec_information decoder,codec_information &encoder)
{
    int result;
    //video encoder
    encoder.video_codec = avcodec_find_encoder_by_name(decoder.video_codec->name);
    encoder.video_codec_context = avcodec_alloc_context3(encoder.video_codec );
    if(encoder.video_codec == nullptr)
    {
        std::cout<<"\n video encoder not found";
        exit(200);
    }
    std::cout<<"\nvideo encoder name: "<<encoder.video_codec->name;
    set_encoder_codec_contex(encoder.video_codec_context,decoder.video_codec_context,encoder.video_codec);
    std::cout<<"\ntimeBase: "<<encoder.video_codec_context->time_base.den<<"(fps)"<< "\t"<<encoder.video_codec_context->time_base.num;
    result = avcodec_open2(encoder.video_codec_context, encoder.video_codec, NULL);
    if(result < 0)
    {
        debug("video encoder",result);
    }
    //audio encoder
    encoder.audio_codec = avcodec_find_encoder_by_name(decoder.audio_codec->name);
    encoder.audio_codec_context = avcodec_alloc_context3(encoder.audio_codec);
    if(encoder.audio_codec == nullptr)
    {
        std::cout<<"\naudio encoder not found";
        exit(200);
    }
    std::cout<<"\naudio encoder name: "<<encoder.audio_codec->name;
    result = avcodec_open2(encoder.audio_codec_context, encoder.audio_codec, NULL);
    if(result < 0)
    {
        debug("video encoder",result);
    }

    //avcodec_parameters_to_context(video_avcc, codecParams);
    //set_encoder_codec_contex(video_avcc,codecContext,video_avc);

}
void set_decoder(codec_information &decoder, AVFormatContext *inputFormatContext)
{
    int result;
    std::cout<<"\n--------------------------DECODERS--------------------------";
    std::cout<<"\nTYPE "<<"\tNAME "<<"\t\tID";
    AVCodecParameters *codecPar = nullptr;
    for(int i = 0;i < inputFormatContext->nb_streams;i++){
        codecPar = inputFormatContext->streams[i]->codecpar;
        if(codecPar->format == AVMEDIA_TYPE_VIDEO){
            decoder.video_codec = avcodec_find_decoder(codecPar->codec_id);
            decoder.video_codec_context = inputFormatContext->streams[i]->codec;
            decoder.video_codec_parameters = codecPar;
            std::cout<<"\nVIDEO"<<"\t"<<decoder.video_codec->long_name<<"\t"<<decoder.video_codec->id;
            std::cout<<"\nCODEC PROPERTIES: ";
            std::cout<<"width "<<decoder.video_codec_parameters->width <<" height "<< decoder.video_codec_parameters->height;
            std::cout<<" bit_rate "<<decoder.video_codec_parameters->bit_rate<<" frame_rate(fps,...) "<<decoder.video_codec_context->framerate.num<<","<<decoder.video_codec_context->framerate.den;
        }
        else{
            decoder.audio_codec = avcodec_find_decoder(codecPar->codec_id);
            decoder.audio_codec_context = inputFormatContext->streams[i]->codec;
            decoder.audio_codec_parameters = codecPar;
            std::cout<<"\nAUDIO"<<"\t"<<decoder.audio_codec->long_name<<"\t"<<decoder.audio_codec->id;
                        std::cout<<"\nCODEC PROPERTIES: ";
            std::cout<<"width "<<decoder.audio_codec_parameters->width <<" height "<< decoder.audio_codec_parameters->height;
            std::cout<<" bit_rate "<<decoder.audio_codec_parameters->bit_rate<<" frame_rate(fps,...) "<<decoder.audio_codec_context->framerate.num<<","<<decoder.audio_codec_context->framerate.den;
        }
    }
    result = avcodec_open2(decoder.video_codec_context, decoder.video_codec, NULL);
    if(result < 0)
    {
        debug("video codec",result);
    }
    result = avcodec_open2(decoder.audio_codec_context, decoder.audio_codec, NULL);
    if(result < 0)
    {
        debug("audio codec",result);
    }

}
AVFormatContext * openMedia(char *inputFilePath)
{
    AVFormatContext *inputFormatContext = nullptr;
    std::cout<<"OPENNING MEDIA FILE: "<<inputFilePath<<"\n";
    int result;
    result = avformat_open_input(&inputFormatContext, inputFilePath, nullptr, nullptr);
    if ( result != 0)
    {
        debug("avformat_open_input",result);
    }
    std::cout<<"Format "<<inputFormatContext->iformat->long_name<<"\nDuration: "<<inputFormatContext->duration;
    result = avformat_find_stream_info(inputFormatContext, nullptr);
    if (result < 0)
    {
        debug("avformat_find_stream_info",result);
    }
    std::cout<<"\nBitRate: "<<inputFormatContext->bit_rate;
    std::cout<<"\nMEDIA OPEN FINISHED";
    return inputFormatContext;
}
void debug(std::string message, int result)
{
    char error[500];
    av_strerror(result, error, 500);
    std::cout<<"\nhas a " + message + " error: "+error;
    exit(50);
}
void set_encoder_codec_contex(AVCodecContext *video_avcc,AVCodecContext *decoder_ctx,AVCodec *video_avc)
{
    video_avcc->height = decoder_ctx->height;
    video_avcc->width = decoder_ctx->width;
    video_avcc->pix_fmt = video_avc->pix_fmts[0];
// control rate
    video_avcc->bit_rate = decoder_ctx->bit_rate;
    video_avcc->rc_buffer_size = decoder_ctx->rc_buffer_size;
    video_avcc->rc_max_rate = decoder_ctx->rc_buffer_size;
    video_avcc->rc_min_rate = decoder_ctx->rc_buffer_size;
    video_avcc->flags = decoder_ctx->flags;
    video_avcc->time_base.den = decoder_ctx->time_base.den;
    video_avcc->time_base.num = decoder_ctx->time_base.num;

}
