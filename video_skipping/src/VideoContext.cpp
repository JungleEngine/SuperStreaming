//
// Created by syrix on 6/27/19.
//

#include "VideoContext.h"

VideoContext::VideoContext(std::string &input_filename, std::string &output_filename) :
        packet_queue_{std::make_unique<PacketQueue>(10)},
        frame_queue_{std::make_unique<FrameQueue>(10)},
        processed_frame_queue_{std::make_unique<FrameQueue>(10)} {

    // Required for ffmpeg init.
    av_register_all();

    this->input_filename = input_filename;
    this->output_filename = output_filename;

    // Open input and output files.
    this->openInputFile();
    this->openOutputFile();

    this->getDecoderCntx();
    this->getEncoderCntx();

    // Header should be written first.
    this->writeHeader();

}

int VideoContext::openInputFile() {
    this->ifmt_ctx = nullptr;
    if ((avformat_open_input(&this->ifmt_ctx, this->input_filename.c_str(), nullptr, nullptr)) < 0) {

        fprintf(stderr, "Could not open input file '%s'", this->input_filename);
        return -1;
    }

    if ((avformat_find_stream_info(ifmt_ctx, nullptr)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        return -1;
    }

    av_dump_format(this->ifmt_ctx, 0, this->input_filename.c_str(), 0);
}

int VideoContext::openOutputFile() {

    int ret;
    avformat_alloc_output_context2(&this->ofmt_ctx, nullptr, nullptr, this->output_filename.c_str());
    if (!this->ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        return ret;
    }

    this->ofmt = this->ofmt_ctx->oformat;

    for (int i = 0; i < this->ifmt_ctx->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = this->ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;



        out_stream = avformat_new_stream(this->ofmt_ctx, nullptr);
        if (!out_stream) {
            fprintf(stderr, "Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            return -1;
        }

        if (in_codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            this->video_stream_indx = i;
        } else if (in_codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            this->audio_stream_indx = i;
            ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
            if (ret < 0) {
                fprintf(stderr, "Failed to copy codec parameters\n");
                exit(1);
            }
//            out_stream->codecpar->codec_tag = 0;
        } else {
            printf("Unkown stream, not video and not audio, exit.\n");
            return -1;
        }



    }

    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&this->ofmt_ctx->pb, this->output_filename.c_str(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", this->output_filename);
            return -1;
        }
    }


}


void VideoContext::close_input_file() {
    avformat_close_input(&ifmt_ctx);

}


void VideoContext::close_output_file() {
    if (this->ofmt_ctx && !(this->ofmt->flags & AVFMT_NOFILE))
        avio_closep(&this->ofmt_ctx->pb);
    avformat_free_context(this->ofmt_ctx);

}


int VideoContext::getDecoderCntx() {
    int ret = 0;
    for (int i = 0; i < ifmt_ctx->nb_streams; i++) {
        AVStream *stream = ifmt_ctx->streams[i];
        AVCodec *dec = avcodec_find_decoder(stream->codecpar->codec_id);
        AVCodecContext *codec_cntx;
        if (!dec) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to find decoder for stream #%u\n", i);
            return AVERROR_DECODER_NOT_FOUND;
        }
        codec_cntx = avcodec_alloc_context3(dec);
        if (!codec_cntx) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to allocate the decoder context for stream #%u\n", i);
            return AVERROR(ENOMEM);
        }
        ret = avcodec_parameters_to_context(codec_cntx, stream->codecpar);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to copy decoder parameters to input decoder context "
                    "for stream #%u\n", i);
            return ret;
        }


        /* Reencode video & audio and remux subtitles etc. */
        if (codec_cntx->codec_type == AVMEDIA_TYPE_VIDEO) {
            codec_cntx->framerate = av_guess_frame_rate(ifmt_ctx, stream, nullptr);
            this->video_dec_cntx = codec_cntx;


        } else if (codec_cntx->codec_type == AVMEDIA_TYPE_AUDIO) {
            this->audio_dec_cntx = codec_cntx;

        } else if (codec_cntx->codec_type == AVMEDIA_TYPE_SUBTITLE) {
            printf("Subtitle Stream found and not supported\n");
            return -1;
        }

        ret = avcodec_open2(codec_cntx, dec, NULL);
        if (ret < 0) {
            av_log(nullptr, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n", i);
            return ret;
        }

    }
    return 0;

}


