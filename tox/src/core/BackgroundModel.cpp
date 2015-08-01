#include <iostream>
#include <fstream>
#include "BackgroundModel.h"
#include "CalibratedKinect.h"

BackgroundModel* BackgroundModel::create(const CalibratedKinect* kinect, const ImageAccumulator& ac, CvPoint topleft, CvPoint topright, CvPoint bottomright, CvPoint bottomleft)
{
    int w = ac.image(0)->width, h = ac.image(0)->height;

    if(ac.count() == 0) {
        std::cerr << "[error] BackgroundModel::create - empty image accumulator" << std::endl;
        return 0;
    }

    BackgroundModel* m = new BackgroundModel;
    m->_vertex[0] = topleft;
    m->_vertex[1] = topright;
    m->_vertex[2] = bottomright;
    m->_vertex[3] = bottomleft;
    m->_img2surface = m->_computeHomography();

    m->_mask = _createMask(w, h, topleft, topright, bottomright, bottomleft);
    if(!m->_mask) {
        std::cerr << "[error] BackgroundModel::create - can't create the mask (out of memory?)" << std::endl;
        delete m;
        return 0;
    }

    m->_image = _createMedianImage(ac, m->_mask);
    if(!m->_image) {
        std::cerr << "[error] BackgroundModel::create - can't compute background image" << std::endl;
        cvReleaseImage(&m->_mask);
        delete m;
        return 0;
    }

    m->_mean = _createMeanImage(ac, m->_mask);
    if(!m->_mean) {
        std::cerr << "[error] BackgroundModel::create - can't compute mean image" << std::endl;
        cvReleaseImage(&m->_mask);
        cvReleaseImage(&m->_image);
        delete m;
        return 0;
    }

    m->_sd = _createSDImage(ac, m->_mask);
    if(!m->_sd) {
        std::cerr << "[error] BackgroundModel::create - can't compute standard deviation image" << std::endl;
        cvReleaseImage(&m->_mask);
        cvReleaseImage(&m->_image);
        cvReleaseImage(&m->_mean);
        delete m;
        return 0;
    }

    IplImage* white = _createMask(w, h, cvPoint(0, 0), cvPoint(w-1, 0), cvPoint(w-1, h-1), cvPoint(0, h-1));
    m->_unmaskedImage = _createMedianImage(ac, white);
    if(!m->_unmaskedImage) {
        std::cerr << "[error] BackgroundModel::create - can't compute unmasked image" << std::endl;
        cvReleaseImage(&m->_mask);
        cvReleaseImage(&m->_image);
        cvReleaseImage(&m->_mean);
        cvReleaseImage(&m->_sd);
        delete m;
        return 0;
    }
    cvReleaseImage(&white);

    m->_compute3DVertices(kinect, m->_image);
    return m;
}

BackgroundModel* BackgroundModel::load()
{
    ifstream f("data/bgmodel/vertices.txt");
    if(!f.is_open()) {
        std::cerr << "[error] BackgroundModel::load - can't open vertices file" << std::endl;
        return 0;
    }

    BackgroundModel* m = new BackgroundModel;
    for(int i=0; i<4; i++)
        f >> m->_vertex[i].x >> m->_vertex[i].y;
    for(int i=0; i<4; i++)
        f >> m->_3dvertex[i].x >> m->_3dvertex[i].y >> m->_3dvertex[i].z;
    m->_img2surface = m->_computeHomography();

    m->_image = (IplImage*)cvLoad("data/bgmodel/image.yaml");
    if(!m->_image) {
        std::cerr << "[error] BackgroundModel::load - can't open background image" << std::endl;
        delete m;
        return 0;
    }

    m->_mask = (IplImage*)cvLoad("data/bgmodel/mask.yaml");
    if(!m->_mask) {
        std::cerr << "[error] BackgroundModel::load - can't open mask image" << std::endl;
        cvReleaseImage(&m->_image);
        delete m;
        return 0;
    }

    m->_mean = (IplImage*)cvLoad("data/bgmodel/mean.yaml");
    if(!m->_mean) {
        std::cerr << "[error] BackgroundModel::load - can't open mean image" << std::endl;
        cvReleaseImage(&m->_image);
        cvReleaseImage(&m->_mask);
        delete m;
        return 0;
    }

    m->_sd = (IplImage*)cvLoad("data/bgmodel/sd.yaml");
    if(!m->_sd) {
        std::cerr << "[error] BackgroundModel::load - can't open standard deviation image" << std::endl;
        cvReleaseImage(&m->_mean);
        cvReleaseImage(&m->_image);
        cvReleaseImage(&m->_mask);
        delete m;
        return 0;
    }

    m->_unmaskedImage = (IplImage*)cvLoad("data/bgmodel/unmasked.yaml");
    if(!m->_unmaskedImage) {
        std::cerr << "[error] BackgroundModel::load - can't open unmasked image" << std::endl;
        cvReleaseImage(&m->_mean);
        cvReleaseImage(&m->_image);
        cvReleaseImage(&m->_mask);
        cvReleaseImage(&m->_sd);
        delete m;
        return 0;
    }

    return m;
}

