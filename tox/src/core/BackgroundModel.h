#ifndef _BACKGROUNDMODEL_H
#define _BACKGROUNDMODEL_H

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ImageAccumulator.h"
class CalibratedKinect;

class BackgroundModel
{
public:
    ~BackgroundModel();

    // create a model from a sequence of snapshots of the background + 4 points
    // the vertices are in depth image coords
    static BackgroundModel* create(const CalibratedKinect* kinect, const ImageAccumulator& ac, CvPoint topleft, CvPoint topright, CvPoint bottomright, CvPoint bottomleft);

    // load from disk
    static BackgroundModel* load();

    // save to disk
    bool save();


    // accessors
    inline CvPoint topleftVertex() const { return _vertex[0]; }
    inline CvPoint toprightVertex() const { return _vertex[1]; }
    inline CvPoint bottomrightVertex() const { return _vertex[2]; }
    inline CvPoint bottomleftVertex() const { return _vertex[3]; }

    inline CvPoint3D32f topleft3DVertex() const { return _3dvertex[0]; }
    inline CvPoint3D32f topright3DVertex() const { return _3dvertex[1]; }
    inline CvPoint3D32f bottomright3DVertex() const { return _3dvertex[2]; }
    inline CvPoint3D32f bottomleft3DVertex() const { return _3dvertex[3]; }

    inline const IplImage* image() const { return _image; }
    inline const IplImage* mask() const { return _mask; }
    inline const IplImage* mean() const { return _mean; }
    inline const IplImage* standardDeviation() const { return _sd; }
    inline const IplImage* unmaskedImage() const { return _unmaskedImage; }

    // apply homography (surface coords: [0,1] x [0,1])
    CvPoint2D32f convertToSurfaceCoordinates(CvPoint pointInDepthImage) const;



private:
    BackgroundModel() : _image(0), _mask(0), _mean(0), _sd(0), _img2surface(0) { }

    IplImage* _image; // median
    IplImage* _mask; // mask
    IplImage* _mean; // mean
    IplImage* _sd; // standard deviation
    IplImage* _unmaskedImage; // unmasked image
    CvPoint _vertex[4];
    CvPoint3D32f _3dvertex[4];

    static IplImage* _createMask(int maskWidth, int maskHeight, CvPoint p1, CvPoint p2, CvPoint p3, CvPoint p4);
    static IplImage* _createMedianImage(const ImageAccumulator& ac, const IplImage* mask);
    static IplImage* _createMeanImage(const ImageAccumulator& ac, const IplImage* mask);
    static IplImage* _createSDImage(const ImageAccumulator& ac, const IplImage* mask);

    CvMat* _img2surface;
    CvMat* _computeHomography();
    void _compute3DVertices(const CalibratedKinect* kinect, const IplImage *image);
};

#endif
