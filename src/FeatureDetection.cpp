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
    
    if (pCaptureWidth != 752 && pCaptureHeight != 480) {
        console() << "### camera needs resolution of 752x480 pixels." << endl;    
    }
    
    mDetectionWidth = pDetectionWidth;
    mDetectionHeight = pDetectionHeight;
    mFaceCascade.load( App::getResourcePath( HAARCASCADE[0] ) );

    ///////////////////////////////////
    

    /* get camera */
    dc1394camera_list_t * mCameraList;   
    dc1394_t * d;
    
    d = dc1394_new ();
    check_error(dc1394_camera_enumerate (d, &mCameraList));
    if (mCameraList->num == 0) {
        console() << "### no camera found!" << endl;
    }
    
    const int mCameraID = 0;
    mCamera = dc1394_camera_new (d, mCameraList->ids[mCameraID].guid);
    
    dc1394_iso_release_bandwidth(mCamera, INT_MAX);
	for (int channel = 0; channel < 64; channel++) {
		dc1394_iso_release_channel(mCamera, channel);
	}
    
    if (!mCamera) {
        console() << "### failed to initialize camera with guid %llx" << mCameraList->ids[mCameraID].guid << endl;
    } else {
        console() << "+++ camera (GUID): " << mCamera->vendor << "(" << mCamera->guid << ")" << endl;
    }
    dc1394_camera_free_list (mCameraList);
    
    /* setup camera */    
    dc1394video_mode_t mVideoMode = DC1394_VIDEO_MODE_FORMAT7_0;
    unsigned int mMaxWidth;
    unsigned int mMaxHeight;
    
    check_error(dc1394_video_set_operation_mode(mCamera, DC1394_OPERATION_MODE_1394B));
    check_error(dc1394_format7_get_max_image_size(mCamera, mVideoMode, &mMaxWidth, &mMaxHeight));
    console() << "+++ maximum size for current Format7 mode is " << mMaxWidth << "x" << mMaxHeight << endl;
    
    check_error(dc1394_format7_set_roi(mCamera, 
                                       mVideoMode, 
                                       DC1394_COLOR_CODING_RAW8,
                                       DC1394_USE_MAX_AVAIL, 
                                       0, 0, mMaxWidth, mMaxHeight)); 
    check_error(dc1394_video_set_mode(mCamera, mVideoMode));
    const int NUMBER_DMA_BUFFERS = 2;
    check_error(dc1394_capture_setup(mCamera, 
                                     NUMBER_DMA_BUFFERS, 
                                     DC1394_CAPTURE_FLAGS_DEFAULT));
    
    /* grab first frame and dump info */
    check_error(dc1394_video_set_transmission(mCamera, DC1394_ON));
    
    console() << "+++ capture first frame ..." << endl;
    dc1394video_frame_t* mFrame = NULL;
    check_error(dc1394_capture_dequeue(mCamera, DC1394_CAPTURE_POLICY_WAIT, &mFrame));
    if (mFrame) {
        console() << "+++ dimensions        : " << mFrame->size[0] << ", " << mFrame->size[1] << endl;
        console() << "+++ color coding      : " << mFrame->color_coding << endl;
        console() << "+++ data depth        : " << mFrame->data_depth << endl;
        console() << "+++ stride            : " << mFrame->stride << endl;
        console() << "+++ video_mode        : " << mFrame->video_mode << endl;
        console() << "+++ total_bytes       : " << mFrame->total_bytes << endl;
        console() << "+++ packet_size       : " << mFrame->packet_size << endl;
        console() << "+++ packets_per_frame : " << mFrame->packets_per_frame << endl;
        check_error(dc1394_capture_enqueue(mCamera, mFrame));
    }    
}

void FeatureDetectionFireFly::check_error(dc1394error_t pError) {
    if (pError) {
        console() << "### ERROR #" << pError << " occured." << endl;
    }
}

void FeatureDetectionFireFly::update(gl::Texture& pTexture, 
                                     vector<Rectf>& pFaces) {
    
    dc1394video_frame_t * mFrame = NULL;
    check_error(dc1394_capture_dequeue(mCamera, DC1394_CAPTURE_POLICY_POLL, &mFrame));
    if (mFrame) {
        const unsigned int RGB_CHANNELS = 3;
        const unsigned int mRGBImageSize = mFrame->size[0] * mFrame->size[1] * RGB_CHANNELS;
        unsigned char * mRGBImageData = new unsigned char [mRGBImageSize];
        check_error(dc1394_bayer_decoding_8bit(mFrame->image, 
                                               mRGBImageData, 
                                               mFrame->size[0], mFrame->size[1],
                                               DC1394_COLOR_FILTER_RGGB,
                                               DC1394_BAYER_METHOD_BILINEAR));
        Surface mSurface = Surface(mRGBImageData,
                                   mFrame->size[0], mFrame->size[1], 
                                   mFrame->size[0] * RGB_CHANNELS, 
                                   SurfaceChannelOrder::RGB);
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

        ////////////
        
        check_error(dc1394_capture_enqueue(mCamera, mFrame));
        delete [] mRGBImageData;
    }
}

void FeatureDetectionFireFly::dispose() {
    dc1394_video_set_transmission(mCamera, DC1394_OFF);
    dc1394_capture_stop(mCamera);
    dc1394_reset_bus (mCamera);
    dc1394_camera_free(mCamera);
}

#endif

