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

/* FeatureDetectionCinder */

void FeatureDetectionCinder::setup(int pCaptureWidth, int pCaptureHeight) {
    mCapture = Capture( pCaptureWidth, pCaptureHeight );
    mCapture.start();
    
    mFaceCascade.load( App::getResourcePath( "haarcascade_frontalface_alt.xml" ) );
	mEyeCascade.load( App::getResourcePath( "haarcascade_eye.xml" ) );
}

void FeatureDetectionCinder::update(gl::Texture& pTexture, vector<Rectf>& pFaces) {
    if ( mCapture.checkNewFrame() ) {
        mSurface = mCapture.getSurface();
        
        if (pTexture) {
            pTexture.update( mSurface );
        }
        
        const int calcScale = 2; // calculate the image at half scale
        
        // create a grayscale copy of the input image
        cv::Mat grayCameraImage( toOcv( mSurface, CV_8UC1 ) );
        
        // scale it to half size, as dictated by the calcScale constant
        int scaledWidth = mSurface.getWidth() / calcScale;
        int scaledHeight = mSurface.getHeight() / calcScale; 
        cv::Mat smallImg( scaledHeight, scaledWidth, CV_8UC1 );
        cv::resize( grayCameraImage, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR );
        
        // equalize the histogram
        cv::equalizeHist( smallImg, smallImg );
        
        // clear out the previously deteced faces & eyes
        pFaces.clear();
        
        // detect the faces and iterate them, appending them to mFaces
        vector<cv::Rect> faces;
        mFaceCascade.detectMultiScale( smallImg, 
                                      faces, 
                                      1.2, 
                                      2, 
                                      CV_HAAR_DO_CANNY_PRUNING
                                      ,
                                      cv::Size(), 
                                      cv::Size());
        for( vector<cv::Rect>::const_iterator faceIter = faces.begin(); faceIter != faces.end(); ++faceIter ) {
            Rectf faceRect( fromOcv( *faceIter ) );
            faceRect *= calcScale;
            pFaces.push_back( faceRect );
        }        
    }
}

/* FeatureDetectionOpenCV */

void FeatureDetectionOpenCV::setup(int pCaptureWidth, int pCaptureHeight) {
    capture = cvCreateCameraCapture( 0 );
    if(!capture) {
        cout << "Capture from CAM " << " didn't work" << endl;
    }
    cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH, pCaptureWidth );
    cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT, pCaptureHeight );
    
    mFaceCascade.load( App::getResourcePath( "haarcascade_frontalface_alt.xml" ) );
	mEyeCascade.load( App::getResourcePath( "haarcascade_eye.xml" ) );
}

void FeatureDetectionOpenCV::update(gl::Texture& pTexture, vector<Rectf>& pFaces) {
    cv::Mat frame, frameCopy, image;
    if( capture ) {
        IplImage* iplImg = cvQueryFrame( capture );
        frame = iplImg;
        if( frame.empty() ) {
            return;
        }
        
        if( iplImg->origin == IPL_ORIGIN_TL ) {
            frame.copyTo( frameCopy );
        } else {
            flip( frame, frameCopy, 0 );
        }
//        detectAndDraw( frameCopy, mFaceCascade, mEyeCascade, 1. );
    }
}

//void GesichtertauschApp::detectAndDraw( cv::Mat& img,
//                                       cv::CascadeClassifier& cascade, 
//                                       cv::CascadeClassifier& nestedCascade,
//                                       double scale)
//{
//    // clear out the previously deteced faces & eyes
//	mFaces.clear();
//	mEyes.clear();
//    
//    int i = 0;
//    double t = 0;
//    vector<cv::Rect> faces;
//    //    const static cv::Scalar colors[] =  { CV_RGB(0,0,255),
//    //        CV_RGB(0,128,255),
//    //        CV_RGB(0,255,255),
//    //        CV_RGB(0,255,0),
//    //        CV_RGB(255,128,0),
//    //        CV_RGB(255,255,0),
//    //        CV_RGB(255,0,0),
//    //        CV_RGB(255,0,255)} ;
//    cv::Mat gray, smallImg( cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1 );
//    
//    cvtColor( img, gray, CV_BGR2GRAY );
//    cv::resize( gray, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR );
//    cv::equalizeHist( smallImg, smallImg );
//    
//    t = (double)cvGetTickCount();
//    cascade.detectMultiScale( smallImg, faces,
//                             1.1, 2, 0
//                             //|CV_HAAR_FIND_BIGGEST_OBJECT
//                             //|CV_HAAR_DO_ROUGH_SEARCH
//                             |CV_HAAR_SCALE_IMAGE
//                             ,
//                             cv::Size(30, 30) );
//    t = (double)cvGetTickCount() - t;
//    
//    printf( "detection time = %g ms\n", t/((double)cvGetTickFrequency()*1000.) );
//    console() << faces.size() << endl;
//    
//    for( vector<cv::Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++ ) {
//        cv::Mat smallImgROI;
//        vector<cv::Rect> nestedObjects;
//        //        Point center;
//        //        Scalar color = colors[i%8];
//        //        int radius;
//        //        center.x = cvRound((r->x + r->width*0.5)*scale);
//        //        center.y = cvRound((r->y + r->height*0.5)*scale);
//        //        radius = cvRound((r->width + r->height)*0.25*scale);
//        //        circle( img, center, radius, color, 3, 8, 0 );
//        //        if( nestedCascade.empty() )
//        //            continue;
//        
//        /*
//         CV_WRAP virtual void detectMultiScale(
//         const Mat& image,
//         CV_OUT vector<Rect>& objects,
//         double scaleFactor=1.1,
//         int minNeighbors=3, int flags=0,
//         Size minSize=Size(),
//         Size maxSize=Size() );
//         */
//        
//        smallImgROI = smallImg(*r);
//        //        nestedCascade.detectMultiScale( smallImgROI,
//        //                                       nestedObjects,
//        //                                       1.1,
//        //                                       2,
//        //                                       0
//        //                                       //|CV_HAAR_FIND_BIGGEST_OBJECT
//        //                                       //|CV_HAAR_DO_ROUGH_SEARCH
//        //                                       //|CV_HAAR_DO_CANNY_PRUNING
//        //                                       |CV_HAAR_SCALE_IMAGE
//        //                                       ,
//        //                                       cv::Size(30, 30),
//        //                                       cv::Size(200, 200));
//        nestedCascade.detectMultiScale( smallImgROI, nestedObjects);
//        
//        Rectf faceRect( fromOcv( *r ) );
//		faceRect *= scale;
//		mFaces.push_back( faceRect );
//        
//        for( vector<cv::Rect>::const_iterator nr = nestedObjects.begin(); nr != nestedObjects.end(); nr++ ) {
//            //            center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
//            //            center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
//            //            radius = cvRound((nr->width + nr->height)*0.25*scale);
//            //            circle( img, center, radius, color, 3, 8, 0 );
//        }
//    }
//    //    cv::imshow( "result", img );
//}





