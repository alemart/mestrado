#include <vector>
#include "Music.h"
#include "TouchDetection.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/BackgroundModel.h"
#include "../core/TouchDetector.h"
#include "../core/TouchTracker.h"


#define TEMPO_BASE          0.8 //0.75 // in units per second
#define TEMPO_MULTIPLIER    1.0
#define MAX_PLAYING_SAMPLES 5 // max number of simultaneous samples
#define COMPASS_NUM_SAMPLES 8


MusicScene::MusicScene(MyApplication* app) : Scene(app), _debug(false), _isUsingAProjector(true), _tempo(0.0f), _cursor(0.0f)
{
    ofSetBackgroundColor(0, 0, 0);
    _touchDetector = new TouchDetector(app->kinect(), TouchClassifier::QUALITY_FASTEST);
    _touchTracker = new TouchTracker(60, 6);
    ofSeedRandom();
    _loadSoundEffects();
    _fnt.loadFont("DejaVuSans.ttf", 12);
    _fnt2.loadFont("DejaVuSans.ttf", 24);
    _startTime = ofGetElapsedTimef();
    ofHideCursor();
}

MusicScene::~MusicScene()
{
    delete _touchTracker;
    delete _touchDetector;

    for(std::map<int,ofSoundPlayer*>::iterator it = _sfx.begin(); it != _sfx.end(); ++it) {
        it->second->unloadSound();
        //delete it->second; // crash??
    }
    _sfx.clear();
    ofShowCursor();
}

void MusicScene::update()
{
    // multitouch
    _touchDetector->update();
    _touchTracker->feed( _touchDetector->touchPoints(), 25, ofGetLastFrameTime() );
    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );

    // sort touch points by increasing ID
    std::sort(v.begin(), v.end(), [](const TouchTracker::TrackedTouchPoint& a, const TouchTracker::TrackedTouchPoint& b) -> bool {
        return a(0)->_id < b(0)->_id;
    });

    // adjust the tempo: wand gesture
    float oldtempo = _tempo;
    int numFingers = 0;
    for(TouchTracker::TrackedTouchPoint& q : v) {
        const TouchDetector::TouchPoint* p0 = q(0);
        const TouchDetector::TouchPoint* p1 = q(-1);
        std::string type = _namefix(p0->type);
        if(type == "finger") {
            numFingers++;
            _tempo += p0->surfaceX - p1->surfaceX;
        }
    }
    if(numFingers < 2)
        _tempo = oldtempo;

    // reset tempo & cursor
    if(numFingers >= 4) {
        _tempo = 0.0f;
        _cursor = 0.0f;
        _playedTokens.clear();
    }

    // particles
    std::vector<Particle> ps;
    for(std::vector<Particle>::iterator it = _particles.begin(); it != _particles.end(); ++it) {
        it->update();
        if(it->timeToLive > 0.0f)
            ps.push_back(*it);
    }
    _particles = ps;

    // circle effects
    std::vector<CircleFx> cfx;
    for(std::vector<CircleFx>::iterator it = _circleFxs.begin(); it != _circleFxs.end(); ++it) {
        it->update();
        if(it->timeToLive > 0.0f)
            cfx.push_back(*it);
    }
    _circleFxs = cfx;

    // music canvas
    _updateMusicCanvas();
}

void MusicScene::draw()
{
    if(_isUsingAProjector)
        _projectorDraw();
    else
        _regularDraw();
}

void MusicScene::_projectorDraw()
{
    // music canvas
    _drawMusicCanvas(0, 0, ofGetWidth(), ofGetHeight(), 36, _cursor);

    // particles
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    for(std::vector<Particle>::iterator it = _particles.begin(); it != _particles.end(); ++it)
        it->draw(true);
    ofDisableBlendMode();

    // circle effects
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for(std::vector<CircleFx>::iterator it = _circleFxs.begin(); it != _circleFxs.end(); ++it)
        it->draw(true);
    ofDisableBlendMode();

    // magic wand effect
    bool gotSomeOtherFinger = false;
    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end(); ++it) {
        const TouchDetector::TouchPoint* p = (*it)(0);
        float xpos = p->surfaceX * ofGetWidth();
        float ypos = p->surfaceY * ofGetHeight();
        bool isFinger = (_namefix(p->type) == "finger");

        if(isFinger && gotSomeOtherFinger)
            spawnParticleAt(xpos, ypos, ofColor::fromHex(0xFFFFFF/*0xFF3333*/), ofRandom(0, 359));
        gotSomeOtherFinger = gotSomeOtherFinger || isFinger;
    }
}

