#include <vector>
#include "Paint.h"
#include "TouchDetection.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/BackgroundModel.h"
#include "../core/TouchDetector.h"
#include "../core/TouchTracker.h"


PaintScene::PaintScene(MyApplication* app) : Scene(app), _clear(true)
{
    ofSetBackgroundColor(0, 0, 0);
    ofSetBackgroundAuto(false);
    _touchDetector = new TouchDetector(app->kinect());
    _touchTracker = new TouchTracker(60, 6);
    _startTime = ofGetElapsedTimef();
}

PaintScene::~PaintScene()
{
    ofSetBackgroundAuto(true);
    delete _touchTracker;
    delete _touchDetector;
}

void PaintScene::update()
{
    _touchDetector->update();
    _touchTracker->feed( _touchDetector->touchPoints(), 25, ofGetLastFrameTime() ); // FIXME: area of BackgroundModel
}

void PaintScene::draw()
{
    if(_clear || ofGetElapsedTimef() < _startTime + 1.0f) {
        ofDisableAlphaBlending();
        ofSetColor(0, 0, 0);
        ofRect(0, 0, ofGetWidth(), ofGetHeight());
        _clear = false;
    }

    //ofEnableAlphaBlending();
    //ofSetColor(255, 255, 255, 20);
    //ofRect(0, 0, ofGetWidth(), ofGetHeight());
    //application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());

    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end(); ++it) {
        const TouchDetector::TouchPoint* p0 = (*it)(0);
        const TouchDetector::TouchPoint* p1 = (*it)(-1);
std::cout << "id: " << p0->_id << std::endl;

        float x0 = p0->surfaceX * ofGetWidth();
        float y0 = p0->surfaceY * ofGetHeight();
        float x1 = p1->surfaceX * ofGetWidth();
        float y1 = p1->surfaceY * ofGetHeight();
        std::string type = p0->type;
        if(type.substr(0, 9) == "projetor_")
            type = type.substr(9);

        if(type == "marker_violet")
            ofSetHexColor(0x330099);
        else if(type == "marker_blue")
            ofSetHexColor(0x2266FF);
        else if(type == "marker_green" || type == "eraser_green")
            ofSetHexColor(0x33FF33);
        else if(type == "marker_yellow")
            ofSetHexColor(0xFFFF77);
        else if(type == "marker_red")
            ofSetHexColor(0xFF3300);
        else if(type == "marker_magenta")
            ofSetHexColor(0xFF0099);
        else
            ofSetHexColor(0xFFFFFF);

        ofSetLineWidth(5.0f);
        ofLine(x1, y1, x0, y0);
    }

    ofSetHexColor(0xFFFFFF);
    ofRect(ofGetWindowWidth()*5/6 - 1, ofGetWindowHeight()*5/6 - 1, ofGetWindowWidth()*5/6 + 2, ofGetWindowHeight()*5/6 + 2);
    _touchDetector->blobs()->draw(ofGetWindowWidth()*5/6, ofGetWindowHeight()*5/6, ofGetWindowWidth()/6, ofGetWindowHeight()/6);
}

void PaintScene::keyPressed(int key)
{
    if(key == ' ' || key == OF_KEY_RETURN) {
        _touchDetector->save();
        application()->setScene(
            new TouchDetectionScene(application())
        );
    }
    else if(key == OF_KEY_DEL || key == 'c')
        _clear = true;
}