int VideoContext::getEncoderCntx() {
    AVCodec *video_encoder;
    AVCodec *audio_encoder;

    // Get video encoder context.
    video_encoder = avcodec_find_encoder(this->video_dec_cntx->codec_id);
    if (!video_encoder) {
        av_log(nullptr, AV_LOG_FATAL, "Necessary encoder not found\n");
        return AVERROR_INVALIDDATA;
    }

    this->video_enc_cntx = avcodec_alloc_context3(video_encoder);
    if (!this->video_enc_cntx) {
        av_log(nullptr, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
    }

    this->video_enc_cntx->height = this->video_dec_cntx->height;
    this->video_enc_cntx->width = this->video_dec_cntx->width;
    this->video_enc_cntx->sample_aspect_ratio = this->video_dec_cntx->sample_aspect_ratio;
    /* take first format from list of supported formats */
    if (video_encoder->pix_fmts)
        this->video_enc_cntx->pix_fmt = video_encoder->pix_fmts[0];
    else
        this->video_enc_cntx->pix_fmt = this->video_dec_cntx->pix_fmt;

    /* video time_base can be set to whatever is handy and supported by encoder */
    this->video_enc_cntx->time_base = this->ifmt_ctx->streams[this->video_stream_indx]->time_base;
    printf("time_base:%d/%d\n", this->video_enc_cntx->time_base.num, this->video_enc_cntx->time_base.den);

    av_opt_set(this->video_enc_cntx->priv_data, "profile", "baseline", 0);
    av_opt_set(this->video_enc_cntx->priv_data, "crf", "18", 0);

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        this->video_enc_cntx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }


    int ret = avcodec_open2(this->video_enc_cntx, video_encoder, nullptr);
    if (ret < 0) {
        return ret;
    }

    ret = avcodec_parameters_from_context(this->ofmt_ctx->streams[this->video_stream_indx]->codecpar,
                                          this->video_enc_cntx);
    if (ret < 0) {
        return ret;
    }

    this->ofmt_ctx->streams[this->video_stream_indx]->time_base = this->video_enc_cntx->time_base;



    // Get audio encoder context.
    audio_encoder = avcodec_find_encoder(this->audio_dec_cntx->codec_id);
    if (!audio_encoder) {
        av_log(nullptr, AV_LOG_FATAL, "Necessary encoder not found\n");
        return AVERROR_INVALIDDATA;
    }

    this->audio_enc_cntx = avcodec_alloc_context3(audio_encoder);
    if (!this->audio_enc_cntx) {
        av_log(nullptr, AV_LOG_FATAL, "Failed to allocate the encoder context\n");
        return AVERROR(ENOMEM);
    }
    this->audio_enc_cntx->sample_rate = this->audio_dec_cntx->sample_rate;
    this->audio_enc_cntx->channel_layout = this->audio_dec_cntx->channel_layout;
    this->audio_enc_cntx->channels = av_get_channel_layout_nb_channels(this->audio_enc_cntx->channel_layout);
    /* take first format from list of supported formats */
    this->audio_enc_cntx->sample_fmt = audio_encoder->sample_fmts[0];
    this->audio_enc_cntx->time_base = (AVRational) {1, this->audio_enc_cntx->sample_rate};

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        this->audio_enc_cntx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ret = avcodec_open2(this->audio_enc_cntx, audio_encoder, nullptr);
    if (ret < 0) {
        return ret;
    }

    ret = avcodec_parameters_from_context(this->ofmt_ctx->streams[this->audio_stream_indx]->codecpar,
                                          this->audio_enc_cntx);
    if (ret < 0) {
        return ret;
    }

    this->ofmt_ctx->streams[this->audio_stream_indx]->time_base = this->audio_enc_cntx->time_base;

    return 0;
}


int VideoContext::writeHeader() {
    int ret;
    // Now writing header of file after settings ofmt_ctx parameters.
    ret = avformat_write_header(this->ofmt_ctx, nullptr);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        return -1;
    }
    av_dump_format(this->ofmt_ctx, 0, this->output_filename.c_str(), 1);

    return 0;
}


