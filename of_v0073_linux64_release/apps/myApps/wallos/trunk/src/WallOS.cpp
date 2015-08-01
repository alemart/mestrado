#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <algorithm>
#include "WallOS.h"
#include "WallApp.h"
#include "apps/Canvas.h"

WallOS::WallOS(int tuioPort) : _tuioPort(tuioPort)
{
    std::cout << "Initializing WallOS..." << std::endl;
    std::cout << "[TUIO] connecting to port " << _tuioPort << "..." << std::endl;
    _tuioClient = new TUIO::TuioClient(_tuioPort);
    _tuioClient->connect(false);
    std::cout << "[TUIO] " << (_tuioClient->isConnected() ? "connected!" : "error") << std::endl;
    _currentApp = new Canvas(this);
}

WallOS::~WallOS()
{
    delete _currentApp;
    std::cout << "[TUIO] disconnecting..." << std::endl;
    _tuioClient->disconnect();
    delete _tuioClient;
}

void WallOS::setup()
{
    ofBackgroundHex(0x000000);
    _currentApp->setup();
}

void WallOS::update()
{
    if(_tuioClient->isConnected() && !_sweetspot.isBeingManipulated()) {
        _tuioObjects = _tuioClient->getTuioObjects();
        _tuioClient->lockObjectList();
        _currentApp->updateTouchPoints(_tuioObjects);
        _currentApp->update();
        //_tuioClient->unlockObjectList();
    }
}

void WallOS::draw()
{
    ofEnableAlphaBlending();

    //_tuioClient->lockObjectList();
    //_currentApp->updateTouchPoints(_tuioObjects);
    _currentApp->draw();
    _tuioClient->unlockObjectList();

    _sweetspot.draw();
    ofDisableAlphaBlending();
}

void WallOS::keyPressed(int key)
{
    _sweetspot.keyPressed(key);
}

void WallOS::keyReleased(int key)
{
}

void WallOS::mouseMoved(int x, int y)
{
}

void WallOS::mouseDragged(int x, int y, int button)
{
}

void WallOS::mousePressed(int x, int y, int button)
{
}

void WallOS::mouseReleased(int x, int y, int button)
{
}

void WallOS::windowResized(int w, int h)
{
}

void WallOS::gotMessage(ofMessage msg)
{
}

void WallOS::dragEvent(ofDragInfo dragInfo)
{ 
}

ofVec2f WallOS::norm2win(ofVec2f pointInNormalizedCoordinates)
{
    return _sweetspot.convertToWindowCoordinates(pointInNormalizedCoordinates);
}






// ====================================
//
//            SWEET SPOT
//
// ====================================

const char* WallOS::SweetSpot::_filepath = "data/sweetspot.txt";

WallOS::SweetSpot::SweetSpot() : _activeCorner(4), _sweetspot2screen(0)
{
    _corner[0] = ofPoint(0, 0); // topleft
    _corner[1] = ofPoint(1, 0); // topright
    _corner[2] = ofPoint(1, 1); // bottomright
    _corner[3] = ofPoint(0, 1); // bottomleft
    _fnt.loadFont("DejaVuSans.ttf", 36);
    _loadFromDisk();
    _updateHomography();
}

WallOS::SweetSpot::~SweetSpot()
{
    _saveToDisk();
    if(_sweetspot2screen)
        cvReleaseMat(&_sweetspot2screen);
}

