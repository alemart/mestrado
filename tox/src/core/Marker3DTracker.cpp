#include "Marker3DTracker.h"
#include "ImageLabeler.h"
#include "CalibratedKinect.h"
#include "BackgroundModel.h"

Marker3DTracker::Marker3DTracker(CalibratedKinect* kinect, BackgroundModel* bgmodel, ImageLabeler* labeler, int theLookupRadius, float maxHeightInCm) :
    _kinect(kinect), _bgmodel(bgmodel), _labeler(labeler), _lookupRadius(theLookupRadius), _maxHeight(0.0f), _isTracking(false)
{
    // max height!!
    setMaxHeight(maxHeightInCm);

    // determine the 3d plane of the interactive surface
    ofVec3f topleft = ofVec3f( bgmodel->topleft3DVertex().x, bgmodel->topleft3DVertex().y, bgmodel->topleft3DVertex().z );
    ofVec3f topright = ofVec3f( bgmodel->topright3DVertex().x, bgmodel->topright3DVertex().y, bgmodel->topright3DVertex().z );
    ofVec3f bottomright = ofVec3f( bgmodel->bottomright3DVertex().x, bgmodel->bottomright3DVertex().y, bgmodel->bottomright3DVertex().z );
    ofVec3f bottomleft = ofVec3f( bgmodel->bottomleft3DVertex().x, bgmodel->bottomleft3DVertex().y, bgmodel->bottomleft3DVertex().z );
    _plane = _computeBest3DPlane(topleft, topright, bottomright, bottomleft);
}

Marker3DTracker::~Marker3DTracker()
{
}

void Marker3DTracker::beginFeatureTracking(ofVec2f featurePosition, std::string featureClass)
{
    // reset
    if(isTracking())
        endFeatureTracking();
    featureClass = _namefix(featureClass);

    // let's track the damn thing
    if(featureClass.find("marker") != std::string::npos) { // I want markers only !!!
        ImageLabeler::ConnectedComponent blob(_labeler->getBlobAt(featurePosition, featureClass, _lookupRadius));
        if(blob.classCount[featureClass] > 0 || featureClass == "") {
            _trackedFeatureSpeed = ofVec2f(0.0f, 0.0f);
            _trackedFeaturePosition = ofVec2f(blob.centerOfMass);
            _trackedFeaturePosition.x = std::floor(_trackedFeaturePosition.x);
            _trackedFeaturePosition.y = std::floor(_trackedFeaturePosition.y);
            _trackedFeatureClass = _namefix(featureClass);
            _trackedFeatureArea = blob.pixels.size();

            // done
            _isTracking = true;
        }
    }
}

void Marker3DTracker::updateFeatureTracking()
{
    if(isTracking()) {
        ImageLabeler::ConnectedComponent blob(_labeler->getBlobAt(_trackedFeaturePosition + _trackedFeatureSpeed, _trackedFeatureClass, _lookupRadius)); // predict new position linearly
        if(blob.classCount[_trackedFeatureClass] > 0 && blob.centerOfMass.distance(_trackedFeaturePosition) <= _lookupRadius) {
            // successful tracking
            _trackedFeatureSpeed = blob.centerOfMass - _trackedFeaturePosition;
            _trackedFeatureSpeed.x = std::floor(_trackedFeatureSpeed.x);
            _trackedFeatureSpeed.y = std::floor(_trackedFeatureSpeed.y);
            _trackedFeaturePosition = blob.centerOfMass;
            _trackedFeaturePosition.x = std::floor(_trackedFeaturePosition.x);
            _trackedFeaturePosition.y = std::floor(_trackedFeaturePosition.y);
            _trackedFeatureArea = blob.pixels.size();
        }
        else
            endFeatureTracking();
    }
}

void Marker3DTracker::endFeatureTracking()
{
    _isTracking = false;
}

bool Marker3DTracker::isTracking() const
{
    return _isTracking;
}

ofVec2f Marker3DTracker::coordsOfTrackedFeature()
{
    return _trackedFeaturePosition;
}

void Marker3DTracker::setMaxHeight(float centimiters)
{
    _maxHeight = std::max(0.0f, centimiters * 0.01f);
}