void MusicScene::_regularDraw()
{
    // background
    if(!_debug) {
        ofSetHexColor(0xFFFFFF);
        application()->kinect()->colorImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    }
    else {
        ofSetHexColor(0x8888DD);
        application()->kinect()->grayscaleDepthImage()->draw(0, 0, ofGetWidth(), ofGetHeight());
    }

    // a text
    std::string txt = " ";
    _fnt.drawString(txt, (ofGetWidth() - _fnt.stringWidth(txt))/2, (ofGetHeight() - _fnt.stringHeight(txt))/2);

    // particles
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    for(std::vector<Particle>::iterator it = _particles.begin(); it != _particles.end(); ++it)
        it->draw();
    ofDisableBlendMode();

    // touch points
    bool gotSomeOtherFinger = false;
    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end(); ++it) {
        const TouchDetector::TouchPoint* p = (*it)(0);
        ofVec3f world = application()->kinect()->depthCoords2worldCoords(ofVec2f(p->x, p->y));
        ofVec2f pt = application()->kinect()->worldCoords2colorCoords(world);
        float xpos = pt.x * ofGetWidth() / application()->kinect()->colorImage()->width;
        float ypos = pt.y * ofGetHeight() / application()->kinect()->colorImage()->height;
        bool isFinger = (_namefix(p->type) == "finger");

        ofSetHexColor(0x000000);
        ofCircle(xpos+1, ypos+1, 5);
        ofSetHexColor(!isFinger ? 0xFFFF88 : 0xFF8888);
        ofCircle(xpos, ypos, 5);

        std::stringstream ss;
        ss << std::setprecision(3) << p->surfaceX << ", " << p->surfaceY;

        ofSetHexColor(0x000000);
        _fnt.drawString(_namefix(p->type), xpos-10+1, ypos+25+1);
        _fnt.drawString(ss.str(), xpos-10+1, ypos+45+1);
        ofSetHexColor(!isFinger ? 0xFFFF88 : 0xFF8888);
        _fnt.drawString(_namefix(p->type), xpos-10, ypos+25);
        _fnt.drawString(ss.str(), xpos-10, ypos+45);

        for(Token& t : _tokens) {
            if(t.id == p->_id && t.isActive)
                spawnCircleFxAt(xpos, ypos);
        }

        // magic wand effect
        if(isFinger && gotSomeOtherFinger)
            spawnParticleAt(xpos, ypos, ofColor::fromHex(0xFF88888), ofRandom(0, 359));
        gotSomeOtherFinger = gotSomeOtherFinger || isFinger;
    }

    // music canvas
    _drawMusicCanvas(0, 0, ofGetWidth() * 1/3, ofGetHeight() * 1/3, 8, _cursor);

    // circle effects
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for(std::vector<CircleFx>::iterator it = _circleFxs.begin(); it != _circleFxs.end(); ++it)
        it->draw();
    ofDisableBlendMode();

    // touching blobs & fps
    if(_debug) {
        ofSetHexColor(0xFFFFFF);
        ofRect(ofGetWidth()*5/6 - 1, ofGetHeight()*5/6 - 1, ofGetWidth()*1/6 + 2, ofGetHeight()*1/6 + 2);
        _touchDetector->blobs()->draw(ofGetWidth()*5/6, ofGetHeight()*5/6, ofGetWidth()/6, ofGetHeight()/6);

        std::string txt(std::string("fps: ") + ofToString(ofGetFrameRate(), 2));
        ofSetHexColor(0x000000);
        _fnt.drawString(txt, 10 + 1, ofGetHeight() - _fnt.stringHeight(txt) - 1 + 1);
        ofSetHexColor(0xFFFFFF);
        _fnt.drawString(txt, 10, ofGetHeight() - _fnt.stringHeight(txt) - 1);
    }
}

