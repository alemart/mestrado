#ifndef _TUIO_H
#define _TUIO_H

#include <string>
#include <set>
#include <map>
#include "TuioServer.h"
#include "TuioObject.h"
#include "TuioCursor.h"
#include "TuioTime.h"
#include "ofMain.h"
#include "Scene.h"
#include "../core/TouchTracker.h"
#include "../core/EraserTracker.h"
#include "../core/WandTracker.h"
class TouchDetector;
class EraserDetector;
class Marker3DTracker;
class ImageLabeler;

// tuio scene
class TUIOScene : public Scene
{
public:
    TUIOScene(MyApplication* app);
    virtual ~TUIOScene();

    virtual void update();
    virtual void draw();
    virtual void keyPressed(int key);

private: 
    TUIO::TuioServer* _tuioServer;
    TUIO::TuioTime _tuioCurrentTime;
    std::map<unsigned int, TUIO::TuioObject*> _tuioObjects;
    std::map<unsigned int, TUIO::TuioObject*> _tuioErasers;
    std::map<unsigned int, TUIO::TuioObject*> _tuioWands;
    std::string _tuioHost;
    int _tuioPort;



    TouchDetector* _touchDetector;
    TouchTracker* _touchTracker;

    EraserDetector* _eraserDetector;
    EraserTracker* _eraserTracker;

    ImageLabeler* _imageLabeler;
    WandTracker* _wandTracker;

    ImageLabeler* _bottomImageLabeler;
    WandTracker* _bottomWandTracker;

    Marker3DTracker* _3dtracker;
    std::set<int> _alreadyTracked;
    unsigned _3dtrackerTarget;

    float _startTime;
    std::vector<TouchTracker::TrackedTouchPoint> _trackedTouchPoints;
    std::vector<EraserTracker::TrackedEraser> _trackedErasers;
    std::vector<WandTracker::Wand> _trackedWands;
    ofTrueTypeFont _fnt, _fnt2, _fnt3;
    bool _showWandData, _showTouchData;

    void _shareStuff();
    enum TouchType { NONE = 0, FINGER, MARKER_YELLOW, MARKER_BLUE, MARKER_GREEN, MARKER_MAGENTA, ERASER, WAND_YELLOW, WAND_YELLOW_ACTIVE, WAND_BLUE };
    TouchType string2touchType(std::string str) const;

    void _saveParams(std::string filepath);
    void _loadParams(std::string filepath);
};

#endif