BackgroundModel::~BackgroundModel()
{
    cvReleaseImage(&_mask);
    cvReleaseImage(&_image);
    cvReleaseImage(&_mean);
    cvReleaseImage(&_sd);
    cvReleaseImage(&_unmaskedImage);
    cvReleaseMat(&_img2surface);
}

bool BackgroundModel::save()
{
    ofstream f("data/bgmodel/vertices.txt");
    if(!f.is_open()) {
        std::cerr << "[error] BackgroundModel::save - can't open vertices file" << std::endl;
        return false;
    }

    for(int i=0; i<4; i++)
        f << _vertex[i].x << " " << _vertex[i].y << std::endl;
    f << std::endl;
    for(int i=0; i<4; i++)
        f << _3dvertex[i].x << " " << _3dvertex[i].y << " " << _3dvertex[i].z << std::endl;

    cvSave("data/bgmodel/image.yaml", _image);
    cvSave("data/bgmodel/mask.yaml", _mask);
    cvSave("data/bgmodel/mean.yaml", _mean);
    cvSave("data/bgmodel/sd.yaml", _sd);
    cvSave("data/bgmodel/unmasked.yaml", _unmaskedImage);

    return true;
}

CvPoint2D32f BackgroundModel::convertToSurfaceCoordinates(CvPoint pointInDepthImage) const
{
    CvPoint2D32f p;
    CvMat *src = cvCreateMat(3, 1, CV_32FC1);
    CvMat *dst = cvCreateMat(3, 1, CV_32FC1);

    CV_MAT_ELEM(*src, float, 0, 0) = pointInDepthImage.x;
    CV_MAT_ELEM(*src, float, 1, 0) = pointInDepthImage.y;
    CV_MAT_ELEM(*src, float, 2, 0) = 1.0f;

    // apply the homography
    cvGEMM(_img2surface, src, 1.0f, NULL, 0.0f, dst, 0);

    p.x = CV_MAT_ELEM(*dst, float, 0, 0) / CV_MAT_ELEM(*dst, float, 2, 0);
    p.y = CV_MAT_ELEM(*dst, float, 1, 0) / CV_MAT_ELEM(*dst, float, 2, 0);

    cvReleaseMat(&src);
    cvReleaseMat(&dst);
    return p;
}

IplImage* BackgroundModel::_createMask(int maskWidth, int maskHeight, CvPoint p1, CvPoint p2, CvPoint p3, CvPoint p4)
{
    CvPoint pts[] = { p1, p2, p3, p4 };
/*
    pts[0] = cvPoint(0, 0);
    pts[1] = cvPoint(maskWidth-1, 0);
    pts[2] = cvPoint(maskWidth-1, maskHeight-1);
    pts[3] = cvPoint(0, maskHeight-1);
*/

    IplImage* mask = cvCreateImage(
        cvSize(maskWidth, maskHeight),
        IPL_DEPTH_8U,
        1
    );

    if(mask) {
        cvZero(mask);
        cvFillConvexPoly(mask, pts, 4, cvScalar(255), 8);
        return mask;
    }
    else
        return 0;
}

