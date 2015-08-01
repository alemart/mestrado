#ifndef _WANDCALIBRATION_H
#define _WANDCALIBRATION_H

#include <algorithm>
#include <vector>
#include <string>
#include "ofMain.h"
#include "ofxOpenCv.h"

#define DEFAULT_WANDCALIB_FILE "data/wandcalib/params.yaml"

class WandCalibration
{
public:
    WandCalibration(std::string calibfile = DEFAULT_WANDCALIB_FILE);
    ~WandCalibration();

    void reset();
    bool load(std::string calibfile = DEFAULT_WANDCALIB_FILE); // false on error
    bool save(std::string calibfile = DEFAULT_WANDCALIB_FILE);

    void beginCalibration();
    void addCorrespondence(ofVec3f normalizedCoords, ofVec3f worldCoords); // I need at least 4 correspondences
    bool endCalibration(); // false on error

    ofVec3f world2norm(ofVec3f worldCoords); // converts world coords to normalized coords

private:
    CvMat* _calibMatrix; // a 3x4 calibration matrix modelling an Affine Transform from WorldCoords to NormalizedCoords
    CvMat* _tmp[2];
    std::vector< std::pair<ofVec3f, ofVec3f> > _correspondences; // a set of (normalized, world) pairs
};

#endif
