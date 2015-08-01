#include <vector>
#include "Play.h"
#include "TouchDetection.h"
#include "../core/MyApplication.h"
#include "../core/CalibratedKinect.h"
#include "../core/BackgroundModel.h"
#include "../core/TouchDetector.h"
#include "../core/TouchTracker.h"


//#define HORIZONTAL
#define PARTICLES_PER_STEP 5


PlayScene::PlayScene(MyApplication* app) : Scene(app), _debug(false), _mouseJustPressed(false)
{
    ofSetBackgroundColor(0, 0, 0);
    //_touchDetector = new TouchDetector(app->kinect());
    _touchDetector = new TouchDetector(app->kinect(), TouchClassifier::QUALITY_FAST);
    _touchTracker = new TouchTracker(60, 6);
    _lastTouchId = 0;
    ofSeedRandom();
    _loadSoundEffects();
    _fnt.loadFont("DejaVuSans.ttf", 24);

    _borderEntities.push_back( BorderEntity(this, 0, 0, BorderEntity::STATE_RIGHT, ofColor::fromHex(0xFF0099)) );
    _borderEntities.push_back( BorderEntity(this, ofGetWidth(), 0, BorderEntity::STATE_DOWN, ofColor::fromHex(0x44AAFF)) );
    _borderEntities.push_back( BorderEntity(this, ofGetWidth(), ofGetHeight(), BorderEntity::STATE_LEFT, ofColor::fromHex(0x33FF33)) );
    _borderEntities.push_back( BorderEntity(this, 0, ofGetHeight(), BorderEntity::STATE_UP, ofColor::fromHex(0xFFFF33)) );
    ofHideCursor();
}

PlayScene::~PlayScene()
{
    delete _touchTracker;
    delete _touchDetector;

    for(int i=0; i<int(_sfx.size()); i++) {
        _sfx[i]->unloadSound();
        //delete _sfx[i]; // crash??
    }

    ofShowCursor();
}

void PlayScene::update()
{
    bool gotAnyNonFinger = false;

    // multitouch
    _touchDetector->update();
    _touchTracker->feed( _touchDetector->touchPoints(), 25, ofGetLastFrameTime() );
    std::vector<TouchTracker::TrackedTouchPoint> v( _touchTracker->trackedTouchPoints() );

    // got any finger?
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end() && !gotAnyNonFinger; ++it) {
        const TouchDetector::TouchPoint* p0 = (*it)(0);
        if(p0->type != "null" && p0->type != "finger" && p0->type != "projetor_finger")
            gotAnyNonFinger = true;
    }

    // drawing routines
    for(std::vector<TouchTracker::TrackedTouchPoint>::const_iterator it = v.begin(); it != v.end(); ++it) {
        const TouchDetector::TouchPoint* p0 = (*it)(0);
        const TouchDetector::TouchPoint* p1 = (*it)(-1);

        float x0 = p0->surfaceX * ofGetWidth();
        float y0 = p0->surfaceY * ofGetHeight();
        float x1 = p1->surfaceX * ofGetWidth();
        float y1 = p1->surfaceY * ofGetHeight();
        std::string type = p0->type;

        float cx = _cameraPosition.x;
        float cy = _cameraPosition.y;

        if(type != "null" && type != "finger" && type != "projetor_finger") {
            spawnSegmentAt(x0 + cx, y0 + cy, x1 + cx, y1 + cy, type);
        }
        else if(!gotAnyNonFinger) {
            if(p0->_id > _lastTouchId) {
                _lastTouchId = p0->_id;
                std::cout << "touch (finger): " << _lastTouchId << std::endl;
                //spawnCircleFxAt(x0, y0);
            }
        }
    }

    // paths
    std::vector<Segment> sg;
    for(std::vector<Segment>::iterator it = _segments.begin(); it != _segments.end(); ++it) {
        it->update(this, PARTICLES_PER_STEP);
        if(it->timeToLive > 0.0f)
            sg.push_back(*it);
    }
    _segments = sg;

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

    // border entities
    for(std::vector<BorderEntity>::iterator it = _borderEntities.begin(); it != _borderEntities.end(); ++it)
        it->update();
}

