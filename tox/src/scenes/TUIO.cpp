#include <vector>
#include <fstream>
#include <thread>
#include "TUIO.h"
#include "TouchDetection.h"
#include "WandCalibration.h"
#include "BackgroundCalibration.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/BackgroundModel.h"
#include "../core/TouchDetector.h"
#include "../core/TouchTracker.h"
#include "../core/Marker3DTracker.h"
#include "../core/ImageLabeler.h"


#define TUIO_CONFIGFILE "data/tuioserver/params.txt"



TUIOScene::TUIOScene(MyApplication* app) : Scene(app), _showWandData(false), _showTouchData(false)
{
    std::cout << "starting tuio server... " << std::endl;
    ofSetBackgroundColor(0, 0, 0);

    // default values
    _tuioHost = "localhost";
    _tuioPort = 3333;

    // creating stuff
    //_touchDetector = new TouchDetector(app->kinect());
    _touchDetector = new TouchDetector(app->kinect(), TouchClassifier::QUALITY_FAST);
    _touchTracker = new TouchTracker(60, 6);
    _eraserDetector = new EraserDetector(app->kinect());
    _eraserTracker = new EraserTracker(20);
    //_3dtracker = new Marker3DTracker(app->kinect(), _touchDetector->backgroundModel(), _imageLabeler, 100, 20.0f); //new Simple3DTracker(_touchDetector, 100, 0.8f, 40, 40, 7.0f);
    _imageLabeler = new ImageLabeler(app->kinect()->colorImage()->width, app->kinect()->colorImage()->height);//, "yellow");
    _wandTracker = new WandTracker(app->kinect(), _touchDetector->backgroundModel(), _imageLabeler, 20.0f, 200);//, "yellow");
    _bottomImageLabeler = new ImageLabeler(app->kinect()->colorImage()->width, app->kinect()->colorImage()->height, "blue");
    _bottomWandTracker = new WandTracker(app->kinect(), _touchDetector->backgroundModel(), _bottomImageLabeler, 20.0f, 200, "blue");
    _loadParams(TUIO_CONFIGFILE);

    _tuioServer = new TUIO::TuioServer(_tuioHost.c_str(), _tuioPort);
    _tuioServer->setVerbose(true);
    _tuioCurrentTime = TUIO::TuioTime::getSessionTime();

    _startTime = ofGetElapsedTimef();
    _fnt.loadFont("DejaVuSans.ttf", 18);
    _fnt2.loadFont("DejaVuSans.ttf", 12);
    _fnt3.loadFont("DejaVuSans.ttf", 24);
    _3dtrackerTarget = 0;
}

TUIOScene::~TUIOScene()
{
    std::cout << "closing tuio server... " << std::endl;
    application()->setDefaultWindowTitle();
    _saveParams(TUIO_CONFIGFILE);

    delete _tuioServer;

    delete _touchTracker;
    delete _touchDetector;
    delete _eraserTracker;
    delete _eraserDetector;
    //delete _3dtracker;
    delete _wandTracker;
    delete _imageLabeler;
    delete _bottomWandTracker;
    delete _bottomImageLabeler;
}

