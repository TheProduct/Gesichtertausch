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

void FeatureDetectionCinder::setup(int pCaptureWidth, 
                                   int pCaptureHeight, 
                                   int pDetectionWidth, 
                                   int pDetectionHeight,
                                   int pCameraID) {
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

void FeatureDetectionCinder::dispose() {
    mCapture.stop();
}

/*
 *
 * FeatureDetectionOpenCV 
 *
 */
#ifdef COMPILE_CAPTURE_OPENCV
void FeatureDetectionOpenCV::setup(int pCaptureWidth,
                                   int pCaptureHeight, 
                                   int pDetectionWidth,
                                   int pDetectionHeight,
                                   int pCameraID) {
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

void FeatureDetectionOpenCV::detect(vector<Rectf>& pFaces,
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

void FeatureDetectionOpenCV::dispose() {
    cvReleaseCapture(mCapture);
}

#endif

/*
 *
 * FeatureDetectionFireFly 
 *
 */
#ifdef COMPILE_CAPTURE_FIREFLY
void FeatureDetectionFireFly::setup(int pCaptureWidth, 
                                    int pCaptureHeight, 
                                    int pDetectionWidth, 
                                    int pDetectionHeight,
                                    int pCameraID) {
    
    mDetectionWidth = pDetectionWidth;
    mDetectionHeight = pDetectionHeight;
    mFaceCascade.load( App::getResourcePath( HAARCASCADE[0] ) );

    ///////////////////////////////////
    
    
    dc1394camera_list_t * list;   
    dc1394_t * d;
    
    d = dc1394_new ();
    err = dc1394_camera_enumerate (d, &list);
    if (list->num == 0) {
        console() << "### no camera found!" << endl;
    }
    
    camera = dc1394_camera_new (d, list->ids[0].guid);
    if (!camera) {
        console() << "### failed to initialize camera with guid %llx" << list->ids[0].guid << endl;
    }
    dc1394_camera_free_list (list);
    console() << "+++ camera (GUID): " << camera->vendor << "(" << camera->guid << ")" << endl;
    
    ////////////
    
        err=dc1394_video_set_operation_mode(camera, DC1394_OPERATION_MODE_1394B);
        err=dc1394_video_set_iso_speed(camera, DC1394_ISO_SPEED_400);
        err=dc1394_video_set_mode(camera, DC1394_VIDEO_MODE_640x480_MONO8);
        err=dc1394_video_set_framerate(camera, DC1394_FRAMERATE_30);
        err=dc1394_capture_setup(camera, 2, DC1394_CAPTURE_FLAGS_DEFAULT);
    
    ////////////
    
    err=dc1394_video_set_transmission(camera, DC1394_ON);
    
    dc1394video_frame_t *frame=NULL;
    console() << "+++ capture ..." << endl;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_WAIT, &frame);
    if (frame) {
        console() << "+++ capture state: " << (err == 0 ? "OK" : "BAD") << endl;
        console() << "+++ dimensions   : " << frame->size[0] << ", " << frame->size[1] << endl;
        console() << "+++ color coding : " << frame->color_coding << endl;
        console() << "+++ data depth   : " << frame->data_depth << endl;
        console() << "+++ stride       : " << frame->stride << endl;
        console() << "+++ video_mode   : " << frame->video_mode << endl; // (90 == FORMAT7_2)
        console() << "+++ total_bytes  : " << frame->total_bytes << endl;
        console() << "+++ packet_size  : " << frame->packet_size << endl;
        console() << "+++ packets_per_frame : " << frame->packets_per_frame << endl;
        err = dc1394_capture_enqueue(camera, frame);
    }
}

void FeatureDetectionFireFly::update(gl::Texture& pTexture, 
                                     vector<Rectf>& pFaces) {
    dc1394video_frame_t *frame = NULL;
    err = dc1394_capture_dequeue(camera, DC1394_CAPTURE_POLICY_POLL, &frame);
    if (frame) {    
        unsigned char * mImageData;
        const unsigned int mSize = frame->size[0] * frame->size[1] * 4;
        mImageData = new unsigned char [mSize];
        for (int i=0; i < mSize; i+=4) {
            mImageData[i+0] = frame->image[i / 4];
            mImageData[i+1] = frame->image[i / 4];
            mImageData[i+2] = frame->image[i / 4];
            mImageData[i+3] = 255;
        }
        
        /* update texture */
        Surface mSurface = Surface(mImageData, 
                                   frame->size[0], 
                                   frame->size[1], 
                                   frame->size[0] * 4, 
                                   SurfaceChannelOrder::RGBA);
        if (pTexture) {
            pTexture.update(mSurface);
        }
        
        /* update faces */
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
        
        err = dc1394_capture_enqueue(camera, frame);
        delete [] mImageData;
    }
}

void FeatureDetectionFireFly::dispose() {
    dc1394_video_set_transmission(camera, DC1394_OFF);
    dc1394_capture_stop(camera);
    dc1394_reset_bus (camera);
    dc1394_camera_free(camera);
}

#endif

