/*
 * Gesichtertausch
 *
 * Copyright (C) 2011 The Product GbR Kochlik + Paul
 *
 */

#include "FeatureDetection.h"

#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/app/App.h"


using namespace ci;
using namespace ci::app;
using namespace std;


const std::string HAARCASCADE[] = {
    "haarcascade_frontalface_default.xml", // 7.0 FPS
    "haarcascade_frontalface_alt.xml", // 3.5 FPS
    "haarcascade_frontalface_alt2.xml", //3.5 FPS
    "haarcascade_frontalface_alt_tree.xml" // 6.0 FPS
};

/*
 *
 * FeatureDetectionCinder 
 *
 */

void FeatureDetectionCinder::setup(int pCaptureWidth, int pCaptureHeight, int pDetectionWidth, int pDetectionHeight) {
    mDetectionWidth = pDetectionWidth;
    mDetectionHeight = pDetectionHeight;
    mCapture = Capture( pCaptureWidth, pCaptureHeight );
    mCapture.start();    
    mFaceCascade.load( App::getResourcePath( HAARCASCADE[0] ) );
}

void FeatureDetectionCinder::update(gl::Texture& pTexture, vector<Rectf>& pFaces) {
    if ( mCapture.checkNewFrame() ) {
        mSurface = mCapture.getSurface();
        
        if (pTexture) {
            pTexture.update( mSurface );
        }
        
        // create a grayscale copy of the input image
        cv::Mat grayCameraImage( toOcv( mSurface, CV_8UC1 ) );
                
        /* scale image */
        cv::Mat mScaledImage( mDetectionHeight, mDetectionWidth, CV_8UC1 );
        cv::resize( grayCameraImage, mScaledImage, mScaledImage.size(), 0, 0, cv::INTER_LINEAR );
        
        // equalize the histogram
        bool equalize_the_histogram = true;
        if (equalize_the_histogram) {
            cv::equalizeHist( mScaledImage, mScaledImage );
        }
        
        // clear out the previously deteced faces & eyes
        pFaces.clear();
        
        // detect the faces and iterate them, appending them to mFaces
        vector<cv::Rect> mFaces;
        doFeatureDetection(mScaledImage, mFaces);
        for( vector<cv::Rect>::const_iterator faceIter = mFaces.begin(); faceIter != mFaces.end(); ++faceIter ) {
            pFaces.push_back( fromOcv( *faceIter ) );
        }        
    }
}

/*
 *
 * FeatureDetectionOpenCV 
 *
 */

void FeatureDetectionOpenCV::setup(int pCaptureWidth, int pCaptureHeight, int pDetectionWidth, int pDetectionHeightt) {
    mCapture = cvCreateCameraCapture( 0 );
    if(!mCapture) {
        console() << "Capture from CAM " << " didn't work" << endl;
    }
    cvSetCaptureProperty( mCapture, CV_CAP_PROP_FRAME_WIDTH, pCaptureWidth );
    cvSetCaptureProperty( mCapture, CV_CAP_PROP_FRAME_HEIGHT, pCaptureHeight );
    
    mFaceCascade.load( App::getResourcePath( "haarcascade_frontalface_alt.xml" ) );
}

void FeatureDetectionOpenCV::update(gl::Texture& pTexture, vector<Rectf>& pFaces) {
    cv::Mat frame, frameCopy, image;
    if( mCapture ) {
        IplImage* iplImg = cvQueryFrame( mCapture );
        frame = iplImg;
        if( frame.empty() ) {
            return;
        }
        
        if( iplImg->origin == IPL_ORIGIN_TL ) {
            frame.copyTo( frameCopy );
        } else {
            flip( frame, frameCopy, 0 );
        }
        
        if (pTexture) {
            cv::Mat mImage = iplImg;
            pTexture.update( fromOcv(mImage) );
        }
        
        detect( pFaces, frameCopy, mFaceCascade, 1. );
    }
}

void FeatureDetectionOpenCV::detect(    vector<Rectf>& pFaces,
                                    cv::Mat& img,
                                    cv::CascadeClassifier& cascade, 
                                    double scale)
{
    // clear out the previously deteced faces
	pFaces.clear();
	
    int i = 0;
    vector<cv::Rect> faces;
    
    cv::Mat gray, smallImg( cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1 );
    
    cvtColor( img, gray, CV_BGR2GRAY );
    cv::resize( gray, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR );
    cv::equalizeHist( smallImg, smallImg );
    
    doFeatureDetection(smallImg, faces);
    
    for( vector<cv::Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++ ) {
        cv::Mat smallImgROI;
        smallImgROI = smallImg(*r);        
        Rectf faceRect( fromOcv( *r ) );
		faceRect *= scale;
		pFaces.push_back( faceRect );
    }
}





