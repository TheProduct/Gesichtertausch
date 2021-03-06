/*
 * Gesichtertausch
 *
 * Copyright (C) 2011 The Product GbR Kochlik + Paul
 *
 */

#include "Defines.h"
#include "Resources.h"

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"
#include "cinder/gl/GlslProg.h"

#include "FeatureDetection.h"

#include "CinderOpenCv.h"

#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "OscSender.h"

#include "SimpleGUI.h"


using namespace ci;
using namespace ci::app;
using namespace std;
using namespace mowa::sgui;

const std::string RES_SETTINGS = "settings.txt";

/* switch resolution */
bool MyDisplaySwitchToMode (CGDirectDisplayID display, CFDictionaryRef mode);
int switch_resolution (int pWidth, int pHeight, double pRefreshRate);

class GesichtertauschApp : public AppBasic {
public:
	void setup();
	void mouseDown( MouseEvent );
    void keyDown( KeyEvent );
	void update();
	void draw();
    
private:
    struct FaceEntity {
        Rectf   border;
        float   lifetime;
        float   idletime;
        int     ID;
        bool    alive;
        bool    visible;
        Rectf   slice;
        
        friend std::ostream& operator<<( std::ostream &o, const FaceEntity &mEntity ) {
            return o << mEntity.ID << " : " << mEntity.border << " / " << mEntity.slice;
        }	
    };

    void    drawEntity(const FaceEntity& pEntity, const ColorA& pColor);
    void    heartbeat(double mDeltaTime);

    FeatureDetection*       mFaceDetection;
    gl::Texture             mCameraTexture;
    vector<FaceEntity>      mEntities;
    Font                    mFont;
    double                  mTime;
    int                     mSerialID;

    /* properties */
    SimpleGUI*              mGui;
    
    int                     WINDOW_WIDTH;
    int                     WINDOW_HEIGHT;    
    int                     CAMERA_WIDTH;
    int                     CAMERA_HEIGHT;
    int                     DETECTION_WIDTH;
    int                     DETECTION_HEIGHT;
    
    int                     FRAME_RATE;
    
    bool                    FULLSCREEN;
    int                     ENABLE_SHADER;
    int                     TRACKING;
    float                   MIN_TRACKING_DISTANCE;
    float                   TIME_BEFOR_IDLE_DEATH;
    float                   MIN_LIFETIME_TO_VIEW;
    ColorA                  FACE_COLOR_UNO;
    ColorA                  FACE_COLOR_DUO;
    ColorA                  BACKGROUND_IMAGE_COLOR;
    float                   FACE_BORDER_SCALE;
    float                   FACE_FADE_BORDER_SCALE;

        
    /* output */
    LabelControl*           mFaceOut;
    LabelControl*           mFPSOut;
        
    /* shader */
    gl::GlslProg            mShader;
    
    /* watchdog */
    osc::Sender     mWatchdogSender;
	std::string     mWatchdogHost;
	int             mWatchdogPort;
    float           mWatchdogCounter;
    float           mWatchdogInterval;
};


