#include <cmath>
#include <queue>
#include "ImageLabeler.h"
#include "ColorClassifier.h"
#include "CalibratedKinect.h"

//#define USE_LEE_YOO_COLOR_CLASSIFIER // use a slower classifier
#define BLOB_MIN_AREA 150 // in pixels

ImageLabeler::ImageLabeler(int width, int height, std::string color) : _color(color)
{
    _colorClassifier = new ColorClassifier();
    _transformedColorImage = cvCreateImage(
        cvSize(width, height),
        IPL_DEPTH_8U,
        3
    );
    _hsvImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 3);

    _visited = new bool[ width * height ];
    _parent = new int[ width * height ];

    _labelImg = cvCreateImage(cvSize(width, height), IPL_DEPTH_LABEL, 1);
    _binaryImg.allocate(width, height);
}

ImageLabeler::~ImageLabeler()
{
    delete[] _parent;
    delete[] _visited;
    cvReleaseImage(&_hsvImage);
    cvReleaseImage(&_transformedColorImage);
    delete _colorClassifier;
    cvReleaseImage(&_labelImg);
}

void ImageLabeler::update(ofxCvColorImage* foreground, double theta) // theta comes from the elliptical boundary model, see touchdetector.cpp for more info
{
    int width = _transformedColorImage->width;
    int height = _transformedColorImage->height;

    // update theta
    _theta = theta;

    // binary image
    IplImage* _binary = _binaryImg.getCvImage();
#ifdef USE_LEE_YOO_COLOR_CLASSIFIER
    cvCvtColor(foreground->getCvImage(), _transformedColorImage, CV_RGB2Lab);
    for(int y=0; y<height; y++) {
        const unsigned char* fg = (const unsigned char*)(foreground->getCvImage()->imageData + y * foreground->getCvImage()->widthStep);
        unsigned char* bin = (unsigned char*)(_binary->imageData + y * _binary->widthStep);
        for(int x=0; x<width; x++) {
            bin[x] = 0;
            if(fg[3*x] > 128 && fg[3*x+1] > 128 && fg[3*x+2] < 128) { // yellowish (fast checking) ::: FIXME
                if(_pixelClass(x, y).substr(0, 6) == "marker") // worsens the signal (also, this is slow)
                    bin[x] = 255;
            }
        }
    }
#else
    cvCvtColor(foreground->getCvImage(), _hsvImage, CV_RGB2HSV);
    if(_color == "blue")
        cvInRangeS(_hsvImage, cvScalar(100,150,64), cvScalar(140,255,255), _binary); // filter blue, from http://stackoverflow.com/questions/17878254/opencv-python-cant-detect-blue-objects
    else
        cvInRangeS(_hsvImage, cvScalar(20,100,100), cvScalar(40,255,255), _binary); // filter yellow, from http://www.aishack.in/tutorials/tracking-colored-objects-in-opencv/, http://stackoverflow.com/questions/9179189/detect-yellow-color-in-opencv
    cvErode(_binary, _binary, NULL, 1);
#endif
    cvSmooth(_binary, _binary, CV_MEDIAN, 3);
    //cvSmooth(_binary, _binary, CV_MEDIAN, 3);
    //cvErode(_binary, _binary, NULL, 1);
    //cvDilate(_binary, _binary, NULL, 2);
    cvDilate(_binary, _binary, NULL, 2);
    _binaryImg.flagImageChanged();

    // get blobs
    cvReleaseBlobs(_cvb);
    cvb::cvLabel(_binary, _labelImg, _cvb);
    cvFilterByArea(_cvb, BLOB_MIN_AREA, 99999);

    // clear stuff
    std::fill(_visited, _visited + width * height, false);
    //std::fill(_parent, _parent + width * height, -1);
}

ImageLabeler::ConnectedComponent ImageLabeler::getBlobAt(ofVec2f position, std::string acceptableClass, int searchRadius)
{
    std::set<std::string> s;
    if(acceptableClass != "")
        s.insert(acceptableClass);
    return getBlobAt(position, s, searchRadius);
}

std::vector<ImageLabeler::ConnectedComponent> ImageLabeler::getAllBlobs(std::string acceptableClass, int searchRadius)
{
    std::set<std::string> s;
    if(acceptableClass != "")
        s.insert(acceptableClass);
    return getAllBlobs(s, searchRadius);
}

std::vector<ImageLabeler::ConnectedComponent> ImageLabeler::getAllBlobs(std::set<std::string> acceptableClasses, int searchRadius)
{
    std::vector<ImageLabeler::ConnectedComponent> result;

    for(cvb::CvBlobs::const_iterator it = _cvb.begin(); it != _cvb.end(); ++it) {
        cvb::CvBlob* blob = it->second;
        if(blob->area >= 50) {
            ofVec2f centroid(blob->centroid.x, blob->centroid.y);
            ImageLabeler::ConnectedComponent cc = getBlobAt(centroid, acceptableClasses, searchRadius);
            result.push_back(cc);
        }
    }

    return result;
}

