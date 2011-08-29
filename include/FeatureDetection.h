/*
 * Gesichtertausch
 *
 * Copyright (C) 2011 The Product GbR Kochlik + Paul
 *
 */

#ifndef Gesichtertausch_FeatureDetection_h
#define Gesichtertausch_FeatureDetection_h

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"

#include "CinderOpenCv.h"
#include "opencv2/opencv.hpp"


using namespace ci;
using namespace ci::app;
using namespace std;

class FeatureDetection {
public:
    virtual void setup(int pCaptureWidth, int pCaptureHeight) = 0;
    virtual void update(gl::Texture& pTexture, vector<Rectf>& pFaces) = 0;

    cv::CascadeClassifier	mFaceCascade;

private:
    void doFeatureDetection(cv::Mat& smallImg, vector<cv::Rect>& faces) {
        mFaceCascade.detectMultiScale( smallImg, 
                                      faces, 
                                      1.2, 
                                      2, 
                                      CV_HAAR_DO_CANNY_PRUNING,
                                      cv::Size(), 
                                      cv::Size());
    }
};

class FeatureDetectionCinder : public FeatureDetection {
public:
    void setup(int pCaptureWidth, int pCaptureHeight);
    void update(gl::Texture& pTexture, vector<Rectf>& pFaces);
    
private:
    Capture                 mCapture;
    Surface                 mSurface;
};

class FeatureDetectionOpenCV : public FeatureDetection {
public:
    void setup(int pCaptureWidth, int pCaptureHeight);
    void update(gl::Texture& pTexture, vector<Rectf>& pFaces);
    
private:
    void detect(    vector<Rectf>& pFaces,
                cv::Mat& pImage,
                cv::CascadeClassifier& pCascade, 
                double pScale);
    
    CvCapture*              mCapture;
};

#endif