void GesichtertauschApp::setup() {
    
    console() << "+++ Gesichtertausch (PID " << getpid() << ")." << endl;
    
    mTime = 0.0;
    mSerialID = 0;
    FACE_COLOR_UNO = ColorA(0, 1, 1, 1);
    FACE_COLOR_DUO = ColorA(1, 0, 0, 1);
    BACKGROUND_IMAGE_COLOR = ColorA(1, 1, 1, 1);
    
    /* watchdog */
    mWatchdogHost = "localhost";
	mWatchdogPort = 8080;
	mWatchdogSender.setup(mWatchdogHost, mWatchdogPort);
    mWatchdogCounter = 0.0;
    mWatchdogInterval = 2.5;
    
    /* shader */
    try {
		mShader = gl::GlslProg( loadResource( RES_PASSTHRU_VERT ), loadResource( RES_BLUR_FRAG ) );
	}
	catch( gl::GlslProgCompileExc &exc ) {
		console() << "Shader compile error: " << std::endl;
		console() << exc.what();
	}
	catch( ... ) {
		console() << "Unable to load shader" << std::endl;
	}
    
    /* settings */
    mGui = new SimpleGUI(this);
    mGui->addParam("WINDOW_WIDTH", &WINDOW_WIDTH, 0, 2048, 640);
    mGui->addParam("WINDOW_HEIGHT", &WINDOW_HEIGHT, 0, 2048, 480);
    mGui->addParam("CAMERA_WIDTH", &CAMERA_WIDTH, 0, 2048, 640);
    mGui->addParam("CAMERA_HEIGHT", &CAMERA_HEIGHT, 0, 2048, 480);
    mGui->addParam("DETECTION_WIDTH", &DETECTION_WIDTH, 0, 2048, 320);
    mGui->addParam("DETECTION_HEIGHT", &DETECTION_HEIGHT, 0, 2048, 240);
    mGui->addParam("FULLSCREEN", &FULLSCREEN, false, 0);
    mGui->addParam("TRACKING", &TRACKING, 0, 1, 0);
    //
    mGui->addParam("FRAME_RATE", &FRAME_RATE, 1, 120, 30);
    mGui->addParam("MIN_TRACKING_DISTANCE", &MIN_TRACKING_DISTANCE, 1, 100, 50);
    mGui->addParam("TIME_BEFOR_IDLE_DEATH", &TIME_BEFOR_IDLE_DEATH, 0, 10, 0.5);
    mGui->addParam("MIN_LIFETIME_TO_VIEW", &MIN_LIFETIME_TO_VIEW, 0, 10, 1.0);
    mGui->addParam("ENABLE_SHADER", &ENABLE_SHADER, 0, 1, 0);
    mGui->addParam("FACE_COLOR_UNO", &FACE_COLOR_UNO, ColorA(0, 1, 1, 1), SimpleGUI::RGB);
    mGui->addParam("FACE_COLOR_DUO", &FACE_COLOR_DUO, ColorA(1, 1, 1, 1), SimpleGUI::RGB);
    mGui->addParam("BACKGROUND_IMAGE_COLOR", &BACKGROUND_IMAGE_COLOR, ColorA(1, 1, 1, 1), SimpleGUI::RGB);
    mGui->addParam("FACE_BORDER_SCALE", &FACE_BORDER_SCALE, 0, 3, 0.7);
    mGui->addParam("FACE_FADE_BORDER_SCALE", &FACE_FADE_BORDER_SCALE, 1, 2, 1.4);
            
    /* clean up controller window */
    mGui->getControlByName("WINDOW_WIDTH")->active=false;
    mGui->getControlByName("WINDOW_HEIGHT")->active=false;
    mGui->getControlByName("CAMERA_WIDTH")->active=false;
    mGui->getControlByName("CAMERA_HEIGHT")->active=false;
    mGui->getControlByName("DETECTION_WIDTH")->active=false;
    mGui->getControlByName("DETECTION_HEIGHT")->active=false;
    mGui->getControlByName("FULLSCREEN")->active=false;
    mGui->getControlByName("TRACKING")->active=false;
    mGui->getControlByName("BACKGROUND_IMAGE_COLOR")->active=false;
    mGui->getControlByName("BACKGROUND_IMAGE_COLOR")->active=false;
    
    mGui->load(getResourcePath(RES_SETTINGS));
    mGui->setEnabled(false);
    
    setWindowSize( WINDOW_WIDTH, WINDOW_HEIGHT );
    setFullScreen( FULLSCREEN );
    if (FULLSCREEN) {
        hideCursor();
//        switch_resolution (WINDOW_WIDTH, WINDOW_HEIGHT, 60.0);
    }
    
    mFont = Font(loadResource("pf_tempesta_seven.ttf"), 8);

    /* setting up capture device */
    mCameraTexture = gl::Texture(CAMERA_WIDTH, CAMERA_HEIGHT);
    switch (TRACKING) {
        case 0:
            mFaceDetection = new FeatureDetectionCinder();
            break;
#ifdef COMPILE_CAPTURE_FIREFLY
        case 1:
            mFaceDetection = new FeatureDetectionFireFly();
            break;
#endif
#ifdef COMPILE_CAPTURE_OPENCV
        case 2:
            mFaceDetection = new FeatureDetectionOpenCV();
            break;
#endif
        default:
            console() << "### choosing default tracking method." << endl;
            mFaceDetection = new FeatureDetectionCinder();
            break;
    }

    mGui->addParam("DETECT_FLAGS",&(mFaceDetection->DETECT_FLAGS), CV_HAAR_DO_CANNY_PRUNING, CV_HAAR_DO_ROUGH_SEARCH, CV_HAAR_DO_CANNY_PRUNING);
    mGui->addParam("DETECT_SCALE_FACTOR",&(mFaceDetection->DETECT_SCALE_FACTOR), 1.1, 5, 1.2);
    mGui->addParam("DETECT_MIN_NEIGHBORS",&(mFaceDetection->DETECT_MIN_NEIGHBORS), 1, 20, 2);

    mGui->addParam("CAMERA_EXPOSURE", &(mFaceDetection->CAMERA_EXPOSURE), 0, 255, 20);
    mGui->addParam("CAMERA_SHUTTER", &(mFaceDetection->CAMERA_SHUTTER), 0, 255, 200);
    mGui->addParam("CAMERA_BRIGHTNESS", &(mFaceDetection->CAMERA_BRIGHTNESS), 0, 255, 166);
    mGui->addParam("CAMERA_GAIN", &(mFaceDetection->CAMERA_GAIN), 0, 255, 17);

    mGui->load(getResourcePath(RES_SETTINGS)); // HACK this is quite stupid, but we have a catch 22 here ...
    mFaceDetection->setup(CAMERA_WIDTH, 
                          CAMERA_HEIGHT, 
                          DETECTION_WIDTH, 
                          DETECTION_HEIGHT,
                          0);
    mGui->dump();
    
    mGui->addSeparator();
    mFaceOut = mGui->addLabel("");
    mFPSOut = mGui->addLabel("");
}

