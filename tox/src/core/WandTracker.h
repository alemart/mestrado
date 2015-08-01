#ifndef _WANDTRACKER_H
#define _WANDTRACKER_H

#include <vector>
#include <string>
#include "ofMain.h"
#include "ImageLabeler.h"

class ImageLabeler;
class CalibratedKinect;
class BackgroundModel;
class WandCalibration;

//
// 3D Wand Tracker
//
class WandTracker
{
public:
    WandTracker(CalibratedKinect* kinect, BackgroundModel* bgmodel, ImageLabeler* labeler, int theLookupRadius = 20, int theMinArea = 200, std::string color = "yellow");
    ~WandTracker();

    struct Wand
    {
        unsigned id;
        int area;
        std::string type; // wand_yellow, wand_yellow:active, ...

        struct {
            ofVec3f coords; // in [0,1]^3 : this is what you usually want...
            ofVec3f projectedCoords; // in [0,1]^3, feature projected onto the interactive plane
            ofVec2f imageCoords; // in depth-image coords
            ofVec2f projectedImageCoords; // projected onto the interactive plane
        } position;

        struct {
            ofVec3f coords;
            ofVec3f projectedCoords;
            ofVec2f imageCoords;
            ofVec2f projectedImageCoords;
        } speed;

        inline bool operator<(const Wand& w) const { return id < w.id; }
    };

    // tracking
    void update(float dt); // call this every frame

    // get wands
    std::vector<Wand> getWands() const;

    // config
    void setLookupRadius(int theLookupRadius); // in pixels
    int lookupRadius() const; // when tracking, I will search for the colored marker within a circle of this radius
    void setMinArea(int area);
    int minArea() const;

    // calibration (TODO: move to some other class)
    WandCalibration* calibration() const;
    bool isCalibrating() const;
    void startCalibrationProcedure(); // will start collecting calibration samples (first wand only)
    bool endCalibrationProcedure(std::string calibfile = ""); // will compute the calibration matrix and so on. false on error

private:
    static unsigned _idAutoIncrement;
    CalibratedKinect* _kinect;
    BackgroundModel* _bgmodel;
    ImageLabeler* _labeler; // you call _labeler->update(), on your own, please...
    WandCalibration* _calibration;
    int _lookupRadius;
    int _minArea;
    std::vector<Wand> _wands, _oldWands;
    std::string _color;
    std::string _namefix(std::string s) const;

    // fill wand data
    bool _fillWandPositions(Wand* w, ofVec2f blobCenterOfMass);
    void _fillWandSpeeds(Wand* w, const Wand& previousWand, float dt);

    // compute best-fitting plane for the interactive surface in 3d space
    // ax + by + cz + d = 0
    struct Plane { float a, b, c, d; } _plane;
    Plane _computeBest3DPlane(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofVec3f p3);

    // calib procedure
    typedef ofVec3f CalibrationSample;
    std::vector<CalibrationSample> _calibsample;
    bool _isCalibrating;

    // misc
    int _ttl, _ttl2;
};

#endif