void TUIOScene::update()
{
    // multitouch & tracking
    std::thread t1([&]() {
            _touchDetector->update();
            _touchTracker->feed( _touchDetector->touchPoints(), 25, ofGetLastFrameTime() );
            _trackedTouchPoints = _touchTracker->trackedTouchPoints();
    });

    // multitouch & tracking: eraser
    std::thread t2([&]() {
            _eraserDetector->update();
            _eraserTracker->feed( _eraserDetector->objects(), 75, ofGetLastFrameTime() );
            _trackedErasers = _eraserTracker->trackedErasers();
    });

    // wand tracker
    float dt = ofGetLastFrameTime();
    std::vector<WandTracker::Wand> v1, v2;
    std::thread t3([&]() {
            _imageLabeler->update(_touchDetector->coloredImageWithoutBackground(), _touchDetector->theta());
            _wandTracker->update(dt);
            v1 = _wandTracker->getWands();
    });
    std::thread t4([&]() {
            _bottomImageLabeler->update(_touchDetector->coloredImageWithoutBackground(), _touchDetector->theta());
            _bottomWandTracker->update(dt);
            v2 = _bottomWandTracker->getWands();
    });

    // join threads
    t1.join();
    t2.join();
    t3.join();
    t4.join();

    _trackedWands.clear();
    _trackedWands.insert(_trackedWands.end(), v1.begin(), v1.end());
    _trackedWands.insert(_trackedWands.end(), v2.begin(), v2.end());

/*
    // 3d tracking of the last(?) touching object
    if(_trackedTouchPoints.size() > 0) {
        unsigned candidateId = 0;
        for(int i=0; i<int(_trackedTouchPoints.size()); i++) {
            if((_trackedTouchPoints[i])(0)->_id >= candidateId) { // last blob
                const TouchDetector::TouchPoint* p = (_trackedTouchPoints[i])(0);
                if(p->type.substr(0, 7) == "marker_" || p->type.substr(0, 16) == "projetor_marker_" || p->type.substr(0, 7) == "eraser_") {
                    if(_alreadyTracked.count(p->_id) == 0) {
                        _alreadyTracked.insert(p->_id);
                        _3dtracker->beginFeatureTracking(ofVec2f(p->x, p->y), p->type);
                        candidateId = p->_id;
                        _3dtrackerTarget = p->_id;
                    }
                }
            }
        }
    }

    // update 3d tracking
    _3dtracker->updateFeatureTracking();

    // debug
    if(_3dtracker->isTracking()) {
        ofVec2f pos2d( _3dtracker->coordsOfTrackedFeature() ); // in depth coords
        ofVec3f pos3d( application()->kinect()->depthCoords2worldCoords(pos2d) ); // in world coords
        ofVec3f pos( _3dtracker->sceneCoordsOfTrackedFeature() ); // in [0,1]^3
        std::cout << "[3d.scene] " << pos.x << " " << pos.y << " " << pos.z << std::endl;
        std::cout << "[3d.world] " << pos3d.x << " " << pos3d.y << " " << pos3d.z << std::endl;
    }
*/

    // share via network
    _shareStuff();

    // window title
    if(_showWandData)
        application()->setWindowTitle("Magic Wand");
    else if(_showTouchData)
        application()->setWindowTitle("Magic Touch");
    else
        application()->setWindowTitle("TUIO"); //DefaultWindowTitle();
}