void GesichtertauschApp::heartbeat(double mDeltaTime) {
    mWatchdogCounter += mDeltaTime;
    if (mWatchdogCounter > mWatchdogInterval) {
        mWatchdogCounter = 0.0;
        console() << "+++ heartbeat" << std::endl;
        osc::Message mMessage;
        mMessage.addIntArg(getpid());
        mMessage.setAddress("/watchdog/register");
        mMessage.setRemoteEndpoint(mWatchdogHost, mWatchdogPort);
        mWatchdogSender.sendMessage(mMessage);
    }    
}

void GesichtertauschApp::update() {
    double mDeltaTime = getElapsedSeconds() - mTime;
    mTime = getElapsedSeconds();
    
    /* heartbeat */
    heartbeat(mDeltaTime);

    {
        stringstream mStr;
        mStr << "FPS: " << getAverageFps();
        mFPSOut->setText(mStr.str());
    }
    setFrameRate( FRAME_RATE );
        

    /* get new faces */
    vector<Rectf> mFaces;
    mFaceDetection->update(mCameraTexture, mFaces);

    {
        stringstream mStr;
        mStr << "FACES: " << mEntities.size();
        mFaceOut->setText(mStr.str());   
    }

    /* rescale from capture to window width */
    Vec2f mScale = Vec2f(float(CAMERA_WIDTH) / float(DETECTION_WIDTH), 
                         float(CAMERA_HEIGHT) / float(DETECTION_HEIGHT));
    mScale.x *= float(WINDOW_WIDTH) / float(CAMERA_WIDTH);
    mScale.y *= float(WINDOW_HEIGHT) / float (CAMERA_HEIGHT);
    for( vector<Rectf>::iterator faceIter = mFaces.begin(); faceIter != mFaces.end(); ++faceIter ) {
        Rectf * mFace = &(*faceIter);
        mFace->x1 *= mScale.x;
        mFace->x2 *= mScale.x;
        mFace->y1 *= mScale.y;
        mFace->y2 *= mScale.y;
    }
    
    /* set all existing entities to dead and invisble*/
    for( vector<FaceEntity>::iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter) {
        FaceEntity* mEntity = &(*mIter);
        mEntity->alive = false;
    }

    /* map new faces to existing entities */
    for( vector<FaceEntity>::iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter) {
        FaceEntity* mEntity = &(*mIter);
        /* test against all new faces */
        for( vector<Rectf>::iterator faceIter = mFaces.begin(); faceIter != mFaces.end(); ) {
            Rectf mFace = *faceIter;
            float mDistance = mFace.getCenter().distance(mEntity->border.getCenter());
            if (mDistance < MIN_TRACKING_DISTANCE) {
//                console() << "### found ID again. (" << mEntity->ID << ")" << mEntity->lifetime << endl;
                // TODO maybe interpolate in a more stable way …
                Vec2f mAveragedP1 = Vec2f(mFace.x1, mFace.y1);
                Vec2f mAveragedP2 = Vec2f(mFace.x2, mFace.y2);
                mAveragedP1 += Vec2f(mEntity->border.x1, mEntity->border.y1);
                mAveragedP2 += Vec2f(mEntity->border.x2, mEntity->border.y2);
                mAveragedP1 *= 0.5;
                mAveragedP2 *= 0.5;
                mEntity->border.set(mAveragedP1.x, mAveragedP1.y,
                                    mAveragedP2.x, mAveragedP2.y);
                mEntity->alive = true;
                mEntity->idletime = 0.0;
                mEntity->lifetime += mDeltaTime;
                mFaces.erase(faceIter);
                break;
            } else {
                ++faceIter;
            }
        }
    }
    
    /* create new entities from left-over faces */
    for( vector<Rectf>::iterator faceIter = mFaces.begin(); faceIter != mFaces.end(); ++faceIter) {
        FaceEntity mNewEntity = FaceEntity();
        mNewEntity.ID = mSerialID++;
//        console() << "### found new face. ID (" << mNewEntity.ID << ")" << endl;
        Rectf mFace = *faceIter;
        mNewEntity.border = mFace;
        mNewEntity.lifetime = 0.0;
        mNewEntity.idletime = 0.0;
        mNewEntity.visible = false;
        mNewEntity.alive = true;
        mEntities.push_back(mNewEntity);
    }
    mFaces.clear();

    /* grant dead entities some extra time */
    for( vector<FaceEntity>::iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter) {
        FaceEntity* mEntity = &(*mIter);
        if (!mEntity->alive) {
            /* if an entity disappeard we ll grant it some seconds before it gets removed */
            if (mEntity->idletime < TIME_BEFOR_IDLE_DEATH) {
                /* start immediately no matter how long entity already lived */
//                if (mEntity->idletime == 0.0) {
//                    console() << "### started dying (" << mEntity->ID << ")" << endl;
//                }
                mEntity->idletime += mDeltaTime;
                mEntity->lifetime += mDeltaTime; /* do we want to count abscence as lifetime? */
                mEntity->alive = true;
            }
        }
    }    
    
    /* erase all dead entities */
    for( vector<FaceEntity>::iterator mIter = mEntities.begin(); mIter != mEntities.end();) {
        FaceEntity mEntity = *mIter;
        if (mEntity.alive) {
            ++mIter;
        } else {
            mEntities.erase(mIter);
        }
    }

    /* set all 'stable' entities to visible */
    for( vector<FaceEntity>::iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter) {
        FaceEntity * mEntity = &(*mIter);
        if (mEntity->lifetime > MIN_LIFETIME_TO_VIEW) {
            mEntity->visible = true;
        } else {
            mEntity->visible = false;
        }
    }

    /* update slice */
    for( vector<FaceEntity>::iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter) {
        FaceEntity * mEntity = &(*mIter);
        mEntity->slice.set(mEntity->border.x1, mEntity->border.y1, mEntity->border.x2, mEntity->border.y2);
    }
}

