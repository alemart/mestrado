#ifndef _ERASERTRACKER_H
#define _ERASERTRACKER_H

#include <map>
#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"
#include "EraserDetector.h"

class EraserTracker
{
public:
    class TrackedEraser
    {
    public:
        // you may call (*this)(0), (*this)(-1), ... (*this)(-9), (*this)(-10)
        // we ensure you'll be getting current/previous data of the same eraser
        inline const EraserDetector::Eraser* operator()(int k) const { return _findEraser(k); }

    private:
        EraserTracker* _tracker;
        unsigned int _myId;
        EraserDetector::Eraser nil;

        inline const EraserDetector::Eraser* _findEraser(int k) const {
            const EraserDetector::Eraser* p = _tracker->_findEraser(_myId, k);
            return p ? p : &nil;
        }

        TrackedEraser(EraserTracker* tracker, unsigned int myId) : _tracker(tracker), _myId(myId)
        {
            nil.x = nil.y = nil.area = 0;
            nil.surfaceX = nil.surfaceY = 0;
            nil.coefficient = 0.0f;
        }

        friend class EraserTracker;
    };

    EraserTracker(int storedSamples = 60);
    ~EraserTracker();

    inline int storedSamples() const { return _storedSamples; }

    void feed(std::vector<EraserDetector::Eraser> objects, float searchRadius, float dt); // call once per framestep
                                                                                          // FIXME: works only for 1 eraser
    std::vector<TrackedEraser> trackedErasers();

private:
    int _storedSamples;
    unsigned int _newId;
    EraserDetector::Eraser* _history;
    int _historySize;

    const EraserDetector::Eraser* _findEraser(unsigned int id, int k) const;

    friend class TrackedEraser;
};

#endif