void PlayScene::draw()
{
    // background
    ofSetBackgroundColor(0, 0, 0);
    ofSetHexColor(0x0);
    ofRect(0, 0, ofGetWidth(), ofGetHeight());
    drawScreenDivisions();

    // multitouch
    std::string txt = " ";//"multi-toque";
    _fnt.drawString(txt, (ofGetWidth() - _fnt.stringWidth(txt))/2, (ofGetHeight() - _fnt.stringHeight(txt))/2);

    // segments
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    for(std::vector<Segment>::iterator it = _segments.begin(); it != _segments.end(); ++it)
        it->draw(_cameraPosition);

    // particles
    for(std::vector<Particle>::iterator it = _particles.begin(); it != _particles.end(); ++it)
        it->draw();
    ofDisableBlendMode();

    // circle effects
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    for(std::vector<CircleFx>::iterator it = _circleFxs.begin(); it != _circleFxs.end(); ++it)
        it->draw();
    ofDisableBlendMode();

    // border entities
    for(std::vector<BorderEntity>::iterator it = _borderEntities.begin(); it != _borderEntities.end(); ++it)
        it->draw();

    // debug
    if(_debug) {
        ofSetHexColor(0xFFFFFF);
        ofRect(ofGetWidth()*5/6 - 1, ofGetHeight()*5/6 - 1, ofGetWidth()*5/6 + 2, ofGetHeight()*5/6 + 2);
        _touchDetector->blobs()->draw(ofGetWidth()*5/6, ofGetHeight()*5/6, ofGetWidth()/6, ofGetHeight()/6);
    }
}








void PlayScene::spawnSegmentAt(int currX, int currY, int prevX, int prevY, std::string colorName)
{
    Segment s(this, currX, currY, prevX, prevY, colorName);
    _segments.push_back(s);
}

PlayScene::Segment::Segment(PlayScene* parent, float curx, float cury, float prevx, float prevy, std::string colorName) : currX(curx), currY(cury), prevX(prevx), prevY(prevy), timeToLive(LIFESPAN)
{
    std::string type = colorName;
    if(type.substr(0, 9) == "projetor_")
        type = type.substr(9);

    if(type == "marker_violet")
        color = ofColor::fromHex(0x330099);
    else if(type == "marker_blue")
        color = ofColor::fromHex(0x44AAFF); //0x2266FF);
    else if(type == "marker_green" || type == "eraser_green")
        color = ofColor::fromHex(0x33FF33);
    else if(type == "marker_yellow")
        color = ofColor::fromHex(0xFFFF77);
    else if(type == "marker_red")
        color = ofColor::fromHex(0xFF3300);
    else if(type == "marker_magenta")
        color = ofColor::fromHex(0xFF0099);
    else
        color = ofColor::fromHex(0xFFFFFF);

    int precision = PARTICLES_PER_STEP;
    ofVec2f v(currX - prevX, currY - prevY);
    for(int i=0; i<=precision; i++) {
        float frac = float(i) / float(precision);
        ofVec2f p = ofVec2f(prevX, prevY) + frac * v;
        parent->spawnParticleAt(p.x, p.y, color, atan2(v.y, v.x) * 180.0f / M_PI);
    }
}

void PlayScene::Segment::update(PlayScene* parent, int precision)
{
    timeToLive -= ofGetLastFrameTime();
/*
    static int k = 0;
    if(k++ % 5 != 0)
        return;

    ofVec2f v(currX - prevX, currY - prevY);
    for(int i=0; i<=precision; i++) {
        float frac = float(i) / float(precision);
        ofVec2f p = ofVec2f(prevX, prevY) + frac * v;
        //parent->spawnParticleAt(p.x, p.y, color, atan2(v.y, v.x));
    }
*/
}