float Marker3DTracker::maxHeight() const
{
    return _maxHeight * 100.0f;
}

void Marker3DTracker::setLookupRadius(int theLookupRadius)
{
    _lookupRadius = theLookupRadius;
}

int Marker3DTracker::lookupRadius() const
{
    return _lookupRadius;
}




ofVec2f Marker3DTracker::coordsOfProjectedTrackedFeature(float* distance) // returns the position of the tracked feature projected onto the interactive plane
{
    ofVec3f q = _kinect->depthCoords2worldCoords( coordsOfTrackedFeature() );
    ofVec3f n = ofVec3f( _plane.a, _plane.b, _plane.c );
    ofVec3f p = q - ((q.dot(n) + _plane.d) / (n.length() * n.length())) * n; // projects q onto _plane

    if(distance)
        *distance = q.distance(p);

    return _kinect->worldCoords2depthCoords(p);
}

ofVec3f Marker3DTracker::sceneCoordsOfTrackedFeature()
{
    float dist = 0.0f;
    coordsOfProjectedTrackedFeature(&dist);
    ofVec2f q = coordsOfTrackedFeature();
    CvPoint2D32f xy = _bgmodel->convertToSurfaceCoordinates(cvPoint(q.x, q.y)); // cvPoint(p.x, p.y));
    return ofVec3f(xy.x, xy.y, dist / _maxHeight);
}

ofVec3f Marker3DTracker::sceneCoordsOfProjectedTrackedFeature()
{
    float dist = 0.0f;
    ofVec2f p = coordsOfProjectedTrackedFeature(&dist);
    CvPoint2D32f xy = _bgmodel->convertToSurfaceCoordinates(cvPoint(p.x, p.y));
    return ofVec3f(xy.x, xy.y, 0.0f);
}

Marker3DTracker::Plane Marker3DTracker::_computeBest3DPlane(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofVec3f p3)
{
    Plane plane = { 0.0f, 0.0, 0.0f, 1.0f }; // we enforce d = 1
    CvMat* M = cvCreateMat(4, 3, CV_32FC1);
    CvMat* v = cvCreateMat(3, 1, CV_32FC1);
    CvMat* b = cvCreateMat(4, 1, CV_32FC1);
    ofVec3f p[4] = { p0, p1, p2, p3 };

    // debug
    for(int i=0; i<4; i++) {
        ofVec2f q = _kinect->worldCoords2depthCoords(p[i]);
        std::cout << "[3dtracker] " << (i+1) << "" <<
                     (i+1 == 1 ? "st" : (i+1 == 2 ? "nd" : (i+1 == 3 ? "rd" : "th"))) << " " <<
                     "plane vertex:\n            " <<
                     "(" << p[i].x << ", " << p[i].y << ", " << p[i].z << ") => " <<
                     "(" << q.x << ", " << q.y << ")" << std::endl;
    }

    // initialize matrix M
    for(int i=0; i<4; i++) {
        CV_MAT_ELEM(*M, float, i, 0) = p[i].x;
        CV_MAT_ELEM(*M, float, i, 1) = p[i].y;
        CV_MAT_ELEM(*M, float, i, 2) = p[i].z;
    }

    // intialize vector v
    cvZero(v);

    // initialize vector b
    for(int i=0; i<4; i++)
        CV_MAT_ELEM(*b, float, i, 0) = -plane.d;

    // solve Mv = b for v (least-squares)
    cvSolve(M, b, v, CV_SVD);

    // get the plane data
    plane.a = CV_MAT_ELEM(*v, float, 0, 0);
    plane.b = CV_MAT_ELEM(*v, float, 1, 0);
    plane.c = CV_MAT_ELEM(*v, float, 2, 0);

    // done!
    cvReleaseMat(&M);
    cvReleaseMat(&v);
    cvReleaseMat(&b);
    return plane;
}



std::string Marker3DTracker::_namefix(std::string s) const
{
    if(s.substr(0, 9) == "projetor_")
        return s.substr(9);
    else if(s == "eraser_green")
        return "marker_green";
    else
        return s;
}
