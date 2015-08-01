#include <vector>
#include <thread>
#include "TouchDetection.h"
#include "TUIO.h"
#include "Music.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/BackgroundModel.h"
#include "../core/TouchDetector.h"
#include "../core/TouchTracker.h"
#include "../core/EraserDetector.h"


TouchDetectionScene::TouchDetectionScene(MyApplication* app) : Scene(app), _displayDepthImage(true)
{
    _fnt.loadFont("DejaVuSans.ttf", 12);
    //_touchDetector = new TouchDetector(app->kinect());
    _touchDetector = new TouchDetector(app->kinect(), TouchClassifier::QUALITY_FAST);
    _touchTracker = new TouchTracker(60, 6);
    _eraserDetector = new EraserDetector(app->kinect());
}

TouchDetectionScene::~TouchDetectionScene()
{
    delete _eraserDetector;
    delete _touchTracker;
    delete _touchDetector;
}

void TouchDetectionScene::update()
{
    std::thread t1([&](){
        _touchDetector->update();
        _touchTracker->feed( _touchDetector->touchPoints(), 25, ofGetLastFrameTime() );
    });
    std::thread t2([&](){
        _eraserDetector->update();
    });
    t1.join();
    t2.join();
}

void TouchDetectionScene::draw()
{
    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );
    std::vector<EraserDetector::Eraser> w( _eraserDetector->objects() );

    // depth camera
    if(_displayDepthImage) {
        ofSetHexColor(0x8888DD);
        application()->kinect()->grayscaleDepthImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    }
    else {
        ofSetHexColor(0xFFFFFF);
        application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    }

    // touch events
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end(); ++it) {
        TouchDetector::TouchPoint p = *((*it)(0));
        float x = p.x * ofGetWidth() / _touchDetector->depthImage()->width;
        float y = p.y * ofGetHeight() / _touchDetector->depthImage()->height;
        ofSetHexColor(0xFFFF88);
        ofCircle(x, y, 5);

        if(1 || !_displayDepthImage) {
            std::stringstream ss, ss2;
            ofVec3f world = application()->kinect()->depthCoords2worldCoords(ofVec2f(p.x, p.y));
            ofVec2f pt = application()->kinect()->worldCoords2colorCoords(world);
            x = pt.x * ofGetWidth() / application()->kinect()->colorImage()->width;
            y = pt.y * ofGetHeight() / application()->kinect()->colorImage()->height;
            //ss << std::setprecision(3) << world.x << "," << world.y << "," << world.z;
            //ss2 << "area: " << p.area;
            ss << "area: " << p.area;
            ss2 << std::setprecision(3) << p.surfaceX << ", " << p.surfaceY;
            std::string type = p.type == "projetor_finger" ? "finger" : p.type;

            ofSetHexColor(0x000000);
            _fnt.drawString(ss2.str(), x-10+1, y+25+1);
            _fnt.drawString(ss.str(), x-10+1, y+45+1);
            _fnt.drawString(type, x-10+1, y+65+1);

            ofSetHexColor(0x88FF88);
            _fnt.drawString(ss2.str(), x-10, y+25);
            _fnt.drawString(ss.str(), x-10, y+45);
            _fnt.drawString(type, x-10, y+65);
            //ofCircle(x, y, 5);
        }
    }

    // eraser
    if(_displayDepthImage) {
        for(std::vector<EraserDetector::Eraser>::const_iterator it = w.begin(); it != w.end(); ++it) {
            EraserDetector::Eraser p = *it;
            float x = p.x * ofGetWidth() / _touchDetector->depthImage()->width;
            float y = p.y * ofGetHeight() / _touchDetector->depthImage()->height;
            ofSetHexColor(0xFFFF88);
            ofCircle(x, y, 5);

            std::stringstream ss, ss2, ss3;
            ofVec3f world = application()->kinect()->depthCoords2worldCoords(ofVec2f(p.x, p.y));
            ofVec2f pt = application()->kinect()->worldCoords2colorCoords(world);
            x = pt.x * ofGetWidth() / application()->kinect()->colorImage()->width;
            y = pt.y * ofGetHeight() / application()->kinect()->colorImage()->height;
            ss << "area: " << p.area;
            ss2 << std::setprecision(3) << p.surfaceX << ", " << p.surfaceY;
            ss3 << "eraser (" << std::setprecision(2) << p.coefficient << ")";

            ofSetHexColor(0x000000);
            _fnt.drawString(ss2.str(), x-10+1, y+25+1);
            _fnt.drawString(ss.str(), x-10+1, y+45+1);
            _fnt.drawString(ss3.str(), x-10+1, y+65+1);

            ofSetHexColor(0x88FF88);
            _fnt.drawString(ss2.str(), x-10, y+25);
            _fnt.drawString(ss.str(), x-10, y+45);
            _fnt.drawString(ss3.str(), x-10, y+65);

            ofSetHexColor(0x00FF00);
            //ofCircle(x, y, sqrt(p.area / M_PI)*2);
            ofCircle(ofGetWidth()*4/5 + x/5, ofGetHeight()*4/5 + y/5, 2*sqrt(p.area / M_PI)/5);
        }
    }

    // valid touch area
    if(_displayDepthImage) {
        const BackgroundModel* bgmodel = _touchDetector->backgroundModel();
        float topleft_x = ofGetWidth() * bgmodel->topleftVertex().x / bgmodel->mask()->width;
        float topleft_y = ofGetHeight() * bgmodel->topleftVertex().y / bgmodel->mask()->height;
        float topright_x = ofGetWidth() * bgmodel->toprightVertex().x / bgmodel->mask()->width;
        float topright_y = ofGetHeight() * bgmodel->toprightVertex().y / bgmodel->mask()->height;
        float bottomleft_x = ofGetWidth() * bgmodel->bottomleftVertex().x / bgmodel->mask()->width;
        float bottomleft_y = ofGetHeight() * bgmodel->bottomleftVertex().y / bgmodel->mask()->height;
        float bottomright_x = ofGetWidth() * bgmodel->bottomrightVertex().x / bgmodel->mask()->width;
        float bottomright_y = ofGetHeight() * bgmodel->bottomrightVertex().y / bgmodel->mask()->height;

        ofEnableAlphaBlending();
        ofSetColor(255, 0, 0, 32);
        ofSetLineWidth(3.0f);
        ofLine(topleft_x, topleft_y, topright_x, topright_y);
        ofLine(topright_x, topright_y, bottomright_x, bottomright_y);
        ofLine(bottomright_x, bottomright_y, bottomleft_x, bottomleft_y);
        ofLine(bottomleft_x, bottomleft_y, topleft_x, topleft_y);
        ofDisableAlphaBlending();

        ofSetHexColor(0x000000);
        _fnt.drawString("0,0", topleft_x - 30 + 1, topleft_y - 20 + 1);
        _fnt.drawString("1,1", bottomright_x - 30 + 1, bottomright_y - 20 + 1);
        ofSetHexColor(0xFFFFFF);
        _fnt.drawString("0,0", topleft_x - 30, topleft_y - 20);
        _fnt.drawString("1,1", bottomright_x - 30, bottomright_y - 20);
    }

    // small windows
    if(_displayDepthImage) {
        ofEnableAlphaBlending();
        ofSetColor(255, 255, 255, 192);
        _touchDetector->background()->draw(ofGetWidth()*3/4, 0, ofGetWidth()/4, ofGetHeight()/4);
        _touchDetector->depthImage()->draw(ofGetWidth()*2/4, 0, ofGetWidth()/4, ofGetHeight()/4);
        _touchDetector->depthImageWithoutBackground()->draw(ofGetWidth()*1/4, 0, ofGetWidth()/4, ofGetHeight()/4);
        //_touchDetector->blacknwhiteImageWithoutBackground()->draw(ofGetWidth()*1/4, 0, ofGetWidth()/4, ofGetHeight()/4);
        _touchDetector->blobs()->draw(ofGetWidth()*0/4, 0, ofGetWidth()/4, ofGetHeight()/4);
        _eraserDetector->blobs()->draw(ofGetWidth()*4/5, ofGetHeight()*4/5, ofGetWidth()/5, ofGetHeight()/5);
        ofDisableAlphaBlending();
    }

    // text
    if(_displayDepthImage) {
        ofSetHexColor(0xFFFF88);

        std::stringstream ss;
        ss << "lo: " << _touchDetector->lo();
        _fnt.drawString(ss.str(), ofGetWidth()*4/5, 20);

        ss.str("");
        ss << "hi: " << _touchDetector->hi();
        _fnt.drawString(ss.str(), ofGetWidth()*4/5+10, 40);

        ss.str("");
        ss << "dilate: " << _touchDetector->dil1();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 20);

        ss.str("");
        ss << "erode: " << _touchDetector->ero1();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 40);

        ss.str("");
        ss << "dilate: " << _touchDetector->dil2();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 60);

        ss.str("");
        ss << "erode: " << _touchDetector->ero2();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 80);

        ss.str("");
        ss << "minArea: " << _touchDetector->minArea();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 120);

        ss.str("");
        ss << "maxArea: " << _touchDetector->maxArea();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 140);

        ss.str("");
        ss << "radius: " << 100*_touchDetector->radius() << "cm";
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 180);

        ss.str("");
        ss << "theta: " << _touchDetector->theta();
        _fnt.drawString(ss.str(), 20, ofGetHeight()/4 + 200);

        ss.str("");
        ss << "lo/hi: " << _eraserDetector->lo() << "/" << _eraserDetector->hi();
        _fnt.drawString(ss.str(), ofGetWidth()*4/5+5, ofGetHeight()*4/5+15);

        ss.str("");
        ss << "mi/ma: " << _eraserDetector->minArea() << "/" << _eraserDetector->maxArea();
        _fnt.drawString(ss.str(), ofGetWidth()*4/5+5, ofGetHeight()*4/5+30);

        ss.str("");
        ss << "coef: " << _eraserDetector->classificationCoefficient();
        _fnt.drawString(ss.str(), ofGetWidth()*4/5+5, ofGetHeight()*4/5+45);
    }
}

