#ifndef _MUSIC_H
#define _MUSIC_H

#include <vector>
#include <string>
#include <set>
#include "ofMain.h"
#include "Scene.h"
class TouchDetector;
class TouchTracker;

class MusicScene : public Scene
{
public:
    MusicScene(MyApplication* app);
    virtual ~MusicScene();

    virtual void update();
    virtual void draw();

    virtual void keyPressed(int key);

    void spawnSegmentAt(int currX, int currY, int prevX, int prevY, std::string colorName);
    void spawnParticleAt(int x, int y, ofColor c, float preferredAngle, float randomAngleParticle = 10.0f);
    void spawnCircleFxAt(int x, int y);

    void playSoundEffect(int id);

private:
    TouchDetector* _touchDetector;
    TouchTracker* _touchTracker;

    ofTrueTypeFont _fnt, _fnt2;
    bool _debug;
    bool _mouseJustPressed;
    std::set<unsigned> _alreadyTracked;
    unsigned _prevFingerId; // for wand gesture

    bool _isUsingAProjector;

    // circle effect
    struct CircleFx
    {
        CircleFx(int x, int y);
        void update();
        void draw(bool isUsingAProjector = false);

        ofVec2f center;
        float timeToLive;
        static constexpr float LIFESPAN = 0.5f; // in seconds
        static constexpr float MAXRADIUS = 45.0f; // in pixels
        static constexpr float THICKNESS = 5.0f;
    };
    std::vector<CircleFx> _circleFxs;

    // particle
    struct Particle
    {
        Particle(int x, int y, ofColor c, float preferredAngle, float randomAngleAmplitude);
        void update();
        void draw(bool isUsingAProjector = false);

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
    std::map<int,ofSoundPlayer*> _sfx;

    // misc
    float _tempo, _cursor, _oldcursor;
    float _startTime;
    void _updateMusicCanvas();
    void _drawMusicCanvas(int x, int y, int width, int height, int discRadius, float cursor); // 0 <= cursor <= 1
    void _regularDraw();
    void _projectorDraw();
    
    struct Token {
        unsigned id;
        float x, y; // in [0,1]
        float _x, _y; // in [0,1], untouched (not fixed/filtered)
        int type; // 0, 1, 2, ...
        bool isActive;
    };

    std::vector<Token> _tokens;
    std::set<unsigned> _playedTokens;
    std::string _namefix(std::string s) const;
};

#endif
