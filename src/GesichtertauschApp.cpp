/*
 * Gesichtertausch
 *
 * Copyright (C) 2011 The Product GbR Kochlik + Paul
 *
 */

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"

#include "CinderOpenCv.h"

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "SimpleGUI.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mowa::sgui;

const std::string RES_SETTINGS = "settings.txt";

class GesichtertauschApp : public AppBasic {
public:
	void setup();
	void mouseDown( MouseEvent );
    void keyDown( KeyEvent );
	void update();
	void draw();
    
private:

    
    
    void updateFacesCV();
    void detectAndDraw( cv::Mat& img,
                       cv::CascadeClassifier& cascade, 
                       cv::CascadeClassifier& nestedCascade,
                       double scale);
    void updateFacesCinder( Surface cameraImage );
    
    bool            USE_CV_CAPTURE;
    CvCapture*      capture;
    Capture			mCapture;
	gl::Texture		mCameraTexture;
    SimpleGUI*      mGui;

    cv::CascadeClassifier	mFaceCascade, mEyeCascade;
	vector<Rectf>			mFaces, mEyes;

    /* properties */
    int WINDOW_WIDTH;
    int WINDOW_HEIGHT;

    int CAMERA_WIDTH;
    int CAMERA_HEIGHT;
    
    int FRAME_RATE;
    
    bool FULLSCREEN;
    
    /* output */
    LabelControl* mFaceOut;
    LabelControl* mFPSOut;
};


void GesichtertauschApp::setup()
{      
    /* settings */
    mGui = new SimpleGUI(this);
    mGui->addParam("WINDOW_WIDTH", &WINDOW_WIDTH, 0, 2048, 640);
    mGui->addParam("WINDOW_HEIGHT", &WINDOW_HEIGHT, 0, 2048, 480);
    mGui->addParam("CAMERA_WIDTH", &CAMERA_WIDTH, 0, 2048, 640);
    mGui->addParam("CAMERA_HEIGHT", &CAMERA_HEIGHT, 0, 2048, 480);
    mGui->addParam("FULLSCREEN", &FULLSCREEN, false, 0);
    mGui->addParam("FRAME_RATE", &FRAME_RATE, 1, 120, 30);
    mGui->addSeparator();
    mFaceOut = mGui->addLabel("");
    mFPSOut = mGui->addLabel("");
    
    /* clean up controller window */
    mGui->getControlByName("WINDOW_WIDTH")->active=false;
    mGui->getControlByName("WINDOW_HEIGHT")->active=false;
    mGui->getControlByName("CAMERA_WIDTH")->active=false;
    mGui->getControlByName("CAMERA_HEIGHT")->active=false;
    mGui->getControlByName("FULLSCREEN")->active=false;
    
    mGui->load(getResourcePath(RES_SETTINGS));
    mGui->setEnabled(false);
    mGui->dump(); 
    
    /* --- */
    mFaceCascade.load( getResourcePath( "haarcascade_frontalface_alt.xml" ) );
	mEyeCascade.load( getResourcePath( "haarcascade_eye.xml" ) );
    
    USE_CV_CAPTURE = false;
    if (USE_CV_CAPTURE) {
        capture = cvCreateCameraCapture( 0 );
        if(!capture) cout << "Capture from CAM " << " didn't work" << endl;
        cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_WIDTH, 320 );
        cvSetCaptureProperty( capture, CV_CAP_PROP_FRAME_HEIGHT, 240 );
    } else {
        mCapture = Capture( CAMERA_WIDTH, CAMERA_HEIGHT );
        mCapture.start();
    }
    
    setFullScreen( FULLSCREEN );
    if (FULLSCREEN) {
        hideCursor();
    }
    setWindowSize( WINDOW_WIDTH, WINDOW_HEIGHT );
}

void GesichtertauschApp::update()
{
    setFrameRate( FRAME_RATE );

    stringstream mStr;
    mStr << "FPS: " << getAverageFps();
    mFPSOut->setText(mStr.str());

    if (USE_CV_CAPTURE) {
        updateFacesCV();
    } else {
        if ( mCapture.checkNewFrame() ) {
            Surface surface = mCapture.getSurface();
            mCameraTexture = gl::Texture( surface );
            updateFacesCinder( surface );
        }
    }
}