void MusicScene::_drawMusicCanvas(int x, int y, int width, int height, int discRadius, float cursor)
{
    // background
    if(!_isUsingAProjector) {
        ofSetHexColor(0xFFFFFF);
        ofRect(x, y, width, height);
        ofSetHexColor(0x000000);
        ofRect(x+1, y+1, width-2, height-2);
        ofSetHexColor(0x555555);
        for(int i=0; i<3; i++)
            ofLine(x+1, y+1 + (height-2)*i/3, x+width-1, y+1 + (height-2)*i/3);
    }
    else {
        ofSetHexColor(0xFFFF00);
        ofSetHexColor(0x000000);
        ofRect(x, y, width, height);
        ofSetHexColor(0x222222);
        ofRect(x+3, y+3, width-4, height-4);
        //ofSetHexColor(0x333333);
        ofSetHexColor(0x888888);
        bool paint = true;
        for(int row=0; row<3; row++) {
            bool firstpaint = paint;
            for(int col=0; col<COMPASS_NUM_SAMPLES; col++) {
                if(true == (paint = !paint)) {
                    ofRect(x+3 + col * (width-4) / COMPASS_NUM_SAMPLES, y+3 + row * (height-4) / 3, (width-4) / COMPASS_NUM_SAMPLES, (height-4) / 3);
                }
            }
            if(firstpaint == paint)
                paint = !paint;
        }

/*
        ofSetHexColor(0x777777);
        std::string label[3] = { "grave", "medio", "agudo" };
        for(int j=0; j<3; j++) {
            ofPushMatrix();
            ofTranslate(x+3 + (COMPASS_NUM_SAMPLES-1) * (width-4) / COMPASS_NUM_SAMPLES + 50, y+3 + (j+0.3f) * (height-4)/3);
            ofRotate(-90);
            ofScale(-1.0f, -1.0f);
            _fnt2.drawString(label[j], 0, 0);
            ofPopMatrix();
        }
*/
    }

    // draw tokens
    for(Token& t : _tokens) {
        float xpos, ypos;

        if(!_isUsingAProjector) {
            xpos = (x+3 + discRadius) + t.x * (width-4 - 2*discRadius);
            ypos = (y+3 + discRadius) + t.y * (height-4 - 2*discRadius);
        }
        else {
            xpos = (x+1 + discRadius) + t._x * (width-2 - 2*discRadius);
            ypos = (y+1 + discRadius) + t._y * (height-2 - 2*discRadius);
        }

        ofColor color = ([](int type) -> ofColor {
            if(type == 0)
                return ofColor::fromHex(0x44AAFF);
            else if(type == 2)
                return ofColor::fromHex(0x33FF33);
            else if(type == 1)
                return ofColor::fromHex(0xFFFF77);
            else if(type == 3)
                return ofColor::fromHex(0xFF0099);
            else
                return ofColor::fromHex(0xFFFFFF);
        })(t.type);

        if(_isUsingAProjector) {
            float R = discRadius, r = discRadius * 0.85f;
            ofPath curve;
            ofPoint p(ofVec2f(xpos,ypos));
            curve.arc(p, R, R, 0, 360);
            curve.arcNegative(p, r, r, 0, 360);
            curve.close();
            curve.setArcResolution(60);
            curve.setFillColor(color);
            curve.setFilled(true);
            curve.draw();
        }
        else {
            ofSetColor(color);
            ofCircle(xpos, ypos, discRadius);
        }

        if(t.isActive)
            spawnCircleFxAt(xpos, ypos);
    }

    // draw cursor
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(255, 32, 32, 192);
    ofRect((x+1 + discRadius) + std::min(1.0f, std::max(0.0f, cursor)) * (width-2 - 2*discRadius), y+1, discRadius / 3, height-2);
    ofDisableBlendMode();

    // show fps?
    if(_debug) {
        std::string txt(std::string("fps: ") + ofToString(ofGetFrameRate(), 2));
        ofSetHexColor(0x000000);
        _fnt.drawString(txt, 10 + 1, ofGetHeight() - _fnt.stringHeight(txt) - 1 + 1);
        ofSetHexColor(0xFFFFFF);
        _fnt.drawString(txt, 10, ofGetHeight() - _fnt.stringHeight(txt) - 1);
    }
}

void MusicScene::_updateMusicCanvas()
{
    // update cursor
    float speed = TEMPO_BASE + TEMPO_MULTIPLIER * _tempo;
    _oldcursor = _cursor;
    _cursor += speed * ofGetLastFrameTime();
    
    // reset music tokens
    _tokens.clear();

    // touch points
    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );
    for(TouchTracker::TrackedTouchPoint& q : v) {
        const TouchDetector::TouchPoint* p = q(0);
        int soundId = ([](std::string type) -> int {
            if(type == "marker_blue")
                return 0;
            else if(type == "marker_yellow")
                return 1;
            else if(type == "marker_green")
                return 2;
            else if(type == "marker_magenta")
                return 3;
            else
                return -1;
        })(_namefix(p->type));

        if(soundId >= 0) {
            auto stabilize = [](float x, float prec) -> float { return std::floor(x * prec) / prec + 0.5f / prec; };
            Token t = { p->_id, stabilize(p->surfaceX, COMPASS_NUM_SAMPLES), p->surfaceY, p->surfaceX, p->surfaceY, soundId, false };
            _tokens.push_back(t);
        }
    }
    std::sort(_tokens.begin(), _tokens.end(), [](const Token& a, const Token& b) -> bool { return a.x < b.x; });

    // play sounds
    for(Token& t : _tokens) {
        bool collision = 
            (speed >= 0.0f && _cursor >= t.x && _oldcursor <= t.x) ||
            (speed < 0.0f && _cursor <= t.x && _oldcursor >= t.x);
        if(collision && _playedTokens.count(t.id) == 0) {
            _playedTokens.insert(t.id);
            t.isActive = true;
            int sfxId = t.type * 10 + int(3.0f * t.y);
            //std::cout << "s" << sfxId << "@" << (ofGetElapsedTimef() - _startTime) << std::endl;
            playSoundEffect(sfxId);
        }
    }

    // wrap cursor
    if((_cursor > 1.0f && speed >= 0.0f) || (_cursor < 0.0f && speed < 0.0f)) {
        _cursor = (speed > 0.0f) ? 0.0f : 1.0f;
        _oldcursor -= (speed > 0.0f) ? 0.0f : 1.0f;
        _playedTokens.clear();
    }
}









