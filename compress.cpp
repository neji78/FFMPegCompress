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
} ;
void debug(std::string message,int result);
void set_video_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder);
void set_audio_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder);
AVFormatContext * openMedia(char *inputFilePath);
void set_decoder(codec_information &decoder, AVFormatContext *inputFormatContext);
void set_encoder(codec_information decoder,codec_information &encoder);
int main()
{
    std::string inputFilePath = "sample.mkv";
    std::string outputFilePath = "output1.mp4";
    AVFormatContext* inputFormatContext = nullptr;
    int result,result_ = -1;

    inputFormatContext = openMedia((char *)inputFilePath.c_str());
    codec_information decoder;
    codec_information encoder;
    set_decoder(decoder,inputFormatContext);
    decoder.video_codec_context->framerate = av_guess_frame_rate(inputFormatContext, inputFormatContext->streams[decoder.video_stream_index], NULL);
    AVFormatContext* outputFormatContext = nullptr;
    result = avformat_alloc_output_context2(&outputFormatContext, nullptr, nullptr, outputFilePath.c_str());
    if ( result< 0)
    {
        // error handling
        debug("avformat_alloc_output_context2",result);
    }

    set_encoder(decoder,encoder);

    AVStream* stream = avformat_new_stream(outputFormatContext, encoder.video_codec);
    if (stream == nullptr)
    {
        // error handling
        debug("avformat_new_stream",-1);
    }

    result = avcodec_parameters_from_context(stream->codecpar, encoder.video_codec_context);
    if(result < 0)
    {
        debug("avcodec_parameters_from_context",result);
    }
    stream->time_base = encoder.video_codec_context->time_base;
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

    std::cout<<"\n\t\t\t~~~~~~~~~~~~~~~~~~~Transcodeing~~~~~~~~~~~~~~~~~~~";
    std::cout<<"\nFrame"<<"\tPTS"<<"\tDTS"<<"\tKey_Frame"<<"\tCoded_Picture_Number"<<"\tDisplay_Picture_Number"<<"\tWidth"<<"\tHeight"<<"\tFormat";
    while (av_read_frame(inputFormatContext, &packet) >= 0)
    {
        if (packet.stream_index == decoder.video_stream_index)
        {
            AVFrame* decodedFrame = av_frame_alloc();
            if (decodedFrame == nullptr)
            {
                // error handling
                debug("decodedFrame == nullpt",-1);
            }

            int frameFinished = 0;
            result = avcodec_send_packet(decoder.video_codec_context, &packet);
            result_ = avcodec_receive_frame(decoder.video_codec_context, decodedFrame);
            if ( result== 0 &&  result_== 0)
            {
                std::cout<<"\n  "<<av_get_picture_type_char(decodedFrame->pict_type);
                std::cout<<"\t "<<decodedFrame->pts;
                std::cout<<"\t "<<decodedFrame->pkt_dts<<"\t    "<<decodedFrame->key_frame<<"\t\t\t ";
                std::cout<<decodedFrame->coded_picture_number<<"\t\t\t   "<<decodedFrame->display_picture_number<<"\t\t "<<decodedFrame->width<<"\t "<<decodedFrame->height<<"\t  "<<decodedFrame->format;
                //save_gray_frame(decodedFrame->data[0], decodedFrame->linesize[0], decodedFrame->width, decodedFrame->height, (char *)frame_filename.c_str());
                //av_frame_unref(frame);
                //av_frame_move_ref(frame, decodedFrame);
                //av_packet_unref(&packet);
                //frame.
                /*
                std::cout<<"\nencoder frame:";
                std::cout<<"\nFrame "<<av_get_picture_type_char(frame->pict_type);
                std::cout<<" "<<decoder.video_codec_context->frame_number<<" pts "<<frame->pts;
                std::cout<<" dts "<<frame->pkt_dts<<" key_frame "<<frame->key_frame<<" [coded_picture_number ";
                std::cout<<frame->coded_picture_number<<" , display_picture_number "<<frame->display_picture_number<<"]"<<frame->width<<"x"<<frame->height<<" "<<frame->format;
*/
                AVPacket encodedPacket;
                av_init_packet(&encodedPacket);
                encodedPacket.data = nullptr;
                encodedPacket.size = 0;

                result = avcodec_send_frame(encoder.video_codec_context, decodedFrame);
                result_ = avcodec_receive_packet(encoder.video_codec_context, &encodedPacket);
                if ( result== 0 &&  result_== 0)
                {
                    encodedPacket.stream_index = stream->index;
                    /*av_packet_rescale_ts(&packet,
                                 inputFormatContext->streams[stream->index]->time_base,
                                 outputFormatContext->streams[stream->index]->time_base);*/

                    result = av_interleaved_write_frame(outputFormatContext, &encodedPacket);

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
    }
    av_write_trailer(outputFormatContext);
}
void set_video_encoder_context(AVCodecContext *decoder,AVCodecContext *encoder)
{
    std::cout<<"\nVideo Context: \n"<<"\theight"<<"\twidth"<<"\tpix_fmt"<<"\tbit_rate"<<"\trc_buffer_size"<<"\trc_max_rate"<<"\trc_min_rate"<<"\tflags"<<"\n";
    std::cout<<"\t"<< decoder->height<<"\t"<<decoder->width <<"\t   "<< decoder->pix_fmt<<"\t "<<decoder->bit_rate;
    std::cout<<"\t\t\t"<< decoder->rc_buffer_size<<"\t  "<<decoder->rc_max_rate <<"\t   "<<decoder->rc_min_rate<<"\t\t  "<< decoder->flags;
    encoder->height = decoder->height;
    encoder->width = decoder->width;
    encoder->sample_aspect_ratio = decoder->sample_aspect_ratio;
    encoder->bit_rate = 400000/*decoder->bit_rate*/;
    encoder->rc_min_rate = decoder->rc_min_rate;
    encoder->flags = decoder->flags;
    encoder->time_base = av_inv_q(decoder->framerate);
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
void set_encoder(codec_information decoder,codec_information &encoder)
{
    std::cout<<"\n\nSetting encoders ...";
    int result;
    //video encoder
    encoder.video_codec = avcodec_find_encoder_by_name(decoder.video_codec->name);
    encoder.video_codec_context = avcodec_alloc_context3(encoder.video_codec );
    if(encoder.video_codec == nullptr)
    {
        std::cout<<"\n video encoder not found";
        exit(200);
    }
    std::cout<<"\nVideo Encoder: "<<encoder.video_codec->name;
    set_video_encoder_context(decoder.video_codec_context,encoder.video_codec_context);
    if (encoder.video_codec->pix_fmts)
        encoder.video_codec_context->pix_fmt = encoder.video_codec->pix_fmts[0];
    else
        encoder.video_codec_context->pix_fmt = decoder.video_codec_context->pix_fmt;
    std::cout<<"\nTimeBase: "<<encoder.video_codec_context->time_base.den<<"(FPS)"<< "\t"<<encoder.video_codec_context->time_base.num;
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
        std::cout<<"\nAudio Encoder not found";
        exit(200);
    }
    std::cout<<"\nAudio Encoder: "<<encoder.audio_codec->name;
    set_audio_encoder_context(decoder.audio_codec_context,encoder.audio_codec_context);
    std::cout<<"\nTimeBase: "<<encoder.audio_codec_context->time_base.den<<"(FPS)"<< "\t"<<encoder.audio_codec_context->time_base.num;
    result = avcodec_open2(encoder.audio_codec_context, encoder.audio_codec, NULL);
    if(result < 0)
    {
        debug("Audio Encoder",result);
    }


}
void set_decoder(codec_information &decoder, AVFormatContext *inputFormatContext)
{
    int result;
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
            result = avcodec_parameters_to_context(decoder.video_codec_context ,inputFormatContext->streams[i]->codecpar);
            if(result < 0)
                debug("avcodec_parameters_to_context",result);
            decoder.video_codec_parameters = codecPar;
            decoder.video_stream_index = i;
            std::cout<<"\nVIDEO"<<"\t"<<decoder.video_codec->long_name<<"\t"<<decoder.video_codec->id;
            std::cout<<"\t "<<decoder.video_codec_parameters->width <<"\t "<< decoder.video_codec_parameters->height;
            std::cout<<"\t "<<decoder.video_codec_parameters->bit_rate<<"\t\t   "<<decoder.video_codec_context->framerate.num<<","<<decoder.video_codec_context->framerate.den;
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
    std::cout<<"\nDecoders are Set";

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