bool VideoContext::sendPacketToDecoder(AVPacket *packet) {
    auto ret = avcodec_send_packet(this->video_dec_cntx, packet);
    return !(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

}


bool VideoContext::receiveFrameFromDecoder(AVFrame *frame) {
    auto ret = avcodec_receive_frame(this->video_dec_cntx, frame);
    return !(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

}


bool VideoContext::sendFrameToEncoder(AVFrame *frame) {
    auto ret = avcodec_send_frame(this->video_enc_cntx, frame);
    return !(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

}


bool VideoContext::receivePacketFromEncoder(AVPacket *packet) {
    auto ret = avcodec_receive_packet(this->video_enc_cntx, packet);
    return !(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

}

/**
 * This function is responsible for initiating threads.
 */
void VideoContext::runSkipping() {
    this->stages_.emplace_back(&VideoContext::demultiplex, this);
    this->stages_.emplace_back(&VideoContext::decode, this);
    this->stages_.emplace_back(&VideoContext::processFrames, this);
    this->stages_.emplace_back(&VideoContext::encode, this);


    for (auto &stage : stages_) {
        stage.join();
    }
    if (exception_) {
        std::rethrow_exception(exception_);
    }

}

void VideoContext::demultiplex() {
    try {
        for (;;) {
            // Create AVPacket
            std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> packet{
                    new AVPacket,
                    [](AVPacket *p) {
                        av_packet_unref(p);
                        delete p;
                    }};

            av_init_packet(packet.get());
            packet->data = nullptr;

            // Read frame into AVPacket
            if ((av_read_frame(this->ifmt_ctx, packet.get())) < 0) {
                packet_queue_->finished();
                break;
            }

            if (!packet_queue_->push(move(packet))) {
                break;
            }

        }
    } catch (...) {
        exception_ = std::current_exception();
        frame_queue_->quit();
        packet_queue_->quit();
    }
}


void VideoContext::decode() {
    try {
        for (;;) {
            // Create AVFrame and packet.
            std::unique_ptr<AVFrame, std::function<void(AVFrame *)>>
                    frame_decoded{
                    av_frame_alloc(), [](AVFrame *f) { av_frame_free(&f); }};

            std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> packet{
                    nullptr, [](AVPacket *p) {
                        av_packet_unref(p);
                        delete p;
                    }};

            // Read packet from queue
            if (!packet_queue_->pop(packet)) {
                frame_queue_->finished();
                break;
            }

            printf("to be decoded audio packet of type:%i|pts:%li\n", packet->stream_index, packet->pts);

            // Audio packets should be written directly into output file.
//            if (packet->stream_index == this->audio_stream_indx) {
//                if (av_interleaved_write_frame(this->ofmt_ctx, packet.get()) < 0) {
//                    printf("error while writing audio packet into output file\n");
//                }
//                continue;
//            }

            // Video packets should be decoded first.
            // This check maybe redundant for now.
            if (!packet->stream_index == this->video_stream_indx)
                continue;

            // If the packet didn't send, receive more frames and try again
            bool sent = false;
            while (!sent) {
                sent = this->sendPacketToDecoder(packet.get());
                while (this->receiveFrameFromDecoder(frame_decoded.get())) {
                    printf("received video frame from decoder with pts:%li\n", frame_decoded->pts);
                    if (!frame_queue_->push(move(frame_decoded))) {
                        break;
                    }
                }
            }
        }
    } catch (...) {
        this->exception_ = std::current_exception();
        this->frame_queue_->quit();
        this->packet_queue_->quit();
    }
}

/*
 * This function reads frames from framequeue and process it and
 * finally it puts frames into processed frames queue to be written.
 */
void VideoContext::processFrames() {
    try {
        for (;;) {
            std::unique_ptr<AVFrame, std::function<void(AVFrame *)>> frame{
                    nullptr, [](AVFrame *f) { av_frame_free(&f); }};

            // Do logic here, pop from frame_queue and push to processed queue.
            if (!this->frame_queue_->pop(frame)) {
                this->processed_frame_queue_->finished();
                break;
            }

            if (!this->processed_frame_queue_->push(move(frame))) {
                break;
            }

        }
    } catch (...) {
        this->frame_queue_->quit();
        this->packet_queue_->quit();
        this->processed_frame_queue_->quit();
    }

}

void VideoContext::encode() {
    try {
        for (;;) {
            // Create AVFrame.
            std::unique_ptr<AVFrame, std::function<void(AVFrame *)>> frame{
                    nullptr, [](AVFrame *f) { av_frame_free(&f); }};


            // Pop from processed queue and write in file.
            if (!this->processed_frame_queue_->pop(frame)) {
                break;
            }


            if (!this->sendFrameToEncoder(frame.get())) {
                break;
            }
            for (;;) {
                // Create AVPacket.
                std::unique_ptr<AVPacket, std::function<void(AVPacket *)>> packet{
                        new AVPacket,
                        [](AVPacket *p) {
                            av_packet_unref(p);
                            delete p;
                        }};

                // Init packet.
                av_init_packet(packet.get());
                packet->data = nullptr;
                packet->size = 0;
                packet->stream_index = this->video_stream_indx;

                if (!this->receivePacketFromEncoder(packet.get()))
                    break;
//                packet->stream_index = this->video_stream_indx;
                if (av_interleaved_write_frame(this->ofmt_ctx, packet.get()) < 0) {
                    printf("error while writing video packet into output file\n");
                    break;
                }

            }


        }
    } catch (...) {
        this->frame_queue_->quit();
        this->packet_queue_->quit();
        this->processed_frame_queue_->quit();
    }
}


int VideoContext::writePacket() {
    return 0;
}