void MusicScene::spawnCircleFxAt(int x, int y)
{
    CircleFx c(x, y);
    _circleFxs.push_back(c);
}

MusicScene::CircleFx::CircleFx(int x, int y) : center(x, y), timeToLive(LIFESPAN)
{
    ;
}

void MusicScene::CircleFx::update()
{
    timeToLive -= ofGetLastFrameTime();
}

void MusicScene::CircleFx::draw(bool isUsingAProjector)
{
    float factor = isUsingAProjector ? 3.0f : 1.0f;
    float normalizedLifetime = std::max(0.0f, std::min(1.0f, 1.0f - timeToLive / LIFESPAN));
    float R = MAXRADIUS * factor * normalizedLifetime;
    float r = (MAXRADIUS - THICKNESS) * factor * normalizedLifetime;
    float alpha = 255 * (1.0f - normalizedLifetime);

    ofPath curve;
    ofPoint p(center);
    curve.arc(p, R, R, 0, 360);
    curve.arcNegative(p, r, r, 0, 360);
    curve.close();
    curve.setArcResolution(60);
    curve.setFillColor(ofColor(255, 255, 255, alpha));
    curve.setFilled(true);
    curve.draw();
}







void MusicScene::spawnParticleAt(int x, int y, ofColor c, float preferredAngle, float randomAngleParticle)
{
    Particle p(x, y, c, preferredAngle, randomAngleParticle);
    _particles.push_back(p);
}

MusicScene::Particle::Particle(int x, int y, ofColor c, float preferredAngle, float randomAngleAmplitude) : position(x, y), timeToLive(LIFESPAN), angle(0.0f), color(c)
{
    float amplitude = randomAngleAmplitude;
    int factor = amplitude > 0 ? (rand() % 2) : 0;
    speed = amplitude > 0 ? SPEED : 0;
    angle = (preferredAngle + ((factor * 180) + ofRandom(-amplitude, amplitude))) * M_PI / 180.0f;
}

void MusicScene::Particle::update()
{
    float dt = ofGetLastFrameTime();
    timeToLive -= dt;
    position += (ofVec2f(1, 0).getRotatedRad(angle) * speed) * dt;
}

void MusicScene::Particle::draw(bool isUsingAProjector)
{
    float factor = !isUsingAProjector ? 1.0f : 2.5f;
    float normalizedLifetime = std::max(0.0f, std::min(1.0f, 1.0f - timeToLive / LIFESPAN));
    float alpha = 255 * (1.0f - normalizedLifetime);
    ofSetColor(color.r, color.g, color.b, alpha);
    ofCircle(position.x, position.y, RADIUS * factor);
}






















void MusicScene::keyPressed(int key)
{
    if(key == OF_KEY_RETURN) {
        _touchDetector->save();
        application()->setScene(
            new TouchDetectionScene(application())
        );
    }
    else if(key == 'p' || key == ' ') {
        _isUsingAProjector = !_isUsingAProjector;
        _circleFxs.clear();
    }
    else if(key == 'd')
        _debug = !_debug;
}




void MusicScene::playSoundEffect(int id)
{
    if(_sfx.count(id) > 0)
        _sfx[id]->play();
    else
        std::cerr << "can't play sfx #" << id << "." << std::endl;
}

void MusicScene::_loadSoundEffects()
{
    for(int i=0; i<100; i++) {
        std::stringstream ss;
        ss << "sfx/" << i << ".wav";
        if(ofFile::doesFileExist(ss.str())) {
            ofSoundPlayer* s = new ofSoundPlayer();
            s->loadSound(ss.str());
            s->setMultiPlay(false);
            s->setVolume(0.9 / MAX_PLAYING_SAMPLES);
            _sfx[i] = s;
        }
    }
}

// gambiarra doida
std::string MusicScene::_namefix(std::string s) const
{
    if(s == "projetor_finger")
        return "marker_yellow";
    if(s.substr(0, 9) == "projetor_")
        return s.substr(9);
    else if(s == "eraser_green")
        return "marker_green";
    else if(s == "finger")
        return "marker_yellow";
    else if(s == "null")
        return "marker_magenta";
    else
        return s;
}