void WallOS::SweetSpot::draw()
{
    int w = ofGetWidth(), h = ofGetHeight();

    ofSetLineWidth(0.0f);
    ofSetColor(0, 0, 0, 128);
    ofFill();
    ofBeginShape();
        ofVertex(0, 0);
        ofVertex(_corner[0].x * w, _corner[0].y * h);
        ofVertex(_corner[1].x * w, _corner[1].y * h);
        ofVertex(w, 0);
    ofEndShape();
    ofBeginShape();
        ofVertex(w, 0);
        ofVertex(_corner[1].x * w, _corner[1].y * h);
        ofVertex(_corner[2].x * w, _corner[2].y * h);
        ofVertex(w, h);
    ofEndShape();
    ofBeginShape();
        ofVertex(w, h);
        ofVertex(_corner[2].x * w, _corner[2].y * h);
        ofVertex(_corner[3].x * w, _corner[3].y * h);
        ofVertex(0, h);
    ofEndShape();
    ofBeginShape();
        ofVertex(0, h);
        ofVertex(_corner[3].x * w, _corner[3].y * h);
        ofVertex(_corner[0].x * w, _corner[0].y * h);
        ofVertex(0, 0);
    ofEndShape();

    ofSetLineWidth(4.0f);
    ofSetColor(255, 0, 255, 128);
    for(int i=0; i<4; i++)
        ofLine(_corner[i].x * w, _corner[i].y * h, _corner[(i+1)%4].x * w, _corner[(i+1)%4].y * h);

    if(isBeingManipulated()) {
        int R = 40, r = 30;
        ofPath curve;
        ofPoint p(_corner[_activeCorner].x * w, _corner[_activeCorner].y * h);
        curve.arc(p, R, R, 0, 360);
        curve.arcNegative(p, r, r, 0, 360);
        curve.close();
        curve.setArcResolution(60);
        curve.setFillColor(ofColor(255, 128, 0, 128));
        curve.setFilled(true);
        curve.draw();

        ofPoint q(p.x-15, p.y+15);
        std::stringstream ss;
        ss << (_activeCorner + 1);
        ofSetHexColor(0x000000);
        _fnt.drawString(ss.str(), q.x+1, q.y+1);
        _fnt.drawString("Por favor, ajuste o sweet-spot.", 20+1, 50+1);
        ofSetHexColor(0xFFFFFF);
        _fnt.drawString(ss.str(), q.x, q.y);
        _fnt.drawString("Por favor, ajuste o sweet-spot.", 20, 50);
    }
}

void WallOS::SweetSpot::keyPressed(int key)
{
    if(key == ' ')
        _activeCorner = (_activeCorner + 1) % 5;
    else if(key == OF_KEY_BACKSPACE)
        _activeCorner = (((_activeCorner - 1) % 5) + 5) % 5;
    else if(_activeCorner < 4) {
        ofPoint& c = _corner[_activeCorner];
        const float speed = 2.0f;
        const float dt = ofGetLastFrameTime();

        // translate active corner
        if(key == OF_KEY_LEFT)
            c += ofPoint(-1,0) * speed * dt;
        else if(key == OF_KEY_UP)
            c += ofPoint(0,-1) * speed * dt;
        else if(key == OF_KEY_RIGHT)
            c += ofPoint(1,0) * speed * dt;
        else if(key == OF_KEY_DOWN)
            c += ofPoint(0,1) * speed * dt;

        // align corners
        if(_activeCorner == 0) {
            _corner[3].x = c.x;
            _corner[1].y = c.y;
        }
        else if(_activeCorner == 1) {
            _corner[2].x = c.x;
            _corner[0].y = c.y;
        }
        else if(_activeCorner == 2) {
            _corner[1].x = c.x;
            _corner[3].y = c.y;
        }
        else if(_activeCorner == 3) {
            _corner[0].x = c.x;
            _corner[2].y = c.y;
        }

        // update homography matrix
        _updateHomography();
    }
}

ofVec2f WallOS::SweetSpot::convertToWindowCoordinates(ofVec2f pointInSweetSpotCoordinates)
{
    ofVec2f p;
    CvMat *src = cvCreateMat(3, 1, CV_32FC1);
    CvMat *dst = cvCreateMat(3, 1, CV_32FC1);

    CV_MAT_ELEM(*src, float, 0, 0) = pointInSweetSpotCoordinates.x;
    CV_MAT_ELEM(*src, float, 1, 0) = pointInSweetSpotCoordinates.y;
    CV_MAT_ELEM(*src, float, 2, 0) = 1.0f;

    cvGEMM(_sweetspot2screen, src, 1.0f, NULL, 0.0f, dst, 0);

    p.x = CV_MAT_ELEM(*dst, float, 0, 0) / CV_MAT_ELEM(*dst, float, 2, 0);
    p.y = CV_MAT_ELEM(*dst, float, 1, 0) / CV_MAT_ELEM(*dst, float, 2, 0);

    cvReleaseMat(&src);
    cvReleaseMat(&dst);
    return p;
}

bool WallOS::SweetSpot::isBeingManipulated()
{
    return _activeCorner < 4;
}

void WallOS::SweetSpot::_loadFromDisk()
{
    std::ifstream f(_filepath);
    if(f.is_open()) {
        for(int i=0; i<4; i++)
            f >> _corner[i].x >> _corner[i].y;
    }
}

