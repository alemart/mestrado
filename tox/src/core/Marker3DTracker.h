#ifndef _COLORED3DTRACKER_H
#define _COLORED3DTRACKER_H

#include <map>
#include <vector>
#include <string>
#include "ofxOpenCv.h"
#include "cvblob.h"

class ImageLabeler;
class CalibratedKinect;
class BackgroundModel;

// rastreamento 3D
class Marker3DTracker
{
public:
    Marker3DTracker(CalibratedKinect* kinect, BackgroundModel* bgmodel, ImageLabeler* labeler, int theLookupRadius = 20, float maxHeightInCm = 10.0f);
    ~Marker3DTracker();

    // tracking
    // TODO: proper conversion between depth <-> color coords
    void beginFeatureTracking(ofVec2f featurePosition, std::string featureClass = ""); // please provide: color image coords, and optionally the class of the tracked object (for better results)
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
    void setLookupRadius(int theLookupRadius); // in pixels
    int lookupRadius() const; // will search for the colored marker within a circle of this radius

private:
    CalibratedKinect* _kinect;
    BackgroundModel* _bgmodel;
    ImageLabeler* _labeler;
    int _lookupRadius;
    float _maxHeight;
    ofVec2f _trackedFeaturePosition; // in color image coords ...
    ofVec2f _trackedFeatureSpeed; // instantaneous speed
    std::string _trackedFeatureClass;
    int _trackedFeatureArea;
    bool _isTracking;

    std::string _namefix(std::string s) const;

    // compute best-fitting plane for the interactive surface in 3d space
    // ax + by + cz + d = 0
    struct Plane { float a, b, c, d; } _plane;
    Plane _computeBest3DPlane(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofVec3f p3);
};

#endif
