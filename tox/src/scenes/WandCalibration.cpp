#include <vector>
#include <fstream>
#include "WandCalibration.h"
#include "TUIO.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/TouchDetector.h"
#include "../core/BackgroundModel.h"
#include "../core/ImageLabeler.h"





WandCalibrationScene::WandCalibrationScene(MyApplication* app) : Scene(app), _first(true)
{
    ofSetBackgroundColor(0, 0, 0);
    _touchDetector = new TouchDetector(app->kinect(), TouchClassifier::QUALITY_FAST);
    _imageLabeler = new ImageLabeler(app->kinect()->colorImage()->width, app->kinect()->colorImage()->height);
    _wandTracker = new WandTracker(app->kinect(), _touchDetector->backgroundModel(), _imageLabeler, 20.0f, 200);
    _fnt.loadFont("DejaVuSans.ttf", 18);
    _fnt2.loadFont("DejaVuSans.ttf", 12);
}

WandCalibrationScene::~WandCalibrationScene()
{
    delete _wandTracker;
    delete _imageLabeler;
    delete _touchDetector;
}

void WandCalibrationScene::update()
{
    _touchDetector->update();
    _imageLabeler->update(_touchDetector->coloredImageWithoutBackground(), _touchDetector->theta());
    _wandTracker->update( ofGetLastFrameTime() );
    _trackedWands = _wandTracker->getWands();

    if(_first) {
        _first = false;
        _wandTracker->startCalibrationProcedure();
    }
}

void WandCalibrationScene::draw()
{
    // background
    //ofSetHexColor(0x8888DD);
    //application()->kinect()->grayscaleDepthImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    ofSetHexColor(0xFFFFAA);
    _touchDetector->coloredImageWithoutBackground()->draw(0, 0, ofGetWidth(), ofGetHeight());
    //_imageLabeler->blobImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    _fnt.drawString("Calibrando varinha...", 20, 40);

    // draw wands
    for(const WandTracker::Wand& wand : _trackedWands) {
        float kw = application()->kinect()->colorImage()->width;
        float kh = application()->kinect()->colorImage()->height;
        float x = wand.position.imageCoords.x;
        float y = wand.position.imageCoords.y;
        ofSetHexColor(0x000000);
        ofCircle(x * ofGetWidth() / kw, y * ofGetHeight() / kh, 7);
        ofSetHexColor(0xFFFF00);
        ofCircle(x * ofGetWidth() / kw, y * ofGetHeight() / kh, 5);

        ofVec3f p = application()->kinect()->depthCoords2worldCoords(wand.position.imageCoords);

        std::stringstream ss;
        ss << std::setprecision(3) << "world: (" << p.x << ", " << p.y << ", " << p.z << ")";
        _fnt2.drawString(wand.type, x * ofGetWidth() / kw, y * ofGetHeight() / kh + 30);
        _fnt2.drawString(ss.str(), x * ofGetWidth() / kw, y * ofGetHeight() / kh + 50);
    }

    // wand screen
    ofSetHexColor(0xFFFFFF);
    ofRect(ofGetWindowWidth()*5/6 - 1, ofGetWindowHeight()*4/6 - 1, ofGetWindowWidth()*5/6 + 2, ofGetWindowHeight()*5/6 + 2);
    ofSetHexColor(0xFF77FF);
    application()->kinect()->grayscaleDepthImage()->draw(ofGetWidth()*5/6, ofGetHeight()*4/6, ofGetWidth()/6, ofGetHeight()/6);
    _imageLabeler->blobImage()->draw(ofGetWidth()*5/6, ofGetHeight()*5/6, ofGetWidth()/6, ofGetHeight()/6);
    if(1) {
        std::stringstream ss;
        ss << "wands";
        ofSetHexColor(0xFFFFFF);
        _fnt2.drawString(ss.str(), ofGetWindowWidth()*5/6+3, ofGetWindowHeight()*5/6+15);
    }
}

void WandCalibrationScene::keyPressed(int key)
{
    if(key == ' ' || key == OF_KEY_RETURN) {
        std::cout << "Calibrating wand..." << std::endl;
        bool r = _wandTracker->endCalibrationProcedure();
        std::cout << (r ? "Success!" : "Failure.") << std::endl;
        application()->setScene(new TUIOScene(application()));
    }
}
