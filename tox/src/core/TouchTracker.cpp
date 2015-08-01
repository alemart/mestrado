#include <algorithm>
#include "TouchTracker.h"
#include "TouchDetector.h"

// number of framesteps a touch point can resist
// given that it is not being received by the sensor
#define INITIAL_TIMETOLIVE 3//6

// number of framesteps a new touch point must
// present itself before it's tracked
#define INITIAL_TIMETOBEBORN 2//3//0

TouchTracker::TouchTracker(int storedSamples, int stopColorClassificationAfterSamples) : _storedSamples(storedSamples), _stopColorClassificationAfterSamples(stopColorClassificationAfterSamples), _newId(0)
{
    if(_stopColorClassificationAfterSamples > _storedSamples)
        std::cerr << "TouchTracker error: stopColorClassificationAfterSamples > storedSamples" << std::endl;
}

TouchTracker::~TouchTracker()
{
}

void TouchTracker::feed(std::vector<TouchDetector::TouchPoint> touchPoints, float searchRadius, float dt)
{
    std::set<unsigned int> usedIds;
    std::set<unsigned int> uselessIds;

    // grab points from previous frame (or rather, grab a copy of them)
    std::vector<TouchDetector::TouchPoint> prevPoints;
    for(std::map<unsigned int, std::vector<TouchDetector::TouchPoint> >::const_iterator it = _trackedPoints.begin(); it != _trackedPoints.end(); ++it) {
        const std::vector<TouchDetector::TouchPoint>& trackedPoint = it->second;
        if(trackedPoint.size() > 0) {
            int k = trackedPoint.size();
            prevPoints.push_back(trackedPoint[k-1]);
        }
    }

    // setup new points (assign new ids, and so on)
    for(std::vector<TouchDetector::TouchPoint>::iterator it = touchPoints.begin(); it != touchPoints.end(); ++it) {
        it->startX = it->x;
        it->startY = it->y;
        it->surfaceStartX = it->surfaceX;
        it->surfaceStartY = it->surfaceY;
        it->speedX = it->speedY = 0.0f;
        it->surfaceSpeedX = it->surfaceSpeedY = 0.0f;
        it->_id = _newId++;
        it->_tracked = false;
        it->_timeToLive = INITIAL_TIMETOLIVE;
    }

    // greedy matching: current vs previous frame
    for(std::vector<TouchDetector::TouchPoint>::const_iterator prev = prevPoints.begin(); prev != prevPoints.end(); ++prev) {
        ofVec2f prevLoc(prev->x, prev->y);
        ofVec2f prevSpeed(prev->speedX, prev->speedY);
        ofVec2f predictedPosition = prevLoc + (prevSpeed * dt);

        float mindist = searchRadius;
        if(prevPoints.size() == 1 && touchPoints.size() == 1) // FIXME: does this introduce a bug?
            mindist = FLT_MAX;

        TouchDetector::TouchPoint* closestPoint = NULL;
        for(std::vector<TouchDetector::TouchPoint>::iterator cur = touchPoints.begin(); cur != touchPoints.end(); ++cur) {
            if(!(cur->_tracked)) {
                ofVec2f curLoc(cur->x, cur->y);
                float dist = curLoc.distance(predictedPosition);
                if(dist <= mindist) {
                    mindist = dist;
                    closestPoint = &(*cur);
                }
            }
        }

        if(closestPoint != NULL) {
            // found a match: closestPoint matches prev
            closestPoint->_id = prev->_id;
            closestPoint->_tracked = true;
            closestPoint->_timeToLive = INITIAL_TIMETOLIVE;
            closestPoint->startX = prev->startX;
            closestPoint->startY = prev->startY;
            closestPoint->surfaceStartX = prev->surfaceStartX;
            closestPoint->surfaceStartY = prev->surfaceStartY;
            closestPoint->speedX = (float)(closestPoint->x - prev->x) / dt;
            closestPoint->speedY = (float)(closestPoint->y - prev->y) / dt;
            closestPoint->surfaceSpeedX = (float)(closestPoint->surfaceX - prev->surfaceX) / dt;
            closestPoint->surfaceSpeedY = (float)(closestPoint->surfaceY - prev->surfaceY) / dt;
        }
    }

    // updating the database
    for(std::vector<TouchDetector::TouchPoint>::iterator it = touchPoints.begin(); it != touchPoints.end(); ++it) {
        std::map<unsigned int, std::vector<TouchDetector::TouchPoint> >::iterator ptr = _trackedPoints.find(it->_id);
        if(it->_tracked && ptr != _trackedPoints.end()) {
            // old touch point...
            std::vector<TouchDetector::TouchPoint>& v = ptr->second;

            // reaproveita classificacoes (de cores) feitas anteriormente?
            if(int(v.size()) >= _stopColorClassificationAfterSamples) {
                std::string candidate( v[int(v.size()) - 1].type );
                bool reapr = true;
                for(int j = 0; j < _stopColorClassificationAfterSamples && reapr; j++)
                    reapr = reapr && (v[ int(v.size()) - 1 - j ].type == candidate );
                if(reapr)
                    it->type = candidate;
                //std::cout << "reaproveitar cor: " << (reapr ? "sim" : "nao") << std::endl;
            }
            else
                ;//std::cout << "reaproveitar cor: " << "nao" << std::endl;

            // update vector
            if(int(v.size()) == _storedSamples) {
                for(int i=1; i<_storedSamples; i++)
                    v[i-1] = v[i];
                v[_storedSamples-1] = (*it);
            }
            else
                v.push_back(*it);
        }
        else {
            // new touch point!!!
            std::vector<TouchDetector::TouchPoint> v;
            v.push_back(*it);
            _trackedPoints.insert( std::pair<unsigned int, std::vector<TouchDetector::TouchPoint> >(it->_id, v) );
        }

        usedIds.insert(it->_id);
    }

    // figuring out which ids are the unused entries
    for(std::map<unsigned int, std::vector<TouchDetector::TouchPoint> >::iterator it = _trackedPoints.begin(); it != _trackedPoints.end(); ++it) {
        unsigned int id = it->first;
        int n = it->second.size();
        if(usedIds.count(id) == 0 && --(it->second[n-1]._timeToLive) <= 0)
            uselessIds.insert(id);
    }

    // cleaning up
    for(std::set<unsigned int>::const_iterator it = uselessIds.begin(); it != uselessIds.end(); ++it)
        _trackedPoints.erase(*it);
}

