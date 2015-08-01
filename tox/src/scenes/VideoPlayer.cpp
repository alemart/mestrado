#include "ofMain.h"
#include "ofxOpenCv.h"
#include "VideoPlayer.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"

VideoPlayerScene::VideoPlayerScene(MyApplication* app) : Scene(app)
{
    _cameraImage = app->kinect()->colorImage();
}

VideoPlayerScene::~VideoPlayerScene()
{
}

void VideoPlayerScene::update()
{
}

void VideoPlayerScene::draw()
{
    ofSetHexColor(0xFFFFFF);
    _cameraImage->draw(0, 0, ofGetWidth(), ofGetHeight());
}

void VideoPlayerScene::keyPressed(int key)
{
    if(key == ' ' || key == OF_KEY_RETURN) {
        if(_cameraImage == application()->kinect()->colorImage())
            _cameraImage = application()->kinect()->grayscaleDepthImage();
        else
            _cameraImage = application()->kinect()->colorImage();
    }
}