void TUIOScene::_shareStuff()
{
    _tuioCurrentTime = TUIO::TuioTime::getSessionTime();
    _tuioServer->initFrame( _tuioCurrentTime );

    auto gotTrackedTouchPoint = [&](unsigned id) {
        bool found = false;
        for(int i=0; i<(int)_trackedTouchPoints.size() && !found; i++) {
            const TouchDetector::TouchPoint* p = (_trackedTouchPoints[i])(0);
            found = (p->_id == id);
        }
        return found;
    };

    // ======================== [ TUIO sharing ] ========================

        std::set<unsigned int> unused;

        // update & add TUIO objects
        for(int i=0; i<(int)_trackedTouchPoints.size(); i++) {
            TouchTracker::TrackedTouchPoint q = _trackedTouchPoints[i];
            const TouchDetector::TouchPoint* p = q(0);

            auto it = _tuioObjects.find(p->_id);
            if(it == _tuioObjects.end()) {
                // new object
                TUIO::TuioObject* tuioObject;
                std::cout << "[TUIO] adding object #" << p->_id << " of type " << string2touchType(p->type) << " (\"" << p->type << "\")" <<std::endl;
                tuioObject = new TUIO::TuioObject(_tuioCurrentTime, _tuioServer->getSessionID(), string2touchType(p->type), p->surfaceX, p->surfaceY, 0.0f);
                _tuioServer->addExternalTuioObject(tuioObject);
                _tuioObjects.insert( std::pair<unsigned int, TUIO::TuioObject*>( p->_id, tuioObject ) );
            }
            else {
                // update old object
                TUIO::TuioObject* tuioObject = it->second;
                tuioObject->update(_tuioCurrentTime, p->surfaceX, p->surfaceY, 0.0f);
                _tuioServer->updateExternalTuioObject(tuioObject);
            }
        }

        // update 3D TUIO objects
/*
        if(_3dtracker->isTracking()) {
            ofVec3f pos3d( _3dtracker->sceneCoordsOfTrackedFeature() ); // in [0,1]^3
            TUIO::TuioObject* tuioObject = _tuioObjects[ _3dtrackerTarget ];
            if(tuioObject && !gotTrackedTouchPoint(_3dtrackerTarget)) {
                //tuioObject->update(_tuioCurrentTime, pos3d.x, pos3d.y, pos3d.z); // FIXME: weird projection ((x,y) coords)
                tuioObject->update(_tuioCurrentTime, tuioObject->getX(), tuioObject->getY(), pos3d.z);
                _tuioServer->updateExternalTuioObject(tuioObject);
            }
        }
*/

        // remove unused TUIO objects
        unused.clear();
        for(auto it = _tuioObjects.begin(); it != _tuioObjects.end(); it++) {
            if(1) { //if(!(_3dtracker->isTracking() && it->first == _3dtrackerTarget)) {
                if(!gotTrackedTouchPoint(it->first))
                    unused.insert(it->first);
            }
        }
        for(const unsigned int& q : unused) {
            TUIO::TuioObject* tuioObject = _tuioObjects[q];
            std::cout << "[TUIO] removing object #" << q << " (" << tuioObject << ")" << std::endl;
            _tuioObjects.erase(q);
            _tuioServer->removeExternalTuioObject( tuioObject );
            delete tuioObject;
        }

        // update & add erasers
        for(int i=0; i<(int)_trackedErasers.size(); i++) {
            EraserTracker::TrackedEraser q = _trackedErasers[i];
            const EraserDetector::Eraser* p = q(0);

            auto it = _tuioErasers.find(p->_id);
            if(it == _tuioErasers.end()) {
                // new eraser
                TUIO::TuioObject* tuioObject;
                std::cout << "[TUIO] adding eraser #" << p->_id << std::endl;
                tuioObject = new TUIO::TuioObject(_tuioCurrentTime, _tuioServer->getSessionID(), ERASER, p->surfaceX, p->surfaceY, 0.0f);
                _tuioServer->addExternalTuioObject(tuioObject);
                _tuioErasers.insert( std::pair<unsigned int, TUIO::TuioObject*>( p->_id, tuioObject ) );
            }
            else {
                // update old eraser
                TUIO::TuioObject* tuioObject = it->second;
                tuioObject->update(_tuioCurrentTime, p->surfaceX, p->surfaceY, 0.0f);
                _tuioServer->updateExternalTuioObject(tuioObject);
            }
        }

        // remove unused erasers
        unused.clear();
        for(auto it = _tuioErasers.begin(); it != _tuioErasers.end(); it++) {
            bool found = false;
            for(int i=0; i<(int)_trackedErasers.size() && !found; i++) {
                const EraserDetector::Eraser* p = (_trackedErasers[i])(0);
                found = (p->_id == it->first);
            }
            if(!found)
                unused.insert(it->first);
        }
        for(const unsigned int& q : unused) {
            TUIO::TuioObject* tuioObject = _tuioErasers[q];
            std::cout << "[TUIO] removing eraser #" << q << " (" << tuioObject << ")" << std::endl;
            _tuioErasers.erase(q);
            _tuioServer->removeExternalTuioObject( tuioObject );
            delete tuioObject;
        }

        // update & add wands
        for(int i=0; i<(int)_trackedWands.size(); i++) {
            WandTracker::Wand& q = _trackedWands[i];

            auto it = _tuioWands.find(q.id);
            if(it == _tuioWands.end()) {
                // new wand
                TUIO::TuioObject* tuioObject;
                std::cout << "[TUIO] adding wand #" << q.id << " of type " << string2touchType(q.type) << " (\"" << q.type << "\")" << std::endl;
                tuioObject = new TUIO::TuioObject(_tuioCurrentTime, _tuioServer->getSessionID(), string2touchType(q.type), q.position.coords.x, q.position.coords.y, q.position.coords.z);
                _tuioServer->addExternalTuioObject(tuioObject);
                _tuioWands.insert( std::pair<unsigned int, TUIO::TuioObject*>( q.id, tuioObject ) );
            }
            else {
                // update old wand
                TUIO::TuioObject* tuioObject = it->second;
                tuioObject->update(_tuioCurrentTime, q.position.coords.x, q.position.coords.y, q.position.coords.z);
                _tuioServer->updateExternalTuioObject(tuioObject);
            }
        }

        // remove unused wands
        unused.clear();
        for(auto it = _tuioWands.begin(); it != _tuioWands.end(); it++) {
            bool found = false;
            for(int i=0; i<(int)_trackedWands.size() && !found; i++) {
                const WandTracker::Wand& p = _trackedWands[i];
                found = (p.id == it->first);
            }
            if(!found)
                unused.insert(it->first);
        }
        for(const unsigned int& q : unused) {
            TUIO::TuioObject* tuioObject = _tuioWands[q];
            std::cout << "[TUIO] removing wand #" << q << " (" << tuioObject << ")" << std::endl;
            _tuioWands.erase(q);
            _tuioServer->removeExternalTuioObject( tuioObject );
            delete tuioObject;
        }

    // ======================== [ TUIO sharing ] ========================

    _tuioServer->stopUntouchedMovingObjects(); // do we need this?
    _tuioServer->commitFrame();

}

