#ifndef _TOUCHTRACKER_H
#define _TOUCHTRACKER_H

#include <map>
#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"
#include "TouchDetector.h"

class TouchTracker
{
public:
    class TrackedTouchPoint
    {
    public:
        // you may call (*this)(0), (*this)(-1), ... (*this)(-9), (*this)(-10)
        // we ensure you'll be getting current/previous data of the same touch point
        inline const TouchDetector::TouchPoint* operator()(int k) const { return _findTouchPoint(k); }

        // compares two TrackedTouchPoints
        bool operator<(const TrackedTouchPoint& p) const { return _myId < p._myId; }
        bool operator==(const TrackedTouchPoint& p) const { return _myId == p._myId; }

    private:
        TouchTracker* _tracker;
        unsigned int _myId;
        TouchDetector::TouchPoint nil;

        inline const TouchDetector::TouchPoint* _findTouchPoint(int k) const {
            const TouchDetector::TouchPoint* p = _tracker->_findTouchPoint(_myId, k);
            return p ? p : &nil;
        }

        TrackedTouchPoint(TouchTracker* tracker, unsigned int myId) : _tracker(tracker), _myId(myId)
        {
            nil.x = nil.y = nil.area = 0;
            nil.surfaceX = nil.surfaceY = 0;
            nil.type = "null";
        }

        friend class TouchTracker;
    };

    TouchTracker(int storedSamples = 60, int stopColorClassificationAfterSamples = 30);
    ~TouchTracker();

    inline int storedSamples() const { return _storedSamples; }

    void feed(std::vector<TouchDetector::TouchPoint> touchPoints, float searchRadius, float dt); // call once per framestep
    std::vector<TrackedTouchPoint> trackedTouchPoints();

private:
    int _storedSamples;
    int _stopColorClassificationAfterSamples; // reaproveita classificacoes anteriores
    unsigned int _newId;
    std::map<unsigned int, std::vector<TouchDetector::TouchPoint> > _trackedPoints; // (id, history of touch points)

    const TouchDetector::TouchPoint* _findTouchPoint(unsigned int id, int k) const;

    friend class TrackedTouchPoint;
};

#endif
