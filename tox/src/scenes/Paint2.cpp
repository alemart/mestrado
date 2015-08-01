#include <vector>
#include "Paint2.h"
#include "TouchDetection.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/BackgroundModel.h"
#include "../core/TouchDetector.h"
#include "../core/TouchTracker.h"


Paint2Scene::Paint2Scene(MyApplication* app) : Scene(app), _debug(false)
{
    ofSetBackgroundColor(0, 0, 0);
    _touchDetector = new TouchDetector(app->kinect());
    _touchTracker = new TouchTracker(60, 6);
}

Paint2Scene::~Paint2Scene()
{
    delete _touchTracker;
    delete _touchDetector;
}

void Paint2Scene::update()
{
    _touchDetector->update();
    _touchTracker->feed( _touchDetector->touchPoints(), 25, ofGetLastFrameTime() );

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

        float cx = _cameraPosition.x;
        float cy = _cameraPosition.y;

        if(type != "null" && type != "finger" && type != "projetor_finger") {
            Segment s;
            s.currX = x0 + cx;
            s.currY = y0 + cy;
            s.prevX = x1 + cx;
            s.prevY = y1 + cy;
            s.color = type;
            _segments.push_back(s);
        }
        else {
            const float MINDIST = 13;
            const float MAXDIST = 100;
            float dx = x0 - x1;
            float dy = y0 - y1;
            if(dx*dx + dy*dy >= MINDIST*MINDIST) {
                if(dx*dx + dy*dy <= MAXDIST*MAXDIST) {
                    _cameraPosition.x -= dx * 1.0f;
                    _cameraPosition.y -= dy * 1.0f;
                }
            }
        }
    }
}

void Paint2Scene::draw()
{
    ofSetBackgroundColor(0, 0, 0);
    ofSetHexColor(0x0);
    ofRect(0, 0, ofGetWidth(), ofGetHeight());
    for(int i=0; i<(int)_segments.size(); i++)
        _drawSegment(&_segments[i]);

    if(_debug) {
        ofSetHexColor(0xFFFFFF);
        ofRect(ofGetWindowWidth()*5/6 - 1, ofGetWindowHeight()*5/6 - 1, ofGetWindowWidth()*5/6 + 2, ofGetWindowHeight()*5/6 + 2);
        _touchDetector->blobs()->draw(ofGetWindowWidth()*5/6, ofGetWindowHeight()*5/6, ofGetWindowWidth()/6, ofGetWindowHeight()/6);
    }
}

void Paint2Scene::_drawSegment(Segment* s)
{
    float cx = _cameraPosition.x;
    float cy = _cameraPosition.y;

    if(s) {
        float x0 = s->currX - cx;
        float y0 = s->currY - cy;
        float x1 = s->prevX - cx;
        float y1 = s->prevY - cy;
        std::string type = s->color;
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
}

void Paint2Scene::keyPressed(int key)
{
    if(key == ' ' || key == OF_KEY_RETURN) {
        _touchDetector->save();
        application()->setScene(
            new TouchDetectionScene(application())
        );
    }
    else if(key == 'd')
        _debug = !_debug;
}