void GesichtertauschApp::draw() {
    
    glClearColor( 0, 0, 0, 1.0 );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    
    if ( ! mCameraTexture ) {
        return;
    }
        
//    gl::setMatricesWindow( getWindowSize() ),
    gl::setMatricesWindow(WINDOW_WIDTH, WINDOW_HEIGHT);
    gl::enableAlphaBlending();
    glScalef(-1.0, 1.0, 1.0);
    glTranslatef(-WINDOW_WIDTH, 0, 0);
    
    /* shader */
    // TODO make this more opt'd
    if (ENABLE_SHADER) {
        mShader.bind();
        const int STEPS = 32;
        float mThresholds[STEPS];// = {0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
        for (int i=0; i < STEPS; ++i) {
            mThresholds[i] = float(i) / float(STEPS - 1);
        }
        mShader.uniform("thresholds", mThresholds, STEPS);   
        mShader.uniform( "tex0", 0 );
    }

    /* draw the webcam image */
    gl::color( BACKGROUND_IMAGE_COLOR );
    gl::draw( mCameraTexture, Rectf(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT) );
    mCameraTexture.disable();
    
    /* normalize texture coordinates */
    Vec2f mNormalizeScale = Vec2f(1.0 / float(WINDOW_WIDTH), 1.0 / float(WINDOW_HEIGHT));
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glScalef(mNormalizeScale.x, mNormalizeScale.y, 1.0);
    glMatrixMode(GL_MODELVIEW);
    
    /* draw orgiginal faces */
    if (mEntities.size() < 2) {
        gl::enableAlphaBlending();
        mCameraTexture.enableAndBind();
        for( vector<FaceEntity>::const_iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter ) {
            drawEntity(*mIter, FACE_COLOR_UNO);
        }    
        mCameraTexture.disable();
        gl::disableAlphaBlending();
    }
    
    /* HACK // swap faces */
    mCameraTexture.enableAndBind();
    if (mEntities.size() >= 2) {
        const FaceEntity A = mEntities[0];
        const FaceEntity B = mEntities[1];
        if (A.visible && B.visible) {
            FaceEntity mEntityA = FaceEntity();
            FaceEntity mEntityB = FaceEntity();
            
            mEntityA.border = B.border;
            mEntityB.border = A.border;
            mEntityA.slice = A.slice;
            mEntityB.slice = B.slice;
            mEntityA.visible = A.visible;
            mEntityB.visible = B.visible;
            mEntityA.ID = A.ID;
            mEntityB.ID = B.ID;
            
            drawEntity(mEntityA, FACE_COLOR_DUO);
            drawEntity(mEntityB, FACE_COLOR_DUO);
        }
    }

    /* restore texture coordinates */
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    mCameraTexture.disable();

    /* shader */
    if (ENABLE_SHADER) {
        mShader.unbind();
    }

    /* mask */
    float MASK_LEFT_TOP = 1;
    float MASK_LEFT_BOTTOM = 1;
    float MASK_RIGHT_TOP = 1;
    float MASK_RIGHT_BOTTOM = 1;
    
    gl::color(0, 0, 0, 1);

    Path2d mPathLeft;
    mPathLeft.moveTo(0, 0);
    mPathLeft.lineTo(MASK_LEFT_TOP, 0);
    mPathLeft.lineTo(MASK_LEFT_BOTTOM, WINDOW_HEIGHT);
    mPathLeft.lineTo(0, WINDOW_HEIGHT);
    mPathLeft.close();    
    gl::drawSolid(mPathLeft);
    
    Path2d mPathRight;
    mPathRight.moveTo(WINDOW_WIDTH, 0);
    mPathRight.lineTo(WINDOW_WIDTH-MASK_RIGHT_TOP, 0);
    mPathRight.lineTo(WINDOW_WIDTH-MASK_RIGHT_BOTTOM, WINDOW_HEIGHT);
    mPathRight.lineTo(WINDOW_WIDTH, WINDOW_HEIGHT);
    mPathRight.close();    
    gl::drawSolid(mPathRight);

    /* draw entity IDs */
    const bool DRAW_ENTITY_ID = false;
    if (DRAW_ENTITY_ID) {
        for( vector<FaceEntity>::const_iterator mIter = mEntities.begin(); mIter != mEntities.end(); ++mIter ) {
            const FaceEntity mEntity = *mIter;
            std::stringstream mStr;
            mStr << mEntity.ID;
            gl::drawStringCentered(mStr.str(), mEntity.border.getCenter(), Color(1, 0, 0), mFont);
        }    
    }
        
    /* gooey */
    mGui->draw();
}

