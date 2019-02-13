#include <algorithm>
extern "C"{
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
}

#define av_err2str(errnum) av_make_error_string((char*)__builtin_alloca(AV_ERROR_MAX_STRING_SIZE), AV_ERROR_MAX_STRING_SIZE, errnum)
#define av_ts2str(ts) av_ts_make_string((char*)__builtin_alloca(AV_TS_MAX_STRING_SIZE), ts)

int received_frames = 0;
int written_packets_count = 0;
int largest_packet_size = 0;
int largest_input_packet_size = 0;

int should_skip_frame = 0;
int skipped_frames = 0;
int first_packet_size = 0;
int first_packet_size_read = 0;

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;

typedef struct StreamContext {
    AVCodecContext *dec_ctx;
    AVCodecContext *enc_ctx;
} StreamContext;

static StreamContext *stream_ctx;

static int open_input_file(const char *filename)
{
    int ret;
    unsigned int i;

    ifmt_ctx = NULL;
    if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    stream_ctx = static_cast<StreamContext *>(av_mallocz_array(ifmt_ctx->nb_streams, sizeof(*stream_ctx)));
    if (!stream_ctx)
        return AVERROR(ENOMEM);

    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_ctx;
        if (!dec) {
            av_log(NULL, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        codec_ctx = avcodec_alloc_context3(dec);
        if (!codec_ctx) {
            av_log(NULL, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }

        ret = avcodec_parameters_to_context(codec_ctx, stream->codecpar);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                    "for stream #%u\n", i);
            return ret;
        }
        /* Reencode video & audio and remux subtitles etc. */
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
                codec_ctx->framerate = av_guess_frame_rate(ifmt_ctx, stream, NULL);

                printf("time base %d /%d \n",codec_ctx->time_base.num, codec_ctx->time_base.den);

            /* Open decoder */
            ret = avcodec_open2(codec_ctx,  dec, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
                return ret;
            }
        }
        stream_ctx[i].dec_ctx = codec_ctx;
    }

    av_dump_format(ifmt_ctx, 0, filename, 0);
    return 0;
}


static int open_output_file(const char *filename)
{
    AVStream *out_stream;
    AVStream *in_stream;
    AVCodecContext *dec_ctx, *enc_ctx;
    AVCodec *encoder;
    int ret;
    unsigned int i;

    ofmt_ctx = NULL;
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, filename);
    if (!ofmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        return AVERROR_UNKNOWN;
    }


    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream) {
            av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
            return AVERROR_UNKNOWN;
        }

        in_stream = ifmt_ctx->streams[i];
        dec_ctx = stream_ctx[i].dec_ctx;

        if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
            || dec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            /* in this example, we choose transcoding to same codec */
            encoder = avcodec_find_encoder(dec_ctx->codec_id);
            if (!encoder) {
                av_log(NULL, AV_LOG_FATAL, "Necessary encoder not found\n");
                return AVERROR_INVALIDDATA;
            }
            enc_ctx = avcodec_alloc_context3(encoder);
            if (!enc_ctx) {
                av_log(NULL, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
                return AVERROR(ENOMEM);
            }

            /* In this example, we transcode to same properties (picture size,
             * sample rate etc.). These properties can be changed for output
             * streams easily using filters */
            if (dec_ctx->codec_type == AVMEDIA_TYPE_VIDEO) {
                enc_ctx->height = dec_ctx->height;
                enc_ctx->width = dec_ctx->width;
                enc_ctx->sample_aspect_ratio = dec_ctx->sample_aspect_ratio;
                /* take first format from list of supported formats */
                if (encoder->pix_fmts)
                    enc_ctx->pix_fmt = encoder->pix_fmts[0];
                else
                    enc_ctx->pix_fmt = dec_ctx->pix_fmt;

                /* Video time_base can be set to whatever is handy and supported by encoder
                 * Remember fra
                 *
                 * */

                enc_ctx->time_base = in_stream->time_base;

                av_opt_set( enc_ctx->priv_data, "preset", "veryslow", 0 );

            } else {
                enc_ctx->sample_rate = dec_ctx->sample_rate;
                enc_ctx->channel_layout = dec_ctx->channel_layout;
                enc_ctx->channels = av_get_channel_layout_nb_channels(enc_ctx->channel_layout);
                /* take first format from list of supported formats */
                enc_ctx->sample_fmt = encoder->sample_fmts[0];
                enc_ctx->time_base = (AVRational){1, enc_ctx->sample_rate};
            }

            if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


            /* Third parameter can be used to pass settings to encoder */
            ret = avcodec_open2(enc_ctx, encoder, NULL);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Cannot open video encoder for stream #%u\n", i);
                return ret;
            }
            ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Failed to copy encoder parameters to output stream #%u\n", i);
                return ret;
            }

            out_stream->time_base = enc_ctx->time_base;
            stream_ctx[i].enc_ctx = enc_ctx;
        } else if (dec_ctx->codec_type == AVMEDIA_TYPE_UNKNOWN) {
            av_log(NULL, AV_LOG_FATAL, "Elementary stream #%d is of unknown type, cannot proceed\n", i);
            return AVERROR_INVALIDDATA;
        } else {
            /* if this stream must be remuxed */
            ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Copying parameters for stream #%u failed\n", i);
                return ret;
            }
            out_stream->time_base = dec_ctx->time_base;
        }

    }
    av_dump_format(ofmt_ctx, 0, filename, 1);

    if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
            return ret;
        }
    }


    /* init muxer, write output file header */
    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
        return ret;
    }

    return 0;
}