const TouchDetector::TouchPoint* TouchTracker::_findTouchPoint(unsigned int id, int k) const
{
    std::map<unsigned int, std::vector<TouchDetector::TouchPoint> >::const_iterator u = _trackedPoints.find(id);
    if(u != _trackedPoints.end()) {
        const std::vector<TouchDetector::TouchPoint>& v = u->second;
        int size = int(v.size());
        if(size > 0) {
            int j = (size - 1) + k;
            if(j < 0) j = 0;
            if(j >= size) j = size - 1;
            return &(v[j]);
        }
        else
            return NULL;
    }
    else
        return NULL;
}

std::vector<TouchTracker::TrackedTouchPoint> TouchTracker::trackedTouchPoints()
{
    std::vector<TrackedTouchPoint> w;

    for(std::map<unsigned int, std::vector<TouchDetector::TouchPoint> >::iterator it = _trackedPoints.begin(); it != _trackedPoints.end(); ++it) {
        std::vector<TouchDetector::TouchPoint>& v = it->second;

        // reaproveita classificacoes (de cores) feitas anteriormente?
        if(int(v.size()) >= _stopColorClassificationAfterSamples) {
            std::string candidate( v[v.size()-1].type );
            bool reapr = true;
            for(int j = 0; j < _stopColorClassificationAfterSamples && reapr; j++)
	        reapr = reapr && (v[v.size()-1 - j].type == candidate );
            if(reapr)
	        v[v.size()-1].type = candidate;
        }

        // avoid noise...
        if(v.size() == 0)
            continue;
        if((int(v.size()) < std::min(_storedSamples, 3 * INITIAL_TIMETOBEBORN)) && (v[v.size()-1].type.find("finger") != std::string::npos || v[v.size()-1].type.find("magenta") != std::string::npos || v[v.size()-1].type == "null"))
            continue;
        if(int(v.size()) < std::min(_storedSamples, INITIAL_TIMETOBEBORN))
            continue;



        TrackedTouchPoint p(this, it->first);
        w.push_back(p);
    }

    return w;
}
