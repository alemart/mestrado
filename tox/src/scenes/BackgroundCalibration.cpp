#include "ofMain.h"
#include "ofxOpenCv.h"
#include "BackgroundCalibration.h"
#include "../core/BackgroundModel.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/TouchDetector.h"
#include "TouchDetection.h"


BackgroundCalibrationScene::BackgroundCalibrationScene(MyApplication* app, int numberOfFramesToCollect) : Scene(app), _sampleBackground(false), _numberOfFramesToCollect(numberOfFramesToCollect), _showRGBImage(false)
{
    _fnt.loadFont("DejaVuSans.ttf", 24);
}

BackgroundCalibrationScene::~BackgroundCalibrationScene()
{
}

void BackgroundCalibrationScene::update()
{
    if(_sampleBackground) {

        // capturing frames
        if(_imgAccum.count() < _numberOfFramesToCollect) {
            _imgAccum.add(application()->kinect()->rawDepthImage());
            if(_imgAccum.count() == _numberOfFramesToCollect) {
                float maskWidth = _imgAccum.image(0)->width;
                float maskHeight = _imgAccum.image(0)->height;
                float screenWidth = ofGetWidth();
                float screenHeight = ofGetHeight();
                CvPoint p[4];

                for(int i=0; i<4; i++) {
                    p[i].x = maskWidth * (_point[i].x / screenWidth);
                    p[i].y = maskHeight * (_point[i].y / screenHeight);
                }

                BackgroundModel* m = BackgroundModel::create(application()->kinect(), _imgAccum, p[0], p[1], p[2], p[3]);
                m->save();

                // the calibration is over
                application()->setScene(
                    new TouchDetectionScene(application())
                );
            }
        }

    }
}

void BackgroundCalibrationScene::draw()
{
    // draw depth/rgb image
    if(_showRGBImage) {
        // FIXME: nao fiz, manualmente, a transf color2depth dos pontos selecionados pelo usuario. Por enquanto, utilizo distorcao do depth map do openni.
        ofSetHexColor(0xFFFFFF);
        application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    }
    else {
        ofSetHexColor(0x8888DD);
        application()->kinect()->grayscaleDepthImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    }

    // border
    ofSetHexColor(0xFF0000);
    ofSetLineWidth(2);
    ofLine(1, 1, ofGetWidth()-2, 1);
    ofLine(ofGetWidth()-2, 1, ofGetWidth()-2, ofGetHeight()-2);
    ofLine(ofGetWidth()-2, ofGetHeight()-2, 1, ofGetHeight()-2);
    ofLine(1, ofGetHeight()-2, 1, 1);

    // points
    ofSetHexColor(0xFFFF88);
    ofSetLineWidth(1);
    for(std::vector<ofPoint>::iterator it = _point.begin(); it != _point.end(); ++it)
        ofCircle(it->x, it->y, 5);
    for(int j=1; j<int(_point.size()); j++)
        ofLine(_point[j-1].x, _point[j-1].y, _point[j].x, _point[j].y);
    if(_point.size() == 4)
        ofLine(_point[3].x, _point[3].y, _point[0].x, _point[0].y);

    // title text
    if(_sampleBackground) {
        std::stringstream ss;
        ss << "Calibrando fundo... " << std::setprecision(3) << (100.0f * float(_imgAccum.count()) / float(_numberOfFramesToCollect)) << "%";

        ofSetHexColor(0x000000);
        _fnt.drawString(ss.str(), 10+1, 30+1);

        ofSetHexColor(0xDD8888);
        _fnt.drawString(ss.str(), 10, 30);
    }
    else if(!_showRGBImage) {
        ofSetHexColor(0x000000);
        _fnt.drawString("Delimite a superficie. Preciso de 4 pontos.", 10+1, 30+1);
        _fnt.drawString("topleft, topright, bottomright, bottomleft.", 10+1, 65+1);
        _fnt.drawString("S: skip calib; up-down-0: motor; space: swap img", 10+1, ofGetHeight()-15+1);

        ofSetHexColor(0xDD8888);
        _fnt.drawString("Delimite a superficie. Preciso de 4 pontos.", 10, 30);
        _fnt.drawString("topleft, topright, bottomright, bottomleft.", 10, 65);
        _fnt.drawString("S: skip calib; up-down-0: motor; space: swap img", 10+1, ofGetHeight()-15);
    }
}

void BackgroundCalibrationScene::mousePressed(int x, int y, int button)
{
    if(button == 0 && _point.size() < 4) { // left mouse btn
        _point.push_back( ofPoint(x, y) );
        if(_point.size() == 4)
            _sampleBackground = true;
    }
}

void BackgroundCalibrationScene::keyPressed(int key)
{
    if(key == ' ' || key == OF_KEY_RETURN)
        _showRGBImage = !_showRGBImage;
    else if(key == OF_KEY_UP)
        application()->kinect()->setTiltAngle(application()->kinect()->tiltAngle() + 10);
    else if(key == OF_KEY_DOWN)
        application()->kinect()->setTiltAngle(application()->kinect()->tiltAngle() - 10);
    else if(key == '0')
        application()->kinect()->setTiltAngle(0.0f);
    else if(key == 's')
        application()->setScene(new TouchDetectionScene(application()));
}