void WallOS::SweetSpot::_saveToDisk()
{
    std::ofstream f(_filepath);
    if(f.is_open()) {
        for(int i=0; i<4; i++)
            f << _corner[i].x << " " << _corner[i].y << "\n";
    }
}

void WallOS::SweetSpot::_updateHomography()
{
    // initializing
    CvMat* src = cvCreateMat(4, 2, CV_32FC1);
    CvMat* dst = cvCreateMat(4, 2, CV_32FC1);
    CvMat* h1 = cvCreateMat(3, 3, CV_32FC1);
    CvMat* h2 = cvCreateMat(3, 3, CV_32FC1);
    CvMat* homography = cvCreateMat(3, 3, CV_32FC1);

    //
    // First homography: [0,1]^2 to _corner[]'s
    //

    // setup data
    CV_MAT_ELEM(*src, float, 0, 0) = 0.0f; // topleft
    CV_MAT_ELEM(*src, float, 0, 1) = 0.0f;
    CV_MAT_ELEM(*src, float, 1, 0) = 1.0f; // topright
    CV_MAT_ELEM(*src, float, 1, 1) = 0.0f;
    CV_MAT_ELEM(*src, float, 2, 0) = 1.0f; // bottomright
    CV_MAT_ELEM(*src, float, 2, 1) = 1.0f;
    CV_MAT_ELEM(*src, float, 3, 0) = 0.0f; // bottomleft
    CV_MAT_ELEM(*src, float, 3, 1) = 1.0f;

    CV_MAT_ELEM(*dst, float, 0, 0) = _corner[0].x; // topleft
    CV_MAT_ELEM(*dst, float, 0, 1) = _corner[0].y;
    CV_MAT_ELEM(*dst, float, 1, 0) = _corner[1].x; // topright
    CV_MAT_ELEM(*dst, float, 1, 1) = _corner[1].y;
    CV_MAT_ELEM(*dst, float, 2, 0) = _corner[2].x; // bottomright
    CV_MAT_ELEM(*dst, float, 2, 1) = _corner[2].y;
    CV_MAT_ELEM(*dst, float, 3, 0) = _corner[3].x; // bottomleft
    CV_MAT_ELEM(*dst, float, 3, 1) = _corner[3].y;

    // compute homography
    cvFindHomography(src, dst, h1);

    //
    // Second homography: _corner[]'s to window-space
    //

    // setup data
    CV_MAT_ELEM(*src, float, 0, 0) = _corner[0].x; // topleft
    CV_MAT_ELEM(*src, float, 0, 1) = _corner[0].y;
    CV_MAT_ELEM(*src, float, 1, 0) = _corner[1].x; // topright
    CV_MAT_ELEM(*src, float, 1, 1) = _corner[1].y;
    CV_MAT_ELEM(*src, float, 2, 0) = _corner[2].x; // bottomright
    CV_MAT_ELEM(*src, float, 2, 1) = _corner[2].y;
    CV_MAT_ELEM(*src, float, 3, 0) = _corner[3].x; // bottomleft
    CV_MAT_ELEM(*src, float, 3, 1) = _corner[3].y;

    CV_MAT_ELEM(*dst, float, 0, 0) = _corner[0].x * ofGetWidth(); // topleft
    CV_MAT_ELEM(*dst, float, 0, 1) = _corner[0].y * ofGetHeight();
    CV_MAT_ELEM(*dst, float, 1, 0) = _corner[1].x * ofGetWidth(); // topright
    CV_MAT_ELEM(*dst, float, 1, 1) = _corner[1].y * ofGetHeight();
    CV_MAT_ELEM(*dst, float, 2, 0) = _corner[2].x * ofGetWidth(); // bottomright
    CV_MAT_ELEM(*dst, float, 2, 1) = _corner[2].y * ofGetHeight();
    CV_MAT_ELEM(*dst, float, 3, 0) = _corner[3].x * ofGetWidth(); // bottomleft
    CV_MAT_ELEM(*dst, float, 3, 1) = _corner[3].y * ofGetHeight();

    // compute homography
    cvFindHomography(src, dst, h2);

    //
    // Finally: [0,1]^2 to window-space
    //
    cvMatMul(h2, h1, homography);

    // release stuff
    if(_sweetspot2screen)
        cvReleaseMat(&_sweetspot2screen);
    cvReleaseMat(&src);
    cvReleaseMat(&dst);
    cvReleaseMat(&h1);
    cvReleaseMat(&h2);

    // done!
    _sweetspot2screen = homography;
}