void TUIOScene::draw()
{
    ofSetHexColor(0xFFFFFF);

    //
    // DEBUG 
    //
    if(_showWandData) {
        // display wand data
        //application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
        _touchDetector->coloredImageWithoutBackground()->draw(0, 0, ofGetWidth(), ofGetHeight());

        bool active = false;
        for(const WandTracker::Wand& wand : _trackedWands)
            active = active || (wand.type == "wand_yellow:active");
        for(const WandTracker::Wand& wand : _trackedWands) {
            float kw = application()->kinect()->colorImage()->width;
            float kh = application()->kinect()->colorImage()->height;
            float x = wand.position.imageCoords.x;
            float y = wand.position.imageCoords.y;
            std::stringstream ss;
            ss << std::setprecision(2) << " (" << wand.position.coords.x << ", " << wand.position.coords.y << ", " << wand.position.coords.z << ")";
            ofSetHexColor(0x000000);
            _fnt2.drawString(ss.str(), x * ofGetWidth() / kw + 2 + 15, y * ofGetHeight() / kh + 5 + 2);
            ofSetHexColor(active ? 0xFFEE55 : 0xFF5555);
            _fnt2.drawString(ss.str(), x * ofGetWidth() / kw + 15, y * ofGetHeight() / kh + 5);
        }
        return;
    }
    else if(_showTouchData) {
        // valid touch area
        application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());

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

        // display touch events
        std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );
        for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end(); ++it) {
            TouchDetector::TouchPoint p = *((*it)(0));
            float x = p.x * ofGetWidth() / _touchDetector->depthImage()->width;
            float y = p.y * ofGetHeight() / _touchDetector->depthImage()->height;
            ofSetHexColor(0xFFFF88);
            ofCircle(x, y, 5);

            if(1) {
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
                _fnt2.drawString(ss2.str(), x-10+1, y+25+1);
                _fnt2.drawString(ss.str(), x-10+1, y+45+1);
                _fnt2.drawString(type, x-10+1, y+65+1);

                ofSetHexColor(0x88FF88);
                _fnt2.drawString(ss2.str(), x-10, y+25);
                _fnt2.drawString(ss.str(), x-10, y+45);
                _fnt2.drawString(type, x-10, y+65);
                //ofCircle(x, y, 5);
            }
        }
        return;
    }



    // background
    //ofSetHexColor(0x8888DD);
    //application()->kinect()->grayscaleDepthImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    ofSetHexColor(0xFFFFFF);
    _touchDetector->coloredImageWithoutBackground()->draw(0, 0, ofGetWidth(), ofGetHeight());
    //_imageLabeler->blobImage()->draw(0, 0, ofGetWidth(), ofGetHeight());

    // 3d tracking