// this routine is SLOW !!!!!!!! avoid it whenever possible ...
/*
std::vector<ImageLabeler::ConnectedComponent> ImageLabeler::getAllBlobs(std::set<std::string> acceptableClasses)
{
    ConnectedComponent cc(this);
    int width = _transformedColorImage->width;
    int height = _transformedColorImage->height;
    std::vector<ConnectedComponent> result;
    auto acceptablePixel = [&](int x, int y) -> bool {
        unsigned char* data = (unsigned char*)(_transformedColorImage->imageData + y * _transformedColorImage->widthStep);
        return (data[3*x] > 0) && acceptableClasses.count(_pixelClass(x, y)) > 0;
    };
    //std::set<ConnectedComponent::Pixel> retrievedRoots;
    //auto rootOf = [&](int x, int y) -> ConnectedComponent::Pixel {
    //    ConnectedComponent::Pixel px = cc.packPixel(x, y);
    //    while(_parent[px] != px) { // assuming _parent[px] != -1, meaning that _visited[px] = true
    //        px = _parent[px];
    //    }
    //    return px;
    //};

    std::fill(_visited, _visited + width * height, false);
    //std::fill(_parent, _parent + width * height, -1);

    for(int y=0; y<height; y++) {
        for(int x=0; x<width; x++) {
            if(!_visited[cc.packPixel(x, y)] && acceptablePixel(x, y))
                result.push_back(_getBlobAt(ofVec2f(x, y), acceptableClasses));
        }
    }

    return result;
}
*/

ImageLabeler::ConnectedComponent ImageLabeler::getBlobAt(ofVec2f position, std::set<std::string> acceptableClasses, int searchRadius)
{
    return _getBlobAt(position, acceptableClasses, searchRadius); // here's an intentional "bug", for increased efficiency
    int width = _transformedColorImage->width;
    int height = _transformedColorImage->height;
    std::fill(_visited, _visited + width * height, false);
    //std::fill(_parent, _parent + width * height, -1);
    return _getBlobAt(position, acceptableClasses, searchRadius);
}

