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
    int video_stream_index;
    int audio_stream_index;
    AVPacket *packet;
    AVFrame *frame;
} ;
void debug(std::string message,int ret,bool doExit = true);
void set_video_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder);
void set_audio_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder);
AVFormatContext * openMedia(char *inputFilePath);
void set_decoder(codec_information &decoder, AVFormatContext *inputFormatContext);
void set_encoder(codec_information decoder,codec_information &encoder, AVFormatContext *ofmt_ctx);
void write_encoded_frame(codec_information &encoder, AVFormatContext *ofmt_ctx,AVFormatContext *ifmt_ctx, AVCodecContext *target);
int main()
{
    std::string inputFilePath = "test.mp4";
    std::string outputFilePath = "output.mp4";
    AVFormatContext* inputFormatContext = nullptr;
    int ret;
    inputFormatContext = openMedia((char *)inputFilePath.c_str());

    codec_information decoder;
    set_decoder(decoder,inputFormatContext);
    decoder.video_codec_context->framerate = av_guess_frame_rate(inputFormatContext, inputFormatContext->streams[decoder.video_stream_index], NULL);

    AVFormatContext* outputFormatContext = nullptr;
    ret = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFilePath.c_str());
    if ( ret< 0)
    {
        // error handling
        debug("avformat_alloc_output_context2",ret);
    }
    codec_information encoder;
    set_encoder(decoder,encoder,outputFormatContext);

    AVStream* video_stream = avformat_new_stream(outputFormatContext, encoder.video_codec);
    AVStream* audio_stream = avformat_new_stream(outputFormatContext, decoder.audio_codec);
    ret = avcodec_parameters_from_context(video_stream->codecpar, encoder.video_codec_context);
    if(ret < 0)
    {
        debug("video_stream avcodec_parameters_from_context",ret);
    }
    ret = avcodec_parameters_from_context(audio_stream->codecpar, decoder.audio_codec_context);
    if(ret < 0)
    {
        debug("audio_stream avcodec_parameters_from_context",ret);
    }
    video_stream->time_base.num = 128000;
    video_stream->time_base.den = 1;
    audio_stream->time_base = decoder.audio_codec_context->time_base;
    std::cout<<"\n";
    av_dump_format(outputFormatContext, 0, outputFilePath.c_str(), 1);

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
    decoder.packet = av_packet_alloc();
    decoder.packet->data = nullptr;
    decoder.packet->size = 0;
    decoder.frame = av_frame_alloc();
    std::cout<<"\n\t\t\t~~~~~~~~~~~~~~~~~~~Transcodeing~~~~~~~~~~~~~~~~~~~";
    std::cout<<"\nFrame"<<"\tPTS"<<"\tDTS"<<"\tKey_Frame"<<"\tCoded_Picture_Number"<<"\tDisplay_Picture_Number"<<"\tWidth"<<"\tHeight"<<"\tFormat";
    while(true)
    {
        ret = av_read_frame(inputFormatContext,decoder.packet);
        if(ret < 0)
        {
            debug("av_read_frame",ret,false);
            break;
        }
        if (decoder.packet->stream_index == decoder.video_stream_index)
        {
            ret = avcodec_send_packet(decoder.video_codec_context, decoder.packet);
            if(ret >= 0)
            {
                while(true)
                {
                    ret = avcodec_receive_frame(decoder.video_codec_context, decoder.frame);
                    if(ret < 0)
                    {
                        //debug("avcodec_receive_frame",ret,false);
                        break;
                    }
                    encoder.frame = decoder.frame;
                    encoder.packet = av_packet_alloc();
                    encoder.packet->data = nullptr;
                    encoder.packet->size = 0;
                    encoder.packet->stream_index = decoder.packet->stream_index;

                    std::cout<<"\n  "<<av_get_picture_type_char(encoder.frame->pict_type);
                    std::cout<<"\t "<<encoder.frame->pts;
                    std::cout<<"\t "<<encoder.frame->pkt_dts<<"\t    "<<encoder.frame->key_frame<<"\t\t\t ";
                    std::cout<<encoder.frame->coded_picture_number<<"\t\t\t   "<<encoder.frame->display_picture_number<<"\t\t "<<encoder.frame->width<<"\t "<<encoder.frame->height<<"\t  "<<encoder.frame->format<<"\t"<<encoder.frame->best_effort_timestamp;
                    write_encoded_frame(encoder, outputFormatContext,inputFormatContext,encoder.video_codec_context);
                }
            }
            else
            {
                debug("avcodec_send_packet",ret);
            }
        }/*else if(decoder.packet->stream_index == decoder.audio_stream_index){
            ret = av_interleaved_write_frame(outputFormatContext, decoder.packet);
            if(ret < 0){
                debug("av_interleaved_write_frame",ret,true);
            }
        }*/
    }
    av_write_trailer(outputFormatContext);
}
void write_encoded_frame(codec_information &encoder, AVFormatContext *ofmt_ctx,AVFormatContext *ifmt_ctx,AVCodecContext *target)
{
    int ret;
    av_packet_unref(encoder.packet);
    encoder.frame->pts = encoder.frame->best_effort_timestamp;
    ret = avcodec_send_frame(target,encoder.frame);
    if(ret >= 0)
    {
        while(true)
        {
            ret = avcodec_receive_packet(target,encoder.packet);
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                break;
            else if (ret < 0)
            {
                debug("avcodec_receive_packet",ret,true);
            }
            //encoder.packet.pts
            ret = av_interleaved_write_frame(ofmt_ctx, encoder.packet);
            if(ret < 0)
            {
                debug("av_interleaved_write_frame",ret,true);
            }
        }
    }
}
void set_video_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder)
{
    encoder->height = decoder->height;
    encoder->width = decoder->width;
    encoder->sample_aspect_ratio = decoder->sample_aspect_ratio;
    //encoder->flags = decoder->flags;
    encoder->time_base = av_inv_q(decoder->framerate);
    //int m_bitRate = decoder->bit_rate;
    //encoder->bit_rate = m_bitRate;
    //int delta = 0;
    //encoder->rc_min_rate = m_bitRate;
    //encoder->rc_max_rate = m_bitRate + delta;
    //encoder->rc_buffer_size = 2000000;
    //encoder->bit_rate_tolerance = delta;
}
void set_audio_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder)
{
    std::cout<<"\nAideo Context: \n"<<"\tbit_rate"<<"\tsmaple_fmt"<<"\tsample_rate"<<"\tchannels"<<"\tprofile"<<"\tchannel_layout"<<"\n";
    std::cout<<"\t"<<decoder->bit_rate<<"\t\t    "<<decoder->sample_fmt<<"\t\t  "<<decoder->sample_rate<<"\t\t  "<<decoder->channels<<"\t\t  "<<decoder->profile<<"\t  "<<decoder->channel_layout;
    encoder->bit_rate = decoder->bit_rate;
    encoder->sample_fmt = decoder->sample_fmt;
    encoder->sample_rate = decoder->sample_rate;
    encoder->channels = decoder->channels;
    encoder->profile = decoder->profile;
    encoder->channel_layout = decoder->channel_layout;
    encoder->time_base = (AVRational)(decoder->time_base);
}
void set_encoder(codec_information decoder,codec_information &encoder, AVFormatContext *ofmt_ctx)
{
    std::cout<<"\n\nSetting encoders ..."<<decoder.video_codec_context->b_frame_strategy;
    int ret;
    //video encoder
    encoder.video_codec = avcodec_find_encoder(decoder.video_codec->id);
    encoder.video_codec_context = avcodec_alloc_context3(encoder.video_codec );
    if(encoder.video_codec == nullptr)
    {
        std::cout<<"\n video encoder not found";
        exit(200);
    }
    std::cout<<"\nVideo Encoder: "<<encoder.video_codec->name;
    /*set_video_encoder_context(decoder.video_codec_context,encoder.video_codec_context);
    if (encoder.video_codec->pix_fmts)
        encoder.video_codec_context->pix_fmt = encoder.video_codec->pix_fmts[0];
    else
        encoder.video_codec_context->pix_fmt = decoder.video_codec_context->pix_fmt;*/
    avcodec_copy_context(encoder.video_codec_context,decoder.video_codec_context);
    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        encoder.video_codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ret = avcodec_open2(encoder.video_codec_context, encoder.video_codec, NULL);
    if(ret < 0)
    {
        debug("video encoder",ret);
    }
    std::cout<<"\nVideo Context: \n"<<"\theight"<<"\twidth"<<"\tpix_fmt"<<"\tbit_rate"<<"\trc_buffer_size"<<"\trc_max_rate"<<"\trc_min_rate"<<"\tflags"<<"\n";
    std::cout<<"\t"<< encoder.video_codec_context->height<<"\t"<<encoder.video_codec_context->width <<"\t   "<< encoder.video_codec_context->pix_fmt<<"\t "<<encoder.video_codec_context->bit_rate;
    std::cout<<"\t\t\t"<< encoder.video_codec_context->rc_buffer_size<<"\t  "<<encoder.video_codec_context->rc_max_rate <<"\t   "<<encoder.video_codec_context->rc_min_rate<<"\t\t  "<< encoder.video_codec_context->flags;
    std::cout<<"\nTimeBase: "<<encoder.video_codec_context->time_base.den<<"(FPS)"<< "\t"<<encoder.video_codec_context->time_base.num;
    //audio encoder
    encoder.audio_codec = avcodec_find_encoder(decoder.audio_codec->id);
    encoder.audio_codec_context = avcodec_alloc_context3(encoder.audio_codec);
    if(encoder.audio_codec == nullptr)
    {
        std::cout<<"\nAudio Encoder not found";
        exit(200);
    }
    std::cout<<"\nAudio Encoder: "<<encoder.audio_codec->name;
    set_audio_encoder_context(decoder.audio_codec_context,encoder.audio_codec_context);
    std::cout<<"\nTimeBase: "<<encoder.audio_codec_context->time_base.den<<"(FPS)"<< "\t"<<encoder.audio_codec_context->time_base.num;
    ret = avcodec_open2(encoder.audio_codec_context, encoder.audio_codec, NULL);
    if(ret < 0)
    {
        debug("Audio Encoder",ret);
    }


}
void set_decoder(codec_information &decoder, AVFormatContext *inputFormatContext)
{
    int ret;
    std::cout<<"\n--------------------------DECODERS--------------------------";
    std::cout<<"\nTYPE "<<"\tNAME "<<"\t\tID"<<"\twidth"<<"\theight"<<"\tbit_rate"<<"\tframe_rate(fps,...)";
    AVCodecParameters *codecPar = nullptr;
    for(int i = 0; i < inputFormatContext->nb_streams; i++)
    {
        codecPar = inputFormatContext->streams[i]->codecpar;
        if(codecPar->format == AVMEDIA_TYPE_VIDEO)
        {
            decoder.video_codec = avcodec_find_decoder(codecPar->codec_id);
            decoder.video_codec_context = avcodec_alloc_context3(decoder.video_codec);
            ret = avcodec_parameters_to_context(decoder.video_codec_context,inputFormatContext->streams[i]->codecpar);
            if(ret < 0)
                debug("avcodec_parameters_to_context",ret);
            decoder.video_codec_parameters = codecPar;
            decoder.video_stream_index = i;
            decoder.video_codec_context->framerate = av_guess_frame_rate(inputFormatContext, inputFormatContext->streams[i], NULL);
            std::cout<<"\nVIDEO"<<"\t"<<decoder.video_codec->long_name<<"\t"<<decoder.video_codec->id;
            std::cout<<"\t "<<decoder.video_codec_parameters->width <<"\t "<< decoder.video_codec_parameters->height;
            std::cout<<"\t "<<decoder.video_codec_parameters->bit_rate<<"\t\t   "<<decoder.video_codec_context->framerate.num<<","<<decoder.video_codec_context->framerate.den;
            std::cout<<"\n"<<decoder.video_codec_context->bits_per_coded_sample<< " , "<<decoder.video_codec_context->bits_per_raw_sample;
        }
        else
        {
            decoder.audio_codec = avcodec_find_decoder(codecPar->codec_id);
            decoder.audio_codec_context = inputFormatContext->streams[i]->codec;
            decoder.audio_codec_parameters = codecPar;
            decoder.audio_stream_index = i;
            std::cout<<"\nAUDIO"<<"\t"<<decoder.audio_codec->name<<"\t\t"<<decoder.audio_codec->id;
            std::cout<<"\t "<<decoder.audio_codec_parameters->width <<"\t "<< decoder.audio_codec_parameters->height;
            std::cout<<"\t "<<decoder.audio_codec_parameters->bit_rate<<"\t\t   "<<decoder.audio_codec_context->framerate.num<<","<<decoder.audio_codec_context->framerate.den;
        }
    }
    ret = avcodec_open2(decoder.video_codec_context, decoder.video_codec, NULL);
    if(ret < 0)
    {
        debug("video codec",ret);
    }
    ret = avcodec_open2(decoder.audio_codec_context, decoder.audio_codec, NULL);
    if(ret < 0)
    {
        debug("audio codec",ret);
    }
    std::cout<<"\n";
    av_dump_format(inputFormatContext, 0, "test.mp4", 0);
    std::cout<<"\nDecoders are Set";

}
AVFormatContext * openMedia(char *inputFilePath)
{
    AVFormatContext *inputFormatContext = nullptr;
    std::cout<<"OPENNING MEDIA FILE: "<<inputFilePath<<"\n";
    int ret;
    ret = avformat_open_input(&inputFormatContext, inputFilePath, nullptr, nullptr);
    if ( ret != 0)
    {
        debug("avformat_open_input",ret);
    }
    std::cout<<"Format "<<inputFormatContext->iformat->long_name<<"\nDuration: "<<inputFormatContext->duration;
    ret = avformat_find_stream_info(inputFormatContext, nullptr);
    if (ret < 0)
    {
        debug("avformat_find_stream_info",ret);
    }
    std::cout<<"\nBitRate: "<<inputFormatContext->bit_rate;
    std::cout<<"\nMEDIA OPEN FINISHED";
    return inputFormatContext;
}
void debug(std::string message, int ret,bool doExit)
{
    char error[500];
    av_strerror(ret, error, 500);
    std::cout<<"\nhas a " + message + " error: "+error;
    if(doExit)
        exit(50);
}

