#ifndef _WALLOS_H
#define _WALLOS_H

#include <list>
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "TuioClient.h"
#include "TuioObject.h"
#include "TuioCursor.h"
#include "TuioTime.h"

class WallApp;

class WallOS : public ofBaseApp
{
public:
    WallOS(int tuioPort);
    ~WallOS();

    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);

    ofVec2f norm2win(ofVec2f pointInNormalizedCoordinates); // convert from [0,1]^2 to window coordinates
    inline WallApp* currentApp() const { return _currentApp; }

private:
    // TUIO stuff
    int _tuioPort;
    TUIO::TuioClient* _tuioClient;
    std::list<TUIO::TuioObject*> _tuioObjects;

    // Application
    WallApp* _currentApp;

    // App Menu

    // Sweet Spot
    class SweetSpot {
    public:
        SweetSpot();
        ~SweetSpot();

        void draw();
        void keyPressed(int key);
        ofVec2f convertToWindowCoordinates(ofVec2f pointInSweetSpotCoordinates); // given: a point in [0,1]^2; returns: the corresponding pixel
        bool isBeingManipulated();

    private:
        ofPoint _corner[4]; // topleft, topright, bottomright, bottomleft in [0,1]^2
        int _activeCorner; // 0, 1, 2, 3 or 4 (none)
        CvMat* _sweetspot2screen; // 3x3 (homography matrix)

        void _loadFromDisk();
        void _saveToDisk();
        void _updateHomography();

        static const char* _filepath;
        ofTrueTypeFont _fnt;
    } _sweetspot;
};

#endif
