#include <sstream>
#include "ofxOpenCv.h"
#include "VideoRecorder.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"

VideoRecorderScene::VideoRecorderScene(MyApplication* app, const std::string& sequenceName, int numberOfFramesToCollect) : Scene(app), _seqName(sequenceName), _numberOfFramesToCollect(numberOfFramesToCollect)
{
    _fnt.loadFont("DejaVuSans.ttf", 24);
}

VideoRecorderScene::~VideoRecorderScene()
{
}

void VideoRecorderScene::update()
{
    if(ofGetElapsedTimef() >= 5.0f) {
        if(_imgAccum[0].count() < _numberOfFramesToCollect) {
            _imgAccum[0].add( application()->kinect()->undistortedRawDepthImage() );
            _imgAccum[1].add( application()->kinect()->undistortedColorImage() );
        }
        else {
            _imgAccum[0].save(_seqName + "_d");
            _imgAccum[1].save(_seqName + "_rgb");
            std::exit(0);
        }
    }
}

void VideoRecorderScene::draw()
{
    std::stringstream ss;
    float percent = 100.0f * float(_imgAccum[0].count()) / float(_numberOfFramesToCollect);
    if(percent > 0.0f)
        ss << "Gravando... " << std::setprecision(3) << percent << "%";
    else
        ss << "Preparando...";

    ofSetHexColor(0xFFFFFF);
    application()->kinect()->grayscaleDepthImage()->draw(0, 0, ofGetWidth(), ofGetHeight()/2);
    application()->kinect()->colorImage()->draw(0, ofGetHeight()/2, ofGetWidth(), ofGetHeight()/2);

    ofSetHexColor(0xFF8888);
    _fnt.drawString(ss.str(), 10, 30);
}