void GesichtertauschApp::draw()
{
    if (USE_CV_CAPTURE) {
    } else {
        if ( ! mCameraTexture ) {
            return;
        }
        
        gl::setMatricesWindow( getWindowSize() );
        gl::enableAlphaBlending();
        
        // draw the webcam image
        gl::color( Color( 1, 1, 1 ) );
        gl::draw( mCameraTexture, Rectf(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT) );
        mCameraTexture.disable();
    }

    gl::scale(float(WINDOW_WIDTH) / float(CAMERA_WIDTH), float(WINDOW_HEIGHT) / float(CAMERA_HEIGHT));

	// draw the faces as transparent yellow rectangles
	gl::color( ColorA( 1, 1, 0, 0.45f ) );
	for( vector<Rectf>::const_iterator faceIter = mFaces.begin(); faceIter != mFaces.end(); ++faceIter ) {
		gl::drawSolidRect( *faceIter );
    }
    
	// draw the eyes as transparent blue ellipses
	gl::color( ColorA( 0, 0, 1, 0.35f ) );
	for( vector<Rectf>::const_iterator eyeIter = mEyes.begin(); eyeIter != mEyes.end(); ++eyeIter ) {
		gl::drawSolidCircle( eyeIter->getCenter(), eyeIter->getWidth() / 2 );
    }
    
    mGui->draw();
}

void GesichtertauschApp::updateFacesCV()
{
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
        detectAndDraw( frameCopy, mFaceCascade, mEyeCascade, 1. );
        console() << "FPS: " << getAverageFps() << endl;
    }
}

void GesichtertauschApp::detectAndDraw( cv::Mat& img,
                                       cv::CascadeClassifier& cascade, 
                                       cv::CascadeClassifier& nestedCascade,
                                       double scale)
{
    // clear out the previously deteced faces & eyes
	mFaces.clear();
	mEyes.clear();

    int i = 0;
    double t = 0;
    vector<cv::Rect> faces;
//    const static cv::Scalar colors[] =  { CV_RGB(0,0,255),
//        CV_RGB(0,128,255),
//        CV_RGB(0,255,255),
//        CV_RGB(0,255,0),
//        CV_RGB(255,128,0),
//        CV_RGB(255,255,0),
//        CV_RGB(255,0,0),
//        CV_RGB(255,0,255)} ;
    cv::Mat gray, smallImg( cvRound (img.rows/scale), cvRound(img.cols/scale), CV_8UC1 );
    
    cvtColor( img, gray, CV_BGR2GRAY );
    cv::resize( gray, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR );
    cv::equalizeHist( smallImg, smallImg );
    
    t = (double)cvGetTickCount();
    cascade.detectMultiScale( smallImg, faces,
                             1.1, 2, 0
                             //|CV_HAAR_FIND_BIGGEST_OBJECT
                             //|CV_HAAR_DO_ROUGH_SEARCH
                             |CV_HAAR_SCALE_IMAGE
                             ,
                             cv::Size(30, 30) );
    t = (double)cvGetTickCount() - t;
    
    printf( "detection time = %g ms\n", t/((double)cvGetTickFrequency()*1000.) );
    console() << faces.size() << endl;
    
    for( vector<cv::Rect>::const_iterator r = faces.begin(); r != faces.end(); r++, i++ ) {
        cv::Mat smallImgROI;
        vector<cv::Rect> nestedObjects;
//        Point center;
//        Scalar color = colors[i%8];
//        int radius;
//        center.x = cvRound((r->x + r->width*0.5)*scale);
//        center.y = cvRound((r->y + r->height*0.5)*scale);
//        radius = cvRound((r->width + r->height)*0.25*scale);
//        circle( img, center, radius, color, 3, 8, 0 );
//        if( nestedCascade.empty() )
//            continue;
        
        /*
         CV_WRAP virtual void detectMultiScale(
         const Mat& image,
         CV_OUT vector<Rect>& objects,
         double scaleFactor=1.1,
         int minNeighbors=3, int flags=0,
         Size minSize=Size(),
         Size maxSize=Size() );
         */
        
        smallImgROI = smallImg(*r);
//        nestedCascade.detectMultiScale( smallImgROI,
//                                       nestedObjects,
//                                       1.1,
//                                       2,
//                                       0
//                                       //|CV_HAAR_FIND_BIGGEST_OBJECT
//                                       //|CV_HAAR_DO_ROUGH_SEARCH
//                                       //|CV_HAAR_DO_CANNY_PRUNING
//                                       |CV_HAAR_SCALE_IMAGE
//                                       ,
//                                       cv::Size(30, 30),
//                                       cv::Size(200, 200));
        nestedCascade.detectMultiScale( smallImgROI, nestedObjects);

        Rectf faceRect( fromOcv( *r ) );
		faceRect *= scale;
		mFaces.push_back( faceRect );
        
        for( vector<cv::Rect>::const_iterator nr = nestedObjects.begin(); nr != nestedObjects.end(); nr++ ) {
//            center.x = cvRound((r->x + nr->x + nr->width*0.5)*scale);
//            center.y = cvRound((r->y + nr->y + nr->height*0.5)*scale);
//            radius = cvRound((nr->width + nr->height)*0.25*scale);
//            circle( img, center, radius, color, 3, 8, 0 );
        }
    }
//    cv::imshow( "result", img );
}


