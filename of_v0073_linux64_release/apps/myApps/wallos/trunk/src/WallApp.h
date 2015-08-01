#ifndef _WALLAPP_H
#define _WALLAPP_H

#include <string>
#include <list>
#include "TuioClient.h"
#include "TuioObject.h"
#include "TuioCursor.h"
#include "TuioTime.h"
class WallOS;

class WallApp
{
public:
    WallApp(WallOS* system, std::string name) : _system(system), _name(name) { }
    virtual ~WallApp() { }

    virtual void setup() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;

    inline WallOS* system() const { return _system; }
    inline std::string name() const { return _name; }
    inline const std::list<TUIO::TuioObject*>& touchPoints() const { return _touchPoints; }

    struct { enum TouchType_ { NONE = 0, FINGER, MARKER_YELLOW, MARKER_BLUE, MARKER_GREEN, MARKER_MAGENTA, ERASER }; } TouchType;
    inline void updateTouchPoints(const std::list<TUIO::TuioObject*>& touchPoints) { _touchPoints = _filter(touchPoints); }

private:
    WallOS* _system;
    std::string _name;
    std::list<TUIO::TuioObject*> _touchPoints;
    std::list<TUIO::TuioObject*> _filter(const std::list<TUIO::TuioObject*>& l) const;
};

#endif