void GesichtertauschApp::drawEntity(const FaceEntity& mEntity, const ColorA& pColor) {
    /* only display entities with a certain lifetime */
    if (mEntity.visible) {
        if (!ENABLE_SHADER) {
            gl::color( pColor );
            Vec2f mLower = Vec2f(mEntity.slice.x1, mEntity.slice.y1);
            Vec2f mUpper = Vec2f(mEntity.slice.x2, mEntity.slice.y2);
            glBegin(GL_POLYGON);
            glTexCoord2f(mLower.x, mLower.y);
            glVertex2f(mEntity.border.x1, mEntity.border.y1);
            glTexCoord2f(mUpper.x, mLower.y);
            glVertex2f(mEntity.border.x2, mEntity.border.y1);
            glTexCoord2f(mUpper.x, mUpper.y);
            glVertex2f(mEntity.border.x2, mEntity.border.y2);
            glTexCoord2f(mLower.x, mUpper.y);
            glVertex2f(mEntity.border.x1, mEntity.border.y2);
            glEnd();
        } else {
            Vec2f mCenter = mEntity.border.getCenter();
            float mRadius = mEntity.border.getWidth() / 2.0; 

            Vec2f mTexCenter = mEntity.slice.getCenter();
            float mTexRadius = mEntity.slice.getWidth() / 2.0;

            gl::color( pColor );
            glBegin(GL_TRIANGLE_FAN);
            glTexCoord2f(mTexCenter.x, mTexCenter.y);
            glVertex2f(mCenter.x, mCenter.y);
            for (float r=0; r<M_PI * 2.0;r += (M_PI * 2.0) / 36.0) {
                Vec2f mTexVertex = Vec2f(sin(r), cos(r)); 
                mTexVertex *= mTexRadius * FACE_BORDER_SCALE;
                glTexCoord2f(mTexVertex.x + mTexCenter.x, mTexVertex.y + mTexCenter.y);
                
                Vec2f mVertex = Vec2f(sin(r), cos(r)); 
                mVertex *= mRadius * FACE_BORDER_SCALE;
                glVertex2f(mVertex.x + mCenter.x, mVertex.y + mCenter.y);
            }
            glEnd();

//            glBegin(GL_POLYGON);
//            for (float r=0; r<M_PI * 2.0;r += (M_PI * 2.0) / 36.0) {
//                Vec2f mTexVertex = Vec2f(sin(r), cos(r)); 
//                mTexVertex *= mTexRadius;
//                glTexCoord2f(mTexVertex.x + mTexCenter.x, mTexVertex.y + mTexCenter.y);
//                
//                Vec2f mVertex = Vec2f(sin(r), cos(r)); 
//                mVertex *= mRadius;
//                glVertex2f(mVertex.x + mCenter.x, mVertex.y + mCenter.y);
//            }
//            glEnd();
            
            glBegin(GL_QUAD_STRIP);
            for (float r=0; r<M_PI * 2.0;r += (M_PI * 2.0) / 36.0) {
                Vec2f mTexVertex = Vec2f(sin(r), cos(r)); 
                mTexVertex *= mTexRadius * FACE_BORDER_SCALE;
                Vec2f mVertex = Vec2f(sin(r), cos(r)); 
                mVertex *= mRadius * FACE_BORDER_SCALE;
                
                glColor4f( pColor.r, pColor.g, pColor.b, pColor.a ); 
                glTexCoord2f(mTexVertex.x + mTexCenter.x, mTexVertex.y + mTexCenter.y);
                glVertex2f(mVertex.x + mCenter.x, mVertex.y + mCenter.y);

                /* extra vertices */
                glColor4f( pColor.r, pColor.g, pColor.b, 0.0 ); 
                glTexCoord2f(mTexVertex.x * FACE_FADE_BORDER_SCALE + mTexCenter.x, 
                             mTexVertex.y * FACE_FADE_BORDER_SCALE + mTexCenter.y);
                glVertex2f(mVertex.x * FACE_FADE_BORDER_SCALE + mCenter.x, 
                           mVertex.y * FACE_FADE_BORDER_SCALE + mCenter.y);
            }
            glEnd();
        }
    }
}