void GesichtertauschApp::updateFacesCinder( Surface cameraImage )
{
	const int calcScale = 2; // calculate the image at half scale
    
	// create a grayscale copy of the input image
	cv::Mat grayCameraImage( toOcv( cameraImage, CV_8UC1 ) );
    
	// scale it to half size, as dictated by the calcScale constant
	int scaledWidth = cameraImage.getWidth() / calcScale;
	int scaledHeight = cameraImage.getHeight() / calcScale; 
	cv::Mat smallImg( scaledHeight, scaledWidth, CV_8UC1 );
	cv::resize( grayCameraImage, smallImg, smallImg.size(), 0, 0, cv::INTER_LINEAR );
	
	// equalize the histogram
	cv::equalizeHist( smallImg, smallImg );
    
	// clear out the previously deteced faces & eyes
	mFaces.clear();
	mEyes.clear();
    
	// detect the faces and iterate them, appending them to mFaces
	vector<cv::Rect> faces;
	mFaceCascade.detectMultiScale( smallImg, 
                                  faces, 
                                  1.2, 
                                  2, 
//                                  CV_HAAR_FIND_BIGGEST_OBJECT
//                                  CV_HAAR_DO_ROUGH_SEARCH
                                  CV_HAAR_DO_CANNY_PRUNING
//                                  CV_HAAR_SCALE_IMAGE
                                  ,
                                  cv::Size(), 
                                  cv::Size());
	for( vector<cv::Rect>::const_iterator faceIter = faces.begin(); faceIter != faces.end(); ++faceIter ) {
		Rectf faceRect( fromOcv( *faceIter ) );
		faceRect *= calcScale;
		mFaces.push_back( faceRect );
		
		// detect eyes within this face and iterate them, appending them to mEyes
//		vector<cv::Rect> eyes;
//		mEyeCascade.detectMultiScale( smallImg( *faceIter ), eyes );
//		for( vector<cv::Rect>::const_iterator eyeIter = eyes.begin(); eyeIter != eyes.end(); ++eyeIter ) {
//			Rectf eyeRect( fromOcv( *eyeIter ) );
//			eyeRect = eyeRect * calcScale + faceRect.getUpperLeft();
//			mEyes.push_back( eyeRect );
//		}
	}
    
    stringstream mStr;
    mStr << "FACES: " << faces.size();
    mFaceOut->setText(mStr.str());
}

void GesichtertauschApp::mouseDown( MouseEvent event )
{
}

void GesichtertauschApp::keyDown( KeyEvent pEvent )
{
    switch(pEvent.getChar()) {				
        case 'd': mGui->dump(); break;
        case 'l': mGui->load(getResourcePath(RES_SETTINGS)); break;
        case 's': mGui->save(getResourcePath(RES_SETTINGS)); break;
    }
    switch(pEvent.getCode()) {
        case KeyEvent::KEY_ESCAPE:  setFullScreen( false ); quit(); break;
        case KeyEvent::KEY_SPACE: mGui->setEnabled(!mGui->isEnabled());break;
    }
}

CINDER_APP_BASIC( GesichtertauschApp, RendererGl )