void TouchDetectionScene::keyPressed(int key)
{
    if(key == OF_KEY_RIGHT)
        _touchDetector->setLo(1 + _touchDetector->lo());
    else if(key == OF_KEY_LEFT)
        _touchDetector->setLo(-1 + _touchDetector->lo());
    else if(key == OF_KEY_UP)
        _touchDetector->setHi(1 + _touchDetector->hi());
    else if(key == OF_KEY_DOWN)
        _touchDetector->setHi(-1 + _touchDetector->hi());
    else if(key == 'q')
        _touchDetector->setDil1(1 + _touchDetector->dil1());
    else if(key == 'a')
        _touchDetector->setDil1(-1 + _touchDetector->dil1());
    else if(key == 'w')
        _touchDetector->setEro1(1 + _touchDetector->ero1());
    else if(key == 's')
        _touchDetector->setEro1(-1 + _touchDetector->ero1());
    else if(key == 'e')
        _touchDetector->setDil2(1 + _touchDetector->dil2());
    else if(key == 'd')
        _touchDetector->setDil2(-1 + _touchDetector->dil2());
    else if(key == 'r')
        _touchDetector->setEro2(1 + _touchDetector->ero2());
    else if(key == 'f')
        _touchDetector->setEro2(-1 + _touchDetector->ero2());
    else if(key == 't')
        _touchDetector->setMinArea(1 + _touchDetector->minArea());
    else if(key == 'g')
        _touchDetector->setMinArea(-1 + _touchDetector->minArea());
    else if(key == 'y')
        _touchDetector->setMaxArea(1 + _touchDetector->maxArea());
    else if(key == 'h')
        _touchDetector->setMaxArea(-1 + _touchDetector->maxArea());
    else if(key == 'u')
        _touchDetector->setRadius(0.001 + _touchDetector->radius());
    else if(key == 'j')
        _touchDetector->setRadius(-0.001 + _touchDetector->radius());
    else if(key == 'i')
        _touchDetector->setTheta(1 + _touchDetector->theta());
    else if(key == 'k')
        _touchDetector->setTheta(-1 + _touchDetector->theta());


    else if(key == 'z')
        _eraserDetector->setLo(-1 + _eraserDetector->lo());
    else if(key == 'x')
        _eraserDetector->setLo(1 + _eraserDetector->lo());
    else if(key == 'c')
        _eraserDetector->setHi(-1 + _eraserDetector->hi());
    else if(key == 'v')
        _eraserDetector->setHi(1 + _eraserDetector->hi());
    else if(key == 'b')
        _eraserDetector->setMinArea(-1 + _eraserDetector->minArea());
    else if(key == 'n')
        _eraserDetector->setMinArea(1 + _eraserDetector->minArea());
    else if(key == 'm')
        _eraserDetector->setMaxArea(-1 + _eraserDetector->maxArea());
    else if(key == ',')
        _eraserDetector->setMaxArea(1 + _eraserDetector->maxArea());
    else if(key == '.')
        _eraserDetector->setClassificationCoefficient(-0.01 + _eraserDetector->classificationCoefficient());
    else if(key == ';')
        _eraserDetector->setClassificationCoefficient(0.01 + _eraserDetector->classificationCoefficient());


    else if(key == ' ' || key == OF_KEY_RETURN) {
        if(!_displayDepthImage) {
            _touchDetector->save();
            if(!ofGetKeyPressed('-'))
                application()->setScene( new TUIOScene(application()) );
            else
                application()->setScene( new MusicScene(application()) );
        }
        else
            _displayDepthImage = false;
    }
}
