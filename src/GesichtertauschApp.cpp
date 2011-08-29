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

#include "FeatureDetection.h"

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
    FeatureDetection*       mFaceDetection;
	vector<Rectf>			mFaces;
    gl::Texture             mCameraTexture;


    /* properties */
    SimpleGUI*              mGui;

    int                     WINDOW_WIDTH;
    int                     WINDOW_HEIGHT;
    
    int                     CAMERA_WIDTH;
    int                     CAMERA_HEIGHT;
    
    int                     FRAME_RATE;
    
    bool                    FULLSCREEN;
    
    /* output */
    LabelControl*           mFaceOut;
    LabelControl*           mFPSOut;

	vector<Rectf>			mEyes;
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
    
    setFullScreen( FULLSCREEN );
    if (FULLSCREEN) {
        hideCursor();
    }
    setWindowSize( WINDOW_WIDTH, WINDOW_HEIGHT );

    /* setting up capture device */
    mCameraTexture = gl::Texture(CAMERA_WIDTH, CAMERA_HEIGHT);
    mFaceDetection = new FeatureDetectionCinder();
    mFaceDetection->setup(CAMERA_WIDTH, CAMERA_HEIGHT);
}

void GesichtertauschApp::update() {
    setFrameRate( FRAME_RATE );

    {
        stringstream mStr;
        mStr << "FPS: " << getAverageFps();
        mFPSOut->setText(mStr.str());
    }
    {
        stringstream mStr;
        mStr << "FACES: " << mFaces.size();
        mFaceOut->setText(mStr.str());   
    }
    
    mFaceDetection->update(mCameraTexture, mFaces);
}

void GesichtertauschApp::draw() {
        if ( ! mCameraTexture ) {
            return;
        }
        
        gl::setMatricesWindow( getWindowSize() );
        gl::enableAlphaBlending();
        
        // draw the webcam image
        gl::color( Color( 1, 1, 1 ) );
        gl::draw( mCameraTexture, Rectf(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT) );
        mCameraTexture.disable();

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