ImageLabeler::ConnectedComponent ImageLabeler::_getBlobAt(ofVec2f position, std::set<std::string> acceptableClasses, int searchRadius)
{
    ConnectedComponent::Pixel px;
    int x, y;
    int width = _transformedColorImage->width;
    int height = _transformedColorImage->height;
    float xsum = 0.0f, ysum = 0.0f, cnt = 0.0f;
    /*auto belongsToClass = [&](std::string className, int x, int y) -> bool {
        return _pixelClass(x, y) == _namefix(className);
        //if(x >= 0 && x < width && y >= 0 && y < height) {
        //    unsigned char* data = (unsigned char*)(_transformedColorImage->imageData + y * _transformedColorImage->widthStep);
        //    ColorClassifier::ChromaPair color(data[3*x+1], data[3*x+2]);
        //    return _colorClassifier->belongsToClass(_namefix(className), color, _theta);
        //}
        //else
        //    return false;
    };*/
    auto acceptablePixel = [&](int x, int y) -> bool {
#ifdef USE_LEE_YOO_COLOR_CLASSIFIER
        return acceptableClasses.count(_pixelClass(x, y)) > 0;
#else
        IplImage* binary = _binaryImg.getCvImage();
        if(x >= 0 && x < binary->width && y >= 0 && y < binary->height) {
            const unsigned char* img = (const unsigned char*)(binary->imageData + y * binary->widthStep);
            return img[x] > 0;
        }
        else
            return false;
#endif
    };

    std::queue<int> q;
    ConnectedComponent component(this);

    // preliminary stuff...
    x = position.x;
    y = position.y;
    if(!(x >= 0 && x < width && y >= 0 && y < height)) {
        component.centerOfMass = position;
        return component;
    }

    // fix names
    std::set<std::string> tmp;
    for(std::set<std::string>::iterator it = acceptableClasses.begin(); it != acceptableClasses.end(); ++it)
        tmp.insert(_namefix(*it));
    acceptableClasses = tmp;

    // look for an acceptable pixel (x,y) nearby
    if(acceptableClasses.size() > 0) {
        if(!acceptablePixel(x, y)) {
            bool done = false;
            for(int length = 0; length <= searchRadius && !done; length++) {
                for(int angle = 0; angle < 360 && !done; angle += 30) {
                    float ang = angle * 3.14159f / 180.0f;
                    int px = x + length * std::cos(ang);
                    int py = y - length * std::sin(ang);
                    if(px >= 0 && px < width && py >= 0 && py < height) {
                        if(acceptablePixel(px, py)) {
                            // found it
                            x = px;
                            y = py;
                            position.x = x;
                            position.y = y;
                            done = true;
                        }
                    }
                }
            }
            if(!done) {
                // error, can't find it
                component.centerOfMass = position;
                return component;
            }
        }
    }
    else
        acceptableClasses.insert(_pixelClass(x, y));

    // floodfill
    px = component.packPixel(x, y);
    q.push(px);
    _visited[px] = true;
    //_parent[px] = px; // root of the tree
    while(!q.empty()) {
        px = q.front(); // current pixel
        q.pop();

        // visiting the current pixel...
        component.unpackPixel(px, &x, &y);
        xsum += x;
        ysum += y;
        cnt += 1.0f;
        component.pixels.insert(px);
        component.classCount[_pixelClass(x, y)]++;

        // 4(8?)-connected neighbors
        if(x > 0 && !_visited[component.packPixel(x-1, y)] && acceptablePixel(x-1, y)) {
            q.push( component.packPixel(x-1,y) );
            _visited[ component.packPixel(x-1,y) ] = true;
            //_parent[ component.packPixel(x-1,y) ] = px;
        }
        if(x < width-1 && !_visited[component.packPixel(x+1, y)] && acceptablePixel(x+1, y)) {
            q.push( component.packPixel(x+1,y) );
            _visited[ component.packPixel(x+1,y) ] = true;
            //_parent[ component.packPixel(x+1,y) ] = px;
        }
        if(y > 0 && !_visited[component.packPixel(x, y-1)] && acceptablePixel(x, y-1)) {
            q.push( component.packPixel(x,y-1) );
            _visited[ component.packPixel(x,y-1) ] = true;
            //_parent[ component.packPixel(x,y-1) ] = px;
        }
        if(y < height-1 && !_visited[component.packPixel(x, y+1)] && acceptablePixel(x, y+1)) {
            q.push( component.packPixel(x,y+1) );
            _visited[ component.packPixel(x,y+1) ] = true;
            //_parent[ component.packPixel(x,y+1) ] = px;
        }

        if(x > 0 && y > 0 && !_visited[component.packPixel(x-1, y-1)] && acceptablePixel(x-1, y-1)) {
            q.push( component.packPixel(x-1,y-1) );
            _visited[ component.packPixel(x-1,y-1) ] = true;
            //_parent[ component.packPixel(x-1,y-1) ] = px;
        }
        if(x < width-1 && y > 0 && !_visited[component.packPixel(x+1, y-1)] && acceptablePixel(x+1, y-1)) {
            q.push( component.packPixel(x+1,y-1) );
            _visited[ component.packPixel(x+1,y-1) ] = true;
            //_parent[ component.packPixel(x+1,y-1) ] = px;
        }
        if(x < width-1 && y < height-1 && !_visited[component.packPixel(x+1, y+1)] && acceptablePixel(x, y+1)) {
            q.push( component.packPixel(x+1,y+1) );
            _visited[ component.packPixel(x+1,y+1) ] = true;
            //_parent[ component.packPixel(x+1,y+1) ] = px;
        }
        if(x > 0 && y < height-1 && !_visited[component.packPixel(x-1, y+1)] && acceptablePixel(x-1, y+1)) {
            q.push( component.packPixel(x-1,y+1) );
            _visited[ component.packPixel(x-1,y+1) ] = true;
            //_parent[ component.packPixel(x-1,y+1) ] = px;
        }
    }

    // find the center of mass and the area
    if(cnt > 0.0f)
        component.centerOfMass = ofVec2f(xsum / cnt, ysum / cnt);
    component.area = int(cnt);

    // done!
    return component;
}

std::string ImageLabeler::_namefix(std::string s) const
{
    if(s.substr(0, 9) == "projetor_")
        return s.substr(9);
    else if(s == "eraser_green")
        return "marker_green";
    else
        return s;
}

std::string ImageLabeler::_pixelClass(int x, int y) const
{
/*
    int width = _transformedColorImage->width;
    int height = _transformedColorImage->height;

    if(x >= 0 && x < width && y >= 0 && y < height) {
        unsigned char* data = (unsigned char*)(_transformedColorImage->imageData + y * _transformedColorImage->widthStep);
        ColorClassifier::ChromaPair color(data[3*x+1], data[3*x+2]);
        return _namefix(_colorClassifier->classify(color, _theta));
    }
    else
        return "null";
*/
    const unsigned char* data = (const unsigned char*)(_transformedColorImage->imageData + y * _transformedColorImage->widthStep);
    ColorClassifier::ChromaPair color(data[3*x+1], data[3*x+2]);
    return _namefix(_colorClassifier->classify(color, _theta));
}
