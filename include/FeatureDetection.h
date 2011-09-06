/*
 * Gesichtertausch
 *
 * Copyright (C) 2011 The Product GbR Kochlik + Paul
 *
 */


#ifndef Gesichtertausch_FeatureDetection_h
#define Gesichtertausch_FeatureDetection_h

#include "Defines.h"
#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"

#include "CinderOpenCv.h"
#include "opencv2/opencv.hpp"

#ifdef COMPILE_CAPTURE_FIREFLY
#include <dc1394/dc1394.h>
#include <dc1394/capture.h>
#endif

using namespace ci;
using namespace ci::app;
using namespace std;

class FeatureDetection {
public:
    virtual void setup(int pCaptureWidth, 
                       int pCaptureHeight, 
                       int pDetectionWidth, 
                       int pDetectionHeight,
                       int pCameraID) = 0;
    virtual void update(gl::Texture& pTexture, 
                        vector<Rectf>& pFaces) = 0;
    virtual void dispose() = 0;

    float DETECT_SCALE_FACTOR;
    int DETECT_MIN_NEIGHBORS;
    int DETECT_FLAGS;
protected:
    cv::CascadeClassifier	mFaceCascade;
    void doFeatureDetection(const cv::Mat& pImage, 
                            vector<cv::Rect>& pFaces) {
        mFaceCascade.detectMultiScale( pImage, 
                                      pFaces, 
                                      DETECT_SCALE_FACTOR,//1.2, 
                                      DETECT_MIN_NEIGHBORS,//2, 
                                      DETECT_FLAGS,
                                      cv::Size(10,10), 
                                      cv::Size(100,100));
        /*
         CV_WRAP virtual void detectMultiScale( 
         const Mat& image,
         CV_OUT vector<Rect>& objects,
         double scaleFactor=1.1,
         int minNeighbors=3, 
         int flags=0,
         Size minSize=Size(),
         Size maxSize=Size() );
         */
        /*
         #define CV_HAAR_DO_CANNY_PRUNING    1
         #define CV_HAAR_SCALE_IMAGE         2
         #define CV_HAAR_FIND_BIGGEST_OBJECT 4
         #define CV_HAAR_DO_ROUGH_SEARCH     8
         */
    }
};

class FeatureDetectionCinder : public FeatureDetection {
public:
    void setup(int pCaptureWidth, 
               int pCaptureHeight, 
               int pDetectionWidth, 
               int pDetectionHeight,
               int pCameraID);
    void update(gl::Texture& pTexture, 
                vector<Rectf>& pFaces);
    void dispose();
    
private:
    Capture                 mCapture;
    Surface                 mSurface;
    int                     mDetectionWidth;
    int                     mDetectionHeight;
};

#ifdef COMPILE_CAPTURE_OPENCV
class FeatureDetectionOpenCV : public FeatureDetection {
public:
    void setup(int pCaptureWidth, 
               int pCaptureHeight, 
               int pDetectionWidth, 
               int pDetectionHeight,
               int pCameraID);
    void update(gl::Texture& pTexture, 
                vector<Rectf>& pFaces);
    void dispose();
    
private:
    void detect(    vector<Rectf>& pFaces,
                cv::Mat& pImage,
                cv::CascadeClassifier& pCascade, 
                double pScale);
    
    CvCapture*              mCapture;
};
#endif


#ifdef COMPILE_CAPTURE_FIREFLY
class FeatureDetectionFireFly : public FeatureDetection {
public:
    void setup(int pCaptureWidth, 
               int pCaptureHeight, 
               int pDetectionWidth, 
               int pDetectionHeight,
               int pCameraID);
    void update(gl::Texture& pTexture, 
                vector<Rectf>& pFaces);
    void dispose();
    
private:
    int                     mDetectionWidth;
    int                     mDetectionHeight;
    dc1394camera_t          *camera;
    dc1394error_t           err;
};
#endif

#endif