void GesichtertauschApp::mouseDown( MouseEvent event ) {
}

void GesichtertauschApp::keyDown( KeyEvent pEvent ) {
    switch(pEvent.getChar()) {				
        case 'd': mGui->dump(); break;
        case 'l': mGui->load(getResourcePath(RES_SETTINGS)); break;
        case 's': mGui->save(getResourcePath(RES_SETTINGS)); break;
//        case 'm':
//            CGDirectDisplayID mDisplayID = getSettings().getDisplay()->getCGDirectDisplayID();
//            CGDisplayConfigRef pConfigRef;
//            CGBeginDisplayConfiguration (&pConfigRef);
//            CGConfigureDisplayOrigin (pConfigRef,
//                                      mDisplayID,
//                                      -100, 0);
//            CGCompleteDisplayConfiguration (pConfigRef, kCGConfigureForAppOnly);
//            break;
    }
    switch(pEvent.getCode()) {
        case KeyEvent::KEY_ESCAPE:  setFullScreen( false ); quit(); break;
        case KeyEvent::KEY_SPACE: mGui->setEnabled(!mGui->isEnabled());break;
    }
}

int switch_resolution (int pWidth, int pHeight, double pRefreshRate) {
    //	int	h; 							// horizontal resolution
    //	int v; 							// vertical resolution
	CFDictionaryRef switchMode; 	// mode to switch to
	CGDirectDisplayID mainDisplay;  // ID of main display
    
	CFDictionaryRef CGDisplayCurrentMode(CGDirectDisplayID display);
    
    //	if (argc == 1) {
	    CGRect screenFrame = CGDisplayBounds(kCGDirectMainDisplay);
		CGSize screenSize  = screenFrame.size;
		printf("%f %f\n", screenSize.width, screenSize.height);
    //		return 0;
    //	}
    //	if (argc != 3 || !(h = atoi(argv[1])) || !(v = atoi(argv[2])) ) {
    //		fprintf(stderr, "ERROR: Use %s horres vertres\n", argv[0]);
    //		return -1;
    //	}
    
	mainDisplay = CGMainDisplayID();
    //	switchMode = CGDisplayBestModeForParameters(mainDisplay, 32, pWidth, pHeight, NULL);
	switchMode = CGDisplayBestModeForParametersAndRefreshRate(mainDisplay, 32, pWidth, pHeight, pRefreshRate, NULL);
    
	if (! MyDisplaySwitchToMode(mainDisplay, switchMode)) {
	    fprintf(stderr, "Error changing resolution to %d %d\n", pWidth, pHeight);
		return 1;
	}
    
	return 0;
}

bool MyDisplaySwitchToMode (CGDirectDisplayID display, CFDictionaryRef mode)
{
	CGDisplayConfigRef config;
	if (CGBeginDisplayConfiguration(&config) == kCGErrorSuccess) {
		CGConfigureDisplayMode(config, display, mode);
		CGCompleteDisplayConfiguration(config, kCGConfigureForSession );
		return true;
	}
	return false;
}

CINDER_APP_BASIC( GesichtertauschApp, RendererGl(0) )
