#include <algorithm>
#include "TouchTracker.h"
#include "TouchDetector.h"
#include "CalibratedKinect.h"
#include "Simple3DTracker.h"
#include "BackgroundModel.h"

// minimum distance such that the tracked template
// gets automatically updated
#define UPDATETPL_MINDIST       1000 //20

Simple3DTracker::Simple3DTracker(TouchDetector* touch, int lookupRadius, float weightedMatchingCoefficient, int templateWidth, int templateHeight, float maxHeightInCm) : _touch(touch), _template(0), _tmp(0), _tmp2(0), _lookupRadius(lookupRadius), _weightedMatchingCoefficient(weightedMatchingCoefficient), _maxHeight(0.0f)
{
    // coeffs
    if(_weightedMatchingCoefficient < 0.0f)
        _weightedMatchingCoefficient = 0.0f;
    else if(_weightedMatchingCoefficient > 1.0f)
        _weightedMatchingCoefficient = 1.0f;

    // max height
    setMaxHeight(maxHeightInCm);

    // create new template image
    _template = new ofxCvGrayscaleImage;
    _template->allocate(templateWidth, templateHeight);
    cvZero(_template->getCvImage());
    _template->flagImageChanged();

    // matching images (float & grayscale)
    _tmp = new ofxCvGrayscaleImage;
    _tmp->allocate(_cameraImage()->width - _template->width + 1, _cameraImage()->height - _template->height + 1);
    _tmp2 = cvCreateImage(
        cvSize(_cameraImage()->width - _template->width + 1, _cameraImage()->height - _template->height + 1),
        IPL_DEPTH_32F,
        1
    );

    // determine the 3d plane of the interactive surface
    BackgroundModel* bg = _touch->backgroundModel();
    ofVec3f topleft = ofVec3f( bg->topleft3DVertex().x, bg->topleft3DVertex().y, bg->topleft3DVertex().z );
    ofVec3f topright = ofVec3f( bg->topright3DVertex().x, bg->topright3DVertex().y, bg->topright3DVertex().z );
    ofVec3f bottomright = ofVec3f( bg->bottomright3DVertex().x, bg->bottomright3DVertex().y, bg->bottomright3DVertex().z );
    ofVec3f bottomleft = ofVec3f( bg->bottomleft3DVertex().x, bg->bottomleft3DVertex().y, bg->bottomleft3DVertex().z );
    _plane = _computeBest3DPlane(topleft, topright, bottomright, bottomleft);
}

Simple3DTracker::~Simple3DTracker()
{
    if(_template)
        delete _template;
    if(_tmp)
        delete _tmp;
    if(_tmp2)
        cvReleaseImage(&_tmp2);
}

void Simple3DTracker::_setTemplate(ofVec2f templateCenter)
{
    IplImage* haystack = _cameraImage();
    int topleft_x = std::max(0, (int)templateCenter.x - _template->width/2);
    int topleft_y = std::max(0, (int)templateCenter.y - _template->height/2);
    int bottomright_x = std::min(topleft_x + _template->width, haystack->width);
    int bottomright_y = std::min(topleft_y + _template->height, haystack->height);
    for(int y = topleft_y; y < bottomright_y; y++) {
        unsigned char* src = (unsigned char*)(haystack->imageData + y * haystack->widthStep);
        unsigned char* dst = (unsigned char*)(_template->getCvImage()->imageData + (y - topleft_y) * _template->getCvImage()->widthStep);
        for(int x = topleft_x; x < bottomright_x; x++)
            dst[x - topleft_x] = src[x];
    }
    _template->flagImageChanged();
}

ofVec2f Simple3DTracker::_predictNextPosition(ofVec2f currentPosition, float* minCost)
{
    int bestx = currentPosition.x, besty = currentPosition.y;
    float bestcost = 9999999, cost, distance;
    const float alpha = _weightedMatchingCoefficient;

    if(!_template || !_tmp || !_tmp2)
        return currentPosition;

    // template matching
    IplImage* haystack = _cameraImage();
    cvMatchTemplate(haystack, _template->getCvImage(), _tmp2, CV_TM_CCOEFF);
    cvNormalize(_tmp2, _tmp2, 1.0, 0.0, CV_MINMAX);

    // find the best match
    for(int y = 0; y < _tmp2->height; y++) {
        const float *src = (const float*)(_tmp2->imageData + y * _tmp2->widthStep);
        unsigned char *dst = (unsigned char*)(_tmp->getCvImage()->imageData + y * _tmp->getCvImage()->widthStep);
        for(int x = 0; x < _tmp2->width; x++) {
            dst[x] = (unsigned char)(src[x] * 255.0f);
            distance = currentPosition.distance(ofVec2f(x, y));
            if(distance <= _lookupRadius) {
                cost = (alpha * (1.0f - src[x])) + ((1.0f - alpha) * distance / _lookupRadius);
                if(cost <= bestcost) { // weighted matching
                    bestx = x;
                    besty = y;
                    bestcost = cost;
                }
            }
        }
    }
    _tmp->flagImageChanged();

    // get the resulting position...
    ofVec2f result(bestx + _template->width/2, besty + _template->height/2);

    // return the min cost?
    if(minCost)
        *minCost = bestcost;

    // update the template?
    if(result.distance(currentPosition) >= UPDATETPL_MINDIST)
        _setTemplate(result);

    // done!
    return result;
}