IplImage* BackgroundModel::_createMedianImage(const ImageAccumulator& ac, const IplImage* mask)
{
    if(ac.count() == 0)
        return 0; // ooops...

    if(ac.image(0)->depth != IPL_DEPTH_16U && ac.image(0)->depth != IPL_DEPTH_16S) {
        std::cerr << "[error] BackgroundModel::_createMedianImage - a set of 16-bit images is required" << std::endl;
        return 0;
    }

    if(mask->depth != IPL_DEPTH_8U) {
        std::cerr << "[error] BackgroundModel::_createMedianImage - invalid mask" << std::endl;
        return 0;
    }

    //
    // Linear time (average) select algorithm
    //
    class {
    public:
        unsigned short operator()(unsigned short v[], int p, int r, int idx)
        {
            if(p != r) {
                int q = _partition(v, p, r);
                int k = q-p+1;

                if(k == idx+1)
                    return v[q];
                else if(k > idx+1)
                    return (*this)(v, p, q-1, idx);
                else
                    return (*this)(v, q+1, r, idx - k);
            }
            else
                return v[p];
        }

    private:
        inline int _partition(unsigned short v[], int p, int r)
        {
            int rnd = p + (std::rand() % (r-p+1));
            _swap(v[rnd], v[r]);

            int j, i = p-1;
            unsigned short pivot = v[r];
            for(j = p; j < r; j++) {
                if(v[j] <= pivot)
                    _swap(v[j], v[++i]);
            }

            _swap(v[i+1], v[r]);
            return i+1;
        }

        inline void _swap(unsigned short& a, unsigned short& b)
        {
            // assuming addr(a) != addr(b)
            if(&a != &b) {
                a ^= b;
                b ^= a;
                a ^= b;
            }
        }
    } select;

    //
    // creating the median image...
    //
    IplImage *img = cvCloneImage(ac.image(0)); // it clones the header too
    cvZero(img);

    // median vector (auxiliary)
    int n = ac.count();
    unsigned short *v = new unsigned short[n];

    // walk through all pixels
    int x, y, q;
    unsigned char *ptr;
    unsigned short *ptr2;
    const IplImage *tmp;
    for(y=0; y<mask->height; y++) {
        ptr = (unsigned char*)(mask->imageData + y * mask->widthStep);
        for(x=0; x<mask->width; x++) {
            if(ptr[x] != 0) { // valid point (mask)
                // get the values of (x,y) from all images
                for(q=0; q<n; q++) {
                    tmp = ac.image(q);
                    ptr2 = (unsigned short*)(tmp->imageData + y * tmp->widthStep);
                    v[q] = ptr2[x];
                }

                // all right, plot the median into img
                ptr2 = (unsigned short*)(img->imageData + y * img->widthStep);
                ptr2[x] = select(v, 0, n-1, n/2);
            }
        }
    }

    // done!
    delete[] v;
    //cvDilate(img, img, NULL, 1); // FIXME: this removes tiny black pixels
    return img;
}

IplImage* BackgroundModel::_createMeanImage(const ImageAccumulator& ac, const IplImage* mask)
{
    if(ac.count() == 0)
        return 0; // ooops...

    if(ac.image(0)->depth != IPL_DEPTH_16U && ac.image(0)->depth != IPL_DEPTH_16S) {
        std::cerr << "[error] BackgroundModel::_createMeanImage - a set of 16-bit images is required" << std::endl;
        return 0;
    }

    if(mask->depth != IPL_DEPTH_8U) {
        std::cerr << "[error] BackgroundModel::_createMeanImage - invalid mask" << std::endl;
        return 0;
    }

    // creating the image...
    IplImage *img = cvCloneImage(ac.image(0)); // it clones the header too
    cvZero(img);

    // auxiliary vector
    int n = ac.count();
    unsigned short *v = new unsigned short[n];

    // walk through all pixels
    int x, y, q;
    unsigned char *ptr;
    unsigned short *ptr2;
    const IplImage *tmp;
    for(y=0; y<mask->height; y++) {
        ptr = (unsigned char*)(mask->imageData + y * mask->widthStep);
        for(x=0; x<mask->width; x++) {
            if(ptr[x] != 0) { // valid point (mask)
                int sum = 0;

                // get the values of (x,y) from all images
                for(q=0; q<n; q++) {
                    tmp = ac.image(q);
                    ptr2 = (unsigned short*)(tmp->imageData + y * tmp->widthStep);
                    sum += (v[q] = ptr2[x]);
                }

                // all right, plot the mean into img
                ptr2 = (unsigned short*)(img->imageData + y * img->widthStep);
                ptr2[x] = (unsigned short)(sum / n);
            }
        }
    }

    // done!
    delete[] v;
    return img;
}

