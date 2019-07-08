//
// Created by syrix on 7/6/19.
//

#ifndef VIDEO_SKIPPING_SKIPPING_H
#define VIDEO_SKIPPING_SKIPPING_H

extern "C" {
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>
#include <libavutil/timestamp.h>
#include <libavutil/opt.h>
}

#include <iostream>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <opencv2/opencv.hpp>

#include "Model.h"
#include "Tensor.h"

class Skipping {
#define THRESHOLD_MSE 25
private:
    /*
     * Model attributes.
     */
    std::string modelPath;
    Model* model;
    Tensor* input_is_training;
    Tensor* input_input_1;
    Tensor* input_input_2;
    Tensor* input_hold_prob_conv;
    Tensor* input_hold_prob_fc;
    Tensor* output_output;





    SwsContext* swsCtx;
    int videoHeight;
    int videoWidth;
    AVPixelFormat videoPixelFormat;
    bool useHyprid;

    // Skipping info.
    int total_checked;
    int total_skipped;
    int total_skipped_model_decision;
    int total_skipped_MSE;

public:
    /*
     * Load model
     */
     Skipping(std::string modelPath, int videoHeight, int videoWidth, AVPixelFormat videoPixelFormat, bool useHyprid=true){
        this->modelPath = modelPath;
        this->videoHeight = videoHeight;
        this->videoWidth = videoWidth;
        this->videoPixelFormat = videoPixelFormat;
        this->useHyprid = useHyprid;

        // Create context to convert yuv to BGR.
        this->swsCtx = sws_getContext(this->videoWidth, this->videoHeight,
                                       this->videoPixelFormat,
                                      this->videoWidth, this->videoHeight, AV_PIX_FMT_BGR24,
                                       SWS_FAST_BILINEAR, NULL, NULL, NULL);
        this->total_checked = 0;
        this->total_skipped = 0;
        this->total_skipped_model_decision = 0;
        this->total_skipped_MSE = 0;

        // Init model.
        this->model = new Model(modelPath);
        input_input_1 = new Tensor(*model, "input_1");
        input_input_2 = new Tensor(*this->model, "input_2");
        output_output = new Tensor(*this->model, "output");


    }

    ~Skipping(){
        sws_freeContext(this->swsCtx);
    }

    double getPSNR(const cv::Mat& I1, const cv::Mat& I2)
    {
        cv::Mat s1;
        absdiff(I1, I2, s1);       // |I1 - I2|
        s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
        s1 = s1.mul(s1);           // |I1 - I2|^2
        cv::Scalar s = sum(s1);        // sum elements per channel
        double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels
        if( sse <= 1e-10) // for small values return zero
            return 0;
        else
        {
            double mse  = sse / (double)(I1.channels() * I1.total() + 1e-10);
            double psnr = 10.0 * log10((1 * 1) / mse);
            return psnr;
        }
    }

    double runModel(cv::Mat groundTruth, cv::Mat interpolated){
        cv::Mat inp1;
        cv::Mat inp2;

        cv::resize(groundTruth, inp1, cv::Size(128, 128));
        cv::resize(interpolated, inp2, cv::Size(128, 128));

         std::vector<float > img1_data;
         img1_data.assign(inp1.data, inp1.data + inp1.total() * inp1.channels());

         std::vector<float > img2_data;
         img2_data.assign(inp2.data, inp2.data + inp2.total() * inp2.channels());

         this->input_input_1->set_data(img1_data, {1, 128, 128, 3});
         this->input_input_2->set_data(img2_data, {1, 128, 128, 3});

         this->model->run({input_input_1, input_input_2}, output_output);

        double output = output_output->get_data<float>()[0];
        return output;

    }
    bool shouldSkip(AVFrame* firstFrame, AVFrame* secondFrame, AVFrame* thirdFrame){
        // First increment total_checked.
        this->total_checked++;

        // Check if should skip..
        cv::Mat firstFrameMat, secondFrameMat, thirdFrameMat;

        this->AVFrameToMat(firstFrame, firstFrameMat);
        this->AVFrameToMat(secondFrame, secondFrameMat);
        this->AVFrameToMat(thirdFrame, thirdFrameMat);
        cv::Mat groundTruth;
        secondFrameMat.convertTo(groundTruth, CV_32FC3, 1.f/255.0);

        // Get average.
        cv::Mat interpolated = cv::Mat::zeros(cv::Size(firstFrameMat.cols, firstFrameMat.rows), CV_32FC3); //larger depth to avoid saturation

        cv::accumulate(firstFrameMat, interpolated);
        cv::accumulate(thirdFrameMat, interpolated);

        // Get normalized interpolated frame.
        interpolated = interpolated / 2.0;
        interpolated /= 255.0;

        double MSEScore = this->getPSNR(interpolated, groundTruth);

        if(MSEScore > THRESHOLD_MSE){
            this->total_skipped_MSE++;
            this->total_skipped++;
            return true;

        }else{
            // Check model.
            double output = this->runModel(interpolated, groundTruth);
            if(output <= 0.5){
                this->total_skipped_model_decision++;
                return true;
            }
        }
        return false;


//        std::cout<<" MSE: "<< MSEScore <<std::endl;

//        cv::imwrite( "ground_truth" + std::to_string(PSNRScore) +".png", groundTruth);
//        cv::imwrite( "frame_mid"+ std::to_string(PSNRScore) +".png", interpolated);

    }

    void AVFrameToMat(const AVFrame* frame, cv::Mat& mat){
        int width = frame->width;
        int height = frame->height;

        // Allocate the opencv mat and store its stride in a 1-element array
        if (mat.rows != height || mat.cols != width || mat.type() != CV_8UC3) mat = cv::Mat(height, width, CV_8UC3);
        int cvLinesizes[1];
        cvLinesizes[0] = static_cast<int>(mat.step1());

        // Convert the colour format and write directly to the opencv matrix
        sws_scale(this->swsCtx, reinterpret_cast<const uint8_t *const *>(frame->data), frame->linesize, 0, height, &mat.data, cvLinesizes);
        cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);

    }


    void printReport(){
        printf("Total checked:%d\nTotal skipped:%d\nSkipped by MSE:%d\nSkipped by Intelligent Model%d\n",
        this->total_checked, this->total_skipped, this->total_skipped_MSE, this->total_skipped_model_decision);
    }
};



#endif //VIDEO_SKIPPING_SKIPPING_H