/*
    float kw = application()->kinect()->colorImage()->width;
    float kh = application()->kinect()->colorImage()->height;
    ofSetHexColor(0x000000);
    ofCircle(_3dtracker->coordsOfTrackedFeature().x * ofGetWidth() / kw, _3dtracker->coordsOfTrackedFeature().y * ofGetHeight() / kh, 7);
    ofSetHexColor(_3dtracker->isTracking() ? 0xFFFF00 : 0xFF5555);
    ofCircle(_3dtracker->coordsOfTrackedFeature().x * ofGetWidth() / kw, _3dtracker->coordsOfTrackedFeature().y * ofGetHeight() / kh, 5);
*/

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

        std::stringstream ss;
        ss << std::setprecision(3) << " (" << wand.position.coords.x << ", " << wand.position.coords.y << ", " << wand.position.coords.z << ")";
        _fnt2.drawString(wand.type, x * ofGetWidth() / kw, y * ofGetHeight() / kh + 30);
        _fnt2.drawString(ss.str(), x * ofGetWidth() / kw, y * ofGetHeight() / kh + 50);
    }

    // draw tracked points
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = _trackedTouchPoints.begin(); it != _trackedTouchPoints.end(); ++it) {
        const TouchDetector::TouchPoint* p0 = (*it)(0);
        const TouchDetector::TouchPoint* p1 = (*it)(-1);

        float x0 = p0->surfaceX * ofGetWidth();
        float y0 = p0->surfaceY * ofGetHeight();
        float x1 = p1->surfaceX * ofGetWidth();
        float y1 = p1->surfaceY * ofGetHeight();

        switch(string2touchType(p0->type)) {
            case MARKER_YELLOW:   ofSetHexColor(0xFFFF33); break;
            case MARKER_BLUE:     ofSetHexColor(0x2266FF); break;
            case MARKER_GREEN:    ofSetHexColor(0x33FF33); break;
            case MARKER_MAGENTA:  ofSetHexColor(0xFF0099); break;
            default:              ofSetHexColor(0xFFFFFF); break;
        }

        ofSetLineWidth(5.0f);
        ofLine(x1, y1, x0, y0);
    }

    // draw erasers
    for(std::vector<EraserTracker::TrackedEraser>::const_iterator it = _trackedErasers.begin(); it != _trackedErasers.end(); ++it) {
        const EraserDetector::Eraser* e = (*it)(0);
        float x = e->surfaceX * ofGetWidth();
        float y = e->surfaceY * ofGetHeight();

        ofSetLineWidth(5.0f);
        ofSetHexColor(0xFFFFFF);
        ofCircle(x, y, 50);
    }

    // wand screen
    ofSetHexColor(0xFFFFFF);
    ofRect(ofGetWindowWidth()*5/6 - 1, ofGetWindowHeight()*3/6 - 1, ofGetWindowWidth()*5/6 + 2, ofGetWindowHeight()*5/6 + 2);
    ofSetHexColor(0xFFFF00);
    _imageLabeler->blobImage()->draw(ofGetWidth()*5/6, ofGetHeight()*3/6, ofGetWidth()/6, ofGetHeight()/6);
    ofEnableAlphaBlending();
    ofSetColor(0,64,255,128);
    _bottomImageLabeler->blobImage()->draw(ofGetWidth()*5/6, ofGetHeight()*3/6, ofGetWidth()/6, ofGetHeight()/6);
    ofDisableAlphaBlending();
    if(1) {
        std::stringstream ss;
        ss << "wands";
        ofSetHexColor(0xFFFFFF);
        _fnt2.drawString(ss.str(), ofGetWindowWidth()*5/6+3, ofGetWindowHeight()*3/6+15);
    }

    // eraser screen
    ofSetHexColor(0xFFFFFF);
    ofRect(ofGetWindowWidth()*5/6 - 1, ofGetWindowHeight()*4/6 - 1, ofGetWindowWidth()*5/6 + 2, ofGetWindowHeight()*5/6 + 2);
    _eraserDetector->blobs()->draw(ofGetWidth()*5/6, ofGetHeight()*4/6, ofGetWidth()/6, ofGetHeight()/6);
    if(1) {
        std::stringstream ss;
        if(_trackedErasers.size() > 0) {
            const EraserDetector::Eraser* e = (_trackedErasers[0])(0);
            ss << "eraser (" << std::setprecision(3) << e->coefficient << ")";
            ofSetHexColor(0x00FF00);
        }
        else
            ss << "eraser (-)";
        _fnt2.drawString(ss.str(), ofGetWindowWidth()*5/6+3, ofGetWindowHeight()*4/6+15);
    }

    // touch detector screen
    ofSetHexColor(0xFFFFFF);
    ofRect(ofGetWindowWidth()*5/6 - 1, ofGetWindowHeight()*5/6 - 1, ofGetWindowWidth()*5/6 + 2, ofGetWindowHeight()*5/6 + 2);
    _touchDetector->blobs()->draw(ofGetWindowWidth()*5/6, ofGetWindowHeight()*5/6, ofGetWindowWidth()/6, ofGetWindowHeight()/6);
    if(1) {
        int n = int(_touchDetector->touchPoints().size());
        std::stringstream ss;
        ss << "touch (" << n << ")";
        if(n > 0) ofSetHexColor(0x00FF00);
        _fnt2.drawString(ss.str(), ofGetWindowWidth()*5/6+3, ofGetWindowHeight()*5/6+15);
    }

    // status
    ofSetHexColor(0xFFFF00);
    if(1) {
        std::stringstream ss;
        int y = _touchDetector->blacknwhiteImageWithoutBackground()->height - 50;
        //ss << "cursor3d.lookupRadius: " << _3dtracker->lookupRadius();
        ss << "wandTracker.lookupRadius: " << _wandTracker->lookupRadius();
        _fnt2.drawString(ss.str(), 40, y);
    }
    if(1) {
        std::stringstream ss;
        int y = _touchDetector->blacknwhiteImageWithoutBackground()->height - 30;
        ss << "wandTracker.minArea: " << _wandTracker->minArea();// << "cm";
        _fnt2.drawString(ss.str(), 40, y);
    }
    if(1) {
        /*std::stringstream ss;
        int y = _touchDetector->blacknwhiteImageWithoutBackground()->height - 30;
        ofVec3f p = _3dtracker->sceneCoordsOfTrackedFeature();
        ss << "cursor3d.scenePos: ";
        if(_3dtracker->isTracking())
            ss << std::setprecision(3) << p.x << ", " << p.y << ", " << p.z;
        else
            ss << "-";
        _fnt2.drawString(ss.str(), 40, y);*/
        std::stringstream ss;
        ss << "wands (" << _trackedWands.size() << "): ";
        for(const WandTracker::Wand& w : _trackedWands) {
            ss << "#" << w.id << " (area: " << w.area << "), ";
        }
        int y = _touchDetector->blacknwhiteImageWithoutBackground()->height - 10;
        _fnt2.drawString(ss.str(), 40, y);
    }
    /*if(1) {
        std::stringstream ss;
        int y = _touchDetector->blacknwhiteImageWithoutBackground()->height - 10;
        ss << "cursor3d.weightedMatchingCoeff: " << _3dtracker->weightedMatchingCoefficient();
        _fnt2.drawString(ss.str(), 40, y);
    }*/
    if(1) {
        std::stringstream ss;
        ss << "TUIOserver@" << _tuioHost << ":" << _tuioPort << " is " << (_tuioServer->isConnected() ? "[online]" : "[offline]");
        ofSetHexColor(0x000000);
        _fnt.drawString(ss.str(), 40, 40);
        ofSetHexColor(0xFFFFFF);
        _fnt.drawString(ss.str(), 41, 41);
    }
}