void PlayScene::Segment::draw(ofVec2f cameraPosition)
{
    float normalizedLifetime = std::max(0.0f, std::min(1.0f, 1.0f - timeToLive / LIFESPAN));
    float alpha = 255 * (1.0f - normalizedLifetime);

    float cx = cameraPosition.x;
    float cy = cameraPosition.y;

    float x0 = currX - cx;
    float y0 = currY - cy;
    float x1 = prevX - cx;
    float y1 = prevY - cy;

    ofSetColor(color.r, color.g, color.b, alpha);
    ofSetLineWidth(5.0f);
    ofLine(x1, y1, x0, y0);
}








void PlayScene::drawScreenDivisions()
{
    const int BORDER = 2;
    static float delta = 0.0f;
    delta += 30.0f * ofGetLastFrameTime();

    int numberOfSteps = 100;
    int n = numberOfSoundEffects();

    ofSetHexColor(0xFFFFFF);
    //ofRect(0, 0, ofGetWidth()-1, ofGetHeight()-1);
    ofSetHexColor(0x0);
    ofRect(BORDER, BORDER, ofGetWidth()-1-BORDER*2, ofGetHeight()-1-BORDER*2);
    ofSetHexColor(0xFFFFFF);
return;
    ofSetLineWidth(BORDER);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    for(int j=1; j<n; j++) {
#ifdef HORIZONTAL
        ofVec2f start(j * ofGetWidth() / n, delta);
        ofVec2f finish(start.x, start.y + ofGetHeight());
        ofVec2f direction = (finish - start).getNormalized();
#else
        ofVec2f start(delta, j * ofGetHeight() / n);
        ofVec2f finish(start.x + ofGetWidth(), start.y);
        ofVec2f direction = (finish - start).getNormalized();
#endif

        float len = (finish - start).length();
        if(delta > 2*len/numberOfSteps)
            delta -= 2*len/numberOfSteps;
        for(int i=0; i<numberOfSteps; i++) {
            if(i % 2 == 0) {
                int x1 = start.x + len * (float(i) / numberOfSteps) * direction.x;
                int y1 = start.y + len * (float(i) / numberOfSteps) * direction.y;
                int x2 = start.x + len * (float(1+i) / numberOfSteps) * direction.x;
                int y2 = start.y + len * (float(1+i) / numberOfSteps) * direction.y;
                x1 %= ofGetWidth();
                y1 %= ofGetHeight();
                x2 %= ofGetWidth();
                y2 %= ofGetHeight();
                if(x1 <= x2 && y1 <= y2) {
                    float x = float(2*i) / numberOfSteps - 1.0f;
                    float alpha = 255.0f * (1.0f - x*x);
                    ofSetColor(255, 255, 255, alpha);
                    ofLine(x1, y1, x2, y2);
                }
            }
        }
    }
    ofDisableBlendMode();
}

void PlayScene::spawnCircleFxAt(int x, int y)
{
    CircleFx c(x, y);
    _circleFxs.push_back(c);
    
#ifdef HORIZONTAL
    float coord = x;
    float len = ofGetWidth();
#else
    float coord = y;
    float len = ofGetHeight();
#endif

    float frac = len / numberOfSoundEffects();
    for(int k=0; k<numberOfSoundEffects(); k++) {
        if(coord >= k * frac && coord < (k+1) * frac)
            playSoundEffect(k);
    }
}

PlayScene::CircleFx::CircleFx(int x, int y) : center(x, y), timeToLive(LIFESPAN)
{
    ;
}

void PlayScene::CircleFx::update()
{
    timeToLive -= ofGetLastFrameTime();
}

