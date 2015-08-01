#ifndef _SIMPLE3DTRACKER_H
#define _SIMPLE3DTRACKER_H

#include <map>
#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"

class TouchDetector;

// rastreamento de dedos acima da
// superficie
class Simple3DTracker
{
public:
    Simple3DTracker(TouchDetector* touch, int lookupRadius = 45, float weightedMatchingCoefficient = 0.5f, int templateWidth = 40, int templateHeight = 40, float maxHeightInCm = 10.0f);
    ~Simple3DTracker();

    // tracking
    // TODO: proper conversion between depth <-> color coords
    void beginFeatureTracking(ofVec2f featurePosition); // please provide: depth image coords
    void updateFeatureTracking(); // call this every frame
    void endFeatureTracking();

    bool isTracking() const;
    ofVec2f coordsOfTrackedFeature(); // depth image coords
    ofVec2f coordsOfProjectedTrackedFeature(float* distance = 0); // depth image coords (feature projected onto interactive plane)

    // 3d tracking
    ofVec3f sceneCoordsOfTrackedFeature(); // scene coords ( [0,1]^3 )
    ofVec3f sceneCoordsOfProjectedTrackedFeature(); // scene coords ( [0,1]^3 )
    void setMaxHeight(float centimiters); // will b equal to 1 in scene coords
    float maxHeight() const; // in cm

    // misc (template matching)
    ofxCvImage* templateImage() const;
    ofxCvImage* matchingImage() const;
    void setWeightedMatchingCoefficient(float coef); // 0 <= coef <= 1
    float weightedMatchingCoefficient() const;

private:
    TouchDetector* _touch;
    ofxCvImage* _template;
    ofxCvImage* _tmp;
    IplImage* _tmp2;
    int _lookupRadius;
    float _weightedMatchingCoefficient;
    float _maxHeight;
    ofVec2f _trackedFeatureCurrentPosition; // in depth image coords ...
    bool _isTracking;

    // template matching
    void _setTemplate(ofVec2f templateCenter);
    ofVec2f _predictNextPosition(ofVec2f currentPosition, float* minCost = 0);
    IplImage* _cameraImage() const;

    // compute best-fitting plane for the interactive surface in 3d space
    // ax + by + cz + d = 0
    struct Plane { float a, b, c, d; } _plane;
    Plane _computeBest3DPlane(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofVec3f p3);
};

#endif
