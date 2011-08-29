/*
 * Gesichtertausch
 *
 * Copyright (C) 2011 The Product GbR Kochlik + Paul
 *
 */


#include <iostream>

#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/Texture.h"
#include "cinder/Capture.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class FeatureDetection {
public:
    virtual void setup(int pCaptureWidth, int pCaptureHeight) = 0;
    virtual void updateTexture(const gl::Texture&) = 0;
    virtual void detect(vector<Rectf>& pFaces, vector<Rectf>& pEyes) = 0;
};


class FeatureDetectionCinder : FeatureDetection {
public:
    void setup(int pCaptureWidth, int pCaptureHeight);
    void updateTexture(const gl::Texture&);
    void detect();

private:
    Capture			mCapture;

};

void FeatureDetectionCinder::setup(int pCaptureWidth, int pCaptureHeight) {
}

void FeatureDetectionCinder::updateTexture(const gl::Texture& pTexture) {
}

void FeatureDetectionCinder::detect() {
}