void PlayScene::CircleFx::draw()
{
    float normalizedLifetime = std::max(0.0f, std::min(1.0f, 1.0f - timeToLive / LIFESPAN));
    float R = MAXRADIUS * normalizedLifetime;
    float r = (MAXRADIUS - THICKNESS) * normalizedLifetime;
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







void PlayScene::spawnParticleAt(int x, int y, ofColor c, float preferredAngle, float randomAngleParticle)
{
    Particle p(x, y, c, preferredAngle, randomAngleParticle);
    _particles.push_back(p);
}

PlayScene::Particle::Particle(int x, int y, ofColor c, float preferredAngle, float randomAngleAmplitude) : position(x, y), timeToLive(LIFESPAN), angle(0.0f), color(c)
{
    float amplitude = randomAngleAmplitude;
    int factor = amplitude > 0 ? (rand() % 2) : 0;
    speed = amplitude > 0 ? SPEED : 0;
    angle = (preferredAngle + ((factor * 180) + ofRandom(-amplitude, amplitude))) * M_PI / 180.0f;
}

void PlayScene::Particle::update()
{
    float dt = ofGetLastFrameTime();
    timeToLive -= dt;
    position += (ofVec2f(1, 0).getRotatedRad(angle) * speed) * dt;
}

void PlayScene::Particle::draw()
{
    float normalizedLifetime = std::max(0.0f, std::min(1.0f, 1.0f - timeToLive / LIFESPAN));
    float alpha = 255 * (1.0f - normalizedLifetime);
    ofSetColor(color.r, color.g, color.b, alpha);
    ofCircle(position.x, position.y, RADIUS);
}






















void PlayScene::keyPressed(int key)
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

void PlayScene::mousePressed(int x, int y, int button)
{
    if(button == 0) {
        spawnCircleFxAt(x, y);
    }

    _mouseJustPressed = true;
}

void PlayScene::mouseDragged(int x, int y, int button)
{
    static int ox = x;
    static int oy = y;

    if(button == 2) {
        if(!_mouseJustPressed)
            spawnSegmentAt(x, y, ox, oy, "marker_yellow");
        ox = x;
        oy = y;
    }

    _mouseJustPressed = false;
}







int PlayScene::numberOfSoundEffects()
{
    return _sfx.size();
}

void PlayScene::playSoundEffect(int id)
{
    if(id >= 0 && id < numberOfSoundEffects())
        _sfx[id]->play();
    else
        std::cerr << "can't play sfx " << id << "." << std::endl;
}

void PlayScene::_loadSoundEffects()
{
    for(int i=0; ; i++) {
        std::stringstream ss;
        ss << "sfx/" << i << ".ogg";
        if(ofFile::doesFileExist(ss.str())) {
            ofSoundPlayer* s = new ofSoundPlayer();
            s->loadSound(ss.str());
            s->setMultiPlay(true);
            _sfx.push_back(s);
        }
        else
            break;
    }
}








PlayScene::BorderEntity::BorderEntity(PlayScene* parnt, int x, int y, PlayScene::BorderEntity::State s, ofColor c) : parent(parnt), position(x, y), state(s), color(c), timer(0.0f)
{
    ;
}

void PlayScene::BorderEntity::update()
{
    float dt = ofGetLastFrameTime();

    // move
    switch(state) {
    case STATE_RIGHT:
        position.x += SPEED * dt;
        if(position.x >= ofGetWidth() - BORDER)
            state = STATE_DOWN;
        break;
    case STATE_DOWN:
        position.y += SPEED * dt;
        if(position.y >= ofGetHeight() - BORDER)
            state = STATE_LEFT;
        break;
    case STATE_LEFT:
        position.x -= SPEED * dt;
        if(position.x <= BORDER)
            state = STATE_UP;
        break;
    case STATE_UP:
        position.y -= SPEED * dt;
        if(position.y <= BORDER)
            state = STATE_RIGHT;
        break;
    }

    // reposition
    if(position.x >= ofGetWidth() - BORDER)
        position.x = ofGetWidth() - BORDER;
    if(position.y >= ofGetHeight() - BORDER)
        position.y = ofGetHeight() - BORDER;
    if(position.x <= BORDER)
        position.x = BORDER;
    if(position.y <= BORDER)
        position.y = BORDER;

    // spawn particles
    if((timer -= dt) < 0.0f) {
        float angle = 0.0f;
        switch(state) {
        case STATE_RIGHT: angle = 0; break;
        case STATE_UP: angle = 90; break;
        case STATE_LEFT: angle = 180; break;
        case STATE_DOWN: angle = 270; break;
        }
        parent->spawnParticleAt(position.x, position.y, color, angle, 0.0f);
        timer += 1.0f / FREQUENCY;
    }
}

void PlayScene::BorderEntity::draw()
{
//    ofSetColor(color);
//    ofCircle(position.x, position.y, 15);
}