void TUIOScene::keyPressed(int key)
{
/*
    if(key == 'q')
        _3dtracker->setLookupRadius(1 + _3dtracker->lookupRadius());
    else if(key == 'a')
        _3dtracker->setLookupRadius(-1 + _3dtracker->lookupRadius());
    else if(key == 'w')
        _3dtracker->setMaxHeight(0.5f + _3dtracker->maxHeight());
    else if(key == 's')
        _3dtracker->setMaxHeight(-0.5f + _3dtracker->maxHeight());
    else if(key == ' ' || key == OF_KEY_RETURN)
        application()->setScene(new TouchDetectionScene(application()));
*/
    if(key == 'q')
        _wandTracker->setLookupRadius(1 + _wandTracker->lookupRadius());
    else if(key == 'a')
        _wandTracker->setLookupRadius(-1 + _wandTracker->lookupRadius());
    else if(key == 'w')
        _wandTracker->setMinArea(1 + _wandTracker->minArea());
    else if(key == 's')
        _wandTracker->setMinArea(-1 + _wandTracker->minArea());
    else if(key == ' ' || key == OF_KEY_RETURN)
        application()->setScene(new TouchDetectionScene(application()));
    else if(key == 'c')
        application()->setScene(new WandCalibrationScene(application()));
    else if(key == OF_KEY_F11) {
        if(!_showWandData && !_showTouchData) {
            _showWandData = true;
            _showTouchData = false;
        }
        else if(_showWandData && !_showTouchData) {
            _showWandData = false;
            _showTouchData = true;
        }
        else if(!_showWandData && _showTouchData) {
            _showWandData = false;
            _showTouchData = false;
        }
        else {
            _showWandData = false;
            _showTouchData = false;
        }
    }
    else if(key == OF_KEY_F12)
        application()->setScene(new BackgroundCalibrationScene(application()));
}