IplImage* BackgroundModel::_createSDImage(const ImageAccumulator& ac, const IplImage* mask)
{
    if(ac.count() == 0)
        return 0; // ooops...

    if(ac.image(0)->depth != IPL_DEPTH_16U && ac.image(0)->depth != IPL_DEPTH_16S) {
        std::cerr << "[error] BackgroundModel::_createMeanImage - a set of 16-bit images is required" << std::endl;
        return 0;
    }

    if(mask->depth != IPL_DEPTH_8U) {
        std::cerr << "[error] BackgroundModel::_createMeanImage - invalid mask" << std::endl;
        return 0;
    }

    // creating the image...
    IplImage *img = cvCloneImage(ac.image(0)); // it clones the header too
    cvZero(img);

    // auxiliary vector
    int n = ac.count();
    unsigned short *v = new unsigned short[n];

    // walk through all pixels
    int x, y, q;
    unsigned char *ptr;
    unsigned short *ptr2;
    const IplImage *tmp;
    for(y=0; y<mask->height; y++) {
        ptr = (unsigned char*)(mask->imageData + y * mask->widthStep);
        for(x=0; x<mask->width; x++) {
            if(ptr[x] != 0) { // valid point (mask)
                double mean = 0;
                double variance = 0;

                // get the mean
                for(q=0; q<n; q++) {
                    tmp = ac.image(q);
                    ptr2 = (unsigned short*)(tmp->imageData + y * tmp->widthStep);
                    mean += (v[q] = ptr2[x]);
                }
                mean /= (double)n;

                // get the standard deviation
                for(q=0; q<n; q++)
                    variance += (v[q] - mean) * (v[q] - mean);
                variance /= (double)n;

                // all right, plot the sd into img
                ptr2 = (unsigned short*)(img->imageData + y * img->widthStep);
                ptr2[x] = (unsigned short)sqrt(variance);
            }
        }
    }

    // done!
    delete[] v;
    return img;
}




CvMat* BackgroundModel::_computeHomography()
{
    int n = 4; // 4 vertices
    CvMat *src = cvCreateMat(n, 2, CV_32FC1);
    CvMat *dst = cvCreateMat(n, 2, CV_32FC1);
    CvMat *homography = cvCreateMat(3, 3, CV_32FC1);

    CV_MAT_ELEM(*src, float, 0, 0) = _vertex[0].x; // topleft
    CV_MAT_ELEM(*src, float, 0, 1) = _vertex[0].y;
    CV_MAT_ELEM(*src, float, 1, 0) = _vertex[1].x; // topright
    CV_MAT_ELEM(*src, float, 1, 1) = _vertex[1].y;
    CV_MAT_ELEM(*src, float, 2, 0) = _vertex[2].x; // bottomright
    CV_MAT_ELEM(*src, float, 2, 1) = _vertex[2].y;
    CV_MAT_ELEM(*src, float, 3, 0) = _vertex[3].x; // bottomleft
    CV_MAT_ELEM(*src, float, 3, 1) = _vertex[3].y;

    CV_MAT_ELEM(*dst, float, 0, 0) = 0.0f; // topleft
    CV_MAT_ELEM(*dst, float, 0, 1) = 0.0f;
    CV_MAT_ELEM(*dst, float, 1, 0) = 1.0f; // topright
    CV_MAT_ELEM(*dst, float, 1, 1) = 0.0f;
    CV_MAT_ELEM(*dst, float, 2, 0) = 1.0f; // bottomright
    CV_MAT_ELEM(*dst, float, 2, 1) = 1.0f;
    CV_MAT_ELEM(*dst, float, 3, 0) = 0.0f; // bottomleft
    CV_MAT_ELEM(*dst, float, 3, 1) = 1.0f;

    cvFindHomography(src, dst, homography);
    cvSave("data/bgmodel/homography.yaml", homography);

    cvReleaseMat(&src);
    cvReleaseMat(&dst);

    return homography;
}


void BackgroundModel::_compute3DVertices(const CalibratedKinect* kinect, const IplImage* image)
{
    for(int i=0; i<4; i++) {
        const unsigned short* ptr = (const unsigned short*)(image->imageData + _vertex[i].y * image->widthStep);
        _3dvertex[i].x = kinect->depthCoords2worldCoords(_vertex[i].x, _vertex[i].y, ptr[_vertex[i].x]).x;
        _3dvertex[i].y = kinect->depthCoords2worldCoords(_vertex[i].x, _vertex[i].y, ptr[_vertex[i].x]).y;
        _3dvertex[i].z = kinect->depthCoords2worldCoords(_vertex[i].x, _vertex[i].y, ptr[_vertex[i].x]).z;
    }
}
