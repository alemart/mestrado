#include "WallOS.h"
#include "WallApp.h"

// this method is used to filter out annoying noise
std::list<TUIO::TuioObject*> WallApp::_filter(const std::list<TUIO::TuioObject*>& l) const
{
    std::list<TUIO::TuioObject*> fingers;
    std::list<TUIO::TuioObject*> erasers;

    // filter erasers
    for(TUIO::TuioObject* t : l) {
        if(t->getSymbolID() == TouchType.ERASER)
            erasers.push_back(t);
    }

    // filter fingers
    for(TUIO::TuioObject* t : l) {
        if(t->getSymbolID() == TouchType.FINGER)
            fingers.push_back(t);
    }
    
    // grab the result
    if(erasers.size() > 0)
        return erasers; // if we've got an eraser, return only the erasers
    else if(fingers.size() > 0)
        return fingers; // if we've got a finger, return only the fingers
    else
        return l; // return the markers
}