IplImage* Simple3DTracker::_cameraImage() const
{
    return _touch->blacknwhiteImageWithoutBackground()->getCvImage();
}

ofxCvImage* Simple3DTracker::templateImage() const
{
    return _template;
}

ofxCvImage* Simple3DTracker::matchingImage() const
{
    return _tmp;
}











void Simple3DTracker::beginFeatureTracking(ofVec2f featurePosition)
{
    _trackedFeatureCurrentPosition = featurePosition;
    _setTemplate(featurePosition);
    _isTracking = true;
}

void Simple3DTracker::endFeatureTracking()
{
    if(_isTracking) {
        _isTracking = false;
        cvZero(_template->getCvImage());
        _template->flagImageChanged();
    }
}

bool Simple3DTracker::isTracking() const
{
    return _isTracking;
}

void Simple3DTracker::updateFeatureTracking()
{
    float cost = 0.0f;
    _trackedFeatureCurrentPosition = _predictNextPosition(_trackedFeatureCurrentPosition, &cost);
    _isTracking = (cost <= 1 - weightedMatchingCoefficient()); // FIXME: this is totally ad-hoc ......
}

ofVec2f Simple3DTracker::coordsOfTrackedFeature()
{
    return _trackedFeatureCurrentPosition;
}

ofVec2f Simple3DTracker::coordsOfProjectedTrackedFeature(float* distance) // returns the position of the tracked feature projected onto the interactive plane
{
    ofVec3f q = _touch->kinect()->depthCoords2worldCoords( coordsOfTrackedFeature() );
    ofVec3f n = ofVec3f( _plane.a, _plane.b, _plane.c );
    ofVec3f p = q - ((q.dot(n) + _plane.d) / (n.length() * n.length())) * n; // projects q onto _plane

    if(distance)
        *distance = q.distance(p);

    return _touch->kinect()->worldCoords2depthCoords(p);
}






ofVec3f Simple3DTracker::sceneCoordsOfTrackedFeature()
{
    float dist = 0.0f;
    ofVec2f p = coordsOfProjectedTrackedFeature(&dist);
    ofVec2f q = coordsOfTrackedFeature();
    CvPoint2D32f xy = _touch->backgroundModel()->convertToSurfaceCoordinates(cvPoint(q.x, q.y)); // cvPoint(p.x, p.y));
    return ofVec3f(xy.x, xy.y, dist / _maxHeight);
}

ofVec3f Simple3DTracker::sceneCoordsOfProjectedTrackedFeature()
{
    float dist = 0.0f;
    ofVec2f p = coordsOfProjectedTrackedFeature(&dist);
    CvPoint2D32f xy = _touch->backgroundModel()->convertToSurfaceCoordinates(cvPoint(p.x, p.y));
    return ofVec3f(xy.x, xy.y, 0.0f);
}

void Simple3DTracker::setMaxHeight(float centimiters)
{
    _maxHeight = std::max(0.0f, centimiters * 0.01f);
}

float Simple3DTracker::maxHeight() const
{
    return _maxHeight * 100.0f;
}

float Simple3DTracker::weightedMatchingCoefficient() const
{
    return _weightedMatchingCoefficient;
}

void Simple3DTracker::setWeightedMatchingCoefficient(float coef)
{
    _weightedMatchingCoefficient = std::min(1.0f, std::max(0.0f, coef));
}




Simple3DTracker::Plane Simple3DTracker::_computeBest3DPlane(ofVec3f p0, ofVec3f p1, ofVec3f p2, ofVec3f p3)
{
    Plane plane = { 0.0f, 0.0, 0.0f, 1.0f }; // we enforce d = 1
    CvMat* M = cvCreateMat(4, 3, CV_32FC1);
    CvMat* v = cvCreateMat(3, 1, CV_32FC1);
    CvMat* b = cvCreateMat(4, 1, CV_32FC1);
    ofVec3f p[4] = { p0, p1, p2, p3 };

    // debug
    for(int i=0; i<4; i++) {
        ofVec2f q = _touch->kinect()->worldCoords2depthCoords(p[i]);
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