void TUIOScene::_saveParams(std::string filepath)
{
    std::ofstream f(filepath.c_str());

    if(f.is_open()) {
        f <<
            _tuioHost << " " <<
            _tuioPort << "\n" <<
            _wandTracker->lookupRadius() << " " <<
            _wandTracker->minArea() << " " <<
            //_3dtracker->maxHeight() << " " <<
            //_3dtracker->lookupRadius() << " " <<
        "";
    }
}

void TUIOScene::_loadParams(std::string filepath)
{
    int lookupRadius;
    int minArea;
    std::ifstream f(filepath.c_str());

    if(f.is_open()) {
        f >>
            _tuioHost >>
            _tuioPort >>
            lookupRadius >>
            minArea
        ;

        _wandTracker->setLookupRadius(lookupRadius);
        _wandTracker->setMinArea(minArea);
        //_3dtracker->setLookupRadius(lookupRadius);
    }
}








TUIOScene::TouchType TUIOScene::string2touchType(std::string str) const
{
    if(str.substr(0, 9) == "projetor_")
        str = str.substr(9);

    return (
        (str == "marker_blue") ? MARKER_BLUE : (
            (str == "marker_yellow") ? MARKER_YELLOW : (
                (str == "marker_green" || str == "eraser_green") ? MARKER_GREEN : (
                    (str == "marker_magenta") ? MARKER_MAGENTA : 
                        (str == "eraser") ? ERASER : 
                            (str == "wand_yellow") ? WAND_YELLOW : 
                                (str == "wand_yellow:active") ? WAND_YELLOW_ACTIVE :
                                    (str == "wand_blue") ? WAND_BLUE :
                                        FINGER /*(str == "finger" ? FINGER : NONE)*/
                    )
                )
            )
        );
}

