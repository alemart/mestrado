#ifndef _PLAY_H
#define _PLAY_H

#include <vector>
#include <string>
#include "ofMain.h"
#include "Scene.h"
class TouchDetector;
class TouchTracker;

// paint2 scene
class PlayScene : public Scene
{
public:
    PlayScene(MyApplication* app);
    virtual ~PlayScene();

    virtual void update();
    virtual void draw();

    virtual void keyPressed(int key);
    virtual void mousePressed(int x, int y, int button);
    virtual void mouseDragged(int x, int y, int button);

    void spawnSegmentAt(int currX, int currY, int prevX, int prevY, std::string colorName);
    void spawnParticleAt(int x, int y, ofColor c, float preferredAngle, float randomAngleParticle = 10.0f);
    void spawnCircleFxAt(int x, int y);

    int numberOfSoundEffects();
    void playSoundEffect(int id);
    void drawScreenDivisions();

private:
    TouchDetector* _touchDetector;
    TouchTracker* _touchTracker;
    bool _debug;
    bool _mouseJustPressed;
    ofTrueTypeFont _fnt;

    // segments
    ofVec2f _cameraPosition;
    struct Segment
    {
        Segment(PlayScene* parent, float curx, float cury, float prevx, float prevy, std::string colorName);
        void update(PlayScene* parent, int precision);
        void draw(ofVec2f cameraPosition);

        float currX, currY, prevX, prevY;
        float timeToLive;
        ofColor color;
        static constexpr float LIFESPAN = 1.0f; // in seconds
    };
    std::vector<Segment> _segments;

    // circle effect
    struct CircleFx
    {
        CircleFx(int x, int y);
        void update();
        void draw();

        ofVec2f center;
        float timeToLive;
        static constexpr float LIFESPAN = 1.0f; // in seconds
        static constexpr float MAXRADIUS = 150.0f; // in pixels
        static constexpr float THICKNESS = 20.0f;
    };
    std::vector<CircleFx> _circleFxs;
    unsigned _lastTouchId;

    // particle
    struct Particle
    {
        Particle(int x, int y, ofColor c, float preferredAngle, float randomAngleAmplitude);
        void update();
        void draw();

        ofVec2f position;
        float timeToLive;
        float angle; // 0 <= angle < 2pi
        ofColor color;
        float speed;

        static constexpr float LIFESPAN = 2.0f; // in seconds
        static constexpr float SPEED = 50.0f; // in px/s
        static constexpr float RADIUS = 5; // in pixels
    };
    std::vector<Particle> _particles;

    // sound effects
    void _loadSoundEffects();
    std::vector<ofSoundPlayer*> _sfx;

    // border entity
    struct BorderEntity
    {
        enum State { STATE_RIGHT, STATE_DOWN, STATE_LEFT, STATE_UP };

        BorderEntity(PlayScene* parnt, int x, int y, State s, ofColor c);
        void update();
        void draw();

        PlayScene* parent;
        ofVec2f position;
        State state;
        ofColor color;
        float timer;

        static constexpr int BORDER = 5; // in pixels
        static constexpr float FREQUENCY = 10.0f; // in s^(-1)
        static constexpr float SPEED = 400.0f; // px/s
    };
    std::vector<BorderEntity> _borderEntities;
};

#endif