static int encode_write_frame(AVFrame *filt_frame, int stream_index) {
    int ret;
    int got_frame_local;
    AVPacket enc_pkt;


    /* encode filtered frame */
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    ret = avcodec_send_frame(stream_ctx[stream_index].enc_ctx, filt_frame);

    //av_frame_free(&filt_frame);
    if (ret < 0)
        return ret;

    while((ret = avcodec_receive_packet(stream_ctx[stream_index].enc_ctx, &enc_pkt) >= 0)){
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            printf("something strange:%s \n", av_err2str(ret));
            return ret;
        }

        enc_pkt.stream_index = stream_index;
        printf("encoding:before adjust ts:%s\n", av_ts2str(enc_pkt.pts));

        av_packet_rescale_ts(&enc_pkt,
                             stream_ctx[stream_index].dec_ctx->time_base,
                             ofmt_ctx->streams[stream_index]->time_base);
        printf("encoding:after adjust ts:%s\n", av_ts2str(enc_pkt.pts));

        av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
        printf("Writing packet with size:%d  number:%d \n", enc_pkt.size, ++written_packets_count);
        largest_packet_size = std::max(enc_pkt.size, largest_packet_size);
        if(first_packet_size == 0){
            first_packet_size = enc_pkt.size;
        }
        ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
        av_packet_unref(&enc_pkt);
        return ret;


    }
}


int main(int argc, char **argv)
{
    // Initialize libav.
    av_register_all();
    int ret;
    AVPacket* packet = av_packet_alloc();
    AVFrame* frame = nullptr;
    int stream_index;
    enum AVMediaType type;

    ret = open_input_file("/media/syrix/programms/projects/GP/test_frames/4_out.mp4");
    if(ret < 0){
        printf("couldn't open input file\n");
        exit(1);
    }
    ret = open_output_file("/media/syrix/programms/projects/GP/test_frames/4_out_even.mp4");
    if(ret < 0){
        printf("couldn't open output file\n");
        exit(1);
    }
    // Read all packets.
    while(true){
        frame = av_frame_alloc();
        if ((ret = av_read_frame(ifmt_ctx, packet)) < 0)
            break;
//        packet->duration = 0;
        if(first_packet_size_read == 0)
        first_packet_size_read = packet->size;
        stream_index = packet->stream_index;
        type = ifmt_ctx->streams[packet->stream_index]->codecpar->codec_type;
        av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
               stream_index);

        av_packet_rescale_ts(packet,
                             ifmt_ctx->streams[stream_index]->time_base,
                             stream_ctx[stream_index].dec_ctx->time_base);

        if(type == AVMEDIA_TYPE_VIDEO){
            largest_input_packet_size = std::max(largest_input_packet_size, packet->size);
            ret = avcodec_send_packet(stream_ctx[stream_index].dec_ctx, packet);
            if (ret < 0) {
                av_frame_free(&frame);
                av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
                break;
            }else{
                // Decode.
                while (ret >= 0) {
                    ret = avcodec_receive_frame(stream_ctx[stream_index].dec_ctx, frame);
                    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                        break;
                    else if (ret < 0) {
                        fprintf(stderr, "Error during decoding\n");
                        exit(1);
                    }

                    printf("received_frames:%d ",received_frames);
                    if(should_skip_frame++%2 == 1){
                        printf("Skipped_frame_number:%d \n", ++skipped_frames);
                        av_frame_free(&frame);
                        break;
                    }
                    received_frames++;
                    printf("#frames:%d \n", received_frames-1);
                    ret = encode_write_frame(frame, stream_index);
                    av_frame_free(&frame);
                    if (ret < 0){
                        printf("badddd\n");
                        exit(1);
                    }
                }

            }

        }
    }

    av_write_trailer(ofmt_ctx);
    av_frame_free(&frame);
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_free_context(&stream_ctx[i].dec_ctx);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] && stream_ctx[i].enc_ctx)
            avcodec_free_context(&stream_ctx[i].enc_ctx);
    }

    av_free(stream_ctx);
    avformat_close_input(&ifmt_ctx);
    if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

        if (ret < 0)
            av_log(NULL, AV_LOG_ERROR, "Error occurred: %s\n", av_err2str(ret));

    printf("Largest written_packet_size:%d largest read_packet_size:%d ,"
                   "total_written_frames_count:%d total_skipped_frames:%d \n", largest_packet_size, largest_input_packet_size,
           received_frames, skipped_frames);

    printf(" First written_packet_size:%d  first_read_packet_size:%d\n", first_packet_size_read, first_packet_size);
    return ret ? 1 : 0;

}
