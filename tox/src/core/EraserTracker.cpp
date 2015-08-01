#include <algorithm>
#include "EraserTracker.h"
#include "EraserDetector.h"

// number of framesteps an eraser can resist
// given that it is not being received by the sensor
#define INITIAL_TIMETOLIVE 3//6

// number of framesteps a new eraser point must
// present itself before it's tracked
#define INITIAL_TIMETOBEBORN 2//3//0



EraserTracker::EraserTracker(int storedSamples) : _storedSamples(storedSamples), _newId(0), _historySize(0)
{
    _history = new EraserDetector::Eraser[storedSamples];
}

EraserTracker::~EraserTracker()
{
    delete[] _history;
}

void EraserTracker::feed(std::vector<EraserDetector::Eraser> objects, float searchRadius, float dt)
{
    // grab only 1 eraser
    if(objects.size() > 1)
        objects.resize(1);
    else if(objects.size() < 1) {
        if(_historySize > 0 && (_history[ _historySize-1 ]._timeToLive)-- <= 0)
            _historySize = 0;
        return;
    }

    // configure a new point
    EraserDetector::Eraser& e = objects[0];
    e.startX = e.x;
    e.startY = e.y;
    e.surfaceStartX = e.surfaceX;
    e.surfaceStartY = e.surfaceY;
    e.speedX = e.speedY = 0.0f;
    e.surfaceSpeedX = e.surfaceSpeedY = 0.0f;
    e._timeToLive = INITIAL_TIMETOLIVE;

    // greedy matching
    if(_historySize > 0) {
        EraserDetector::Eraser& last = _history[ _historySize-1 ];
        ofVec2f prevLoc(last.x, last.y);
        ofVec2f prevSpeed(last.speedX, last.speedY);
        ofVec2f predictedPosition = prevLoc + (prevSpeed * dt);
        ofVec2f curLoc(e.x, e.y);

        if(curLoc.distance(predictedPosition) <= searchRadius) {
            e._id = last._id;
            e.startX = last.startX;
            e.startY = last.startY;
            e.surfaceStartX = last.surfaceStartX;
            e.surfaceStartY = last.surfaceStartY;
            e.speedX = (float)(e.x - last.x) / dt;
            e.speedY = (float)(e.y - last.y) / dt;
            e.surfaceSpeedX = (float)(e.surfaceX - last.surfaceX) / dt;
            e.surfaceSpeedY = (float)(e.surfaceY - last.surfaceY) / dt;
        }
        else {
            _historySize = 0;
            e._id = _newId++;
        }
    }
    else
        e._id = _newId++;

    // add the eraser to the history
    if(_historySize == _storedSamples) {
        for(int i=1; i<_storedSamples; i++)
            _history[i-1] = _history[i];
        _history[_storedSamples-1] = e;
    }
    else if(_historySize < _storedSamples)
        _history[_historySize++] = e;

    // forget the last eraser...!
    if( (_history[ _historySize-1 ]._timeToLive)-- <= 0 )
        _historySize = 0;
}

const EraserDetector::Eraser* EraserTracker::_findEraser(unsigned int id, int k) const
{
    int size = _historySize;

    if(size > 0) {
        int j = (size - 1) + k;
        if(j < 0) j = 0;
        if(j >= size) j = size - 1;

        if(_history[j]._id == id)
            return &(_history[j]);
    }

    return NULL;
}

std::vector<EraserTracker::TrackedEraser> EraserTracker::trackedErasers()
{
    std::vector<TrackedEraser> v;

    if(_historySize >= _storedSamples || _historySize >= INITIAL_TIMETOBEBORN) {
        TrackedEraser e(this, _history[ _historySize-1 ]._id);
        v.push_back(e);
    }

    return v;
}
