#ifndef _IMAGELABELER_H
#define _IMAGELABELER_H

#include <string>
#include <set>
#include <map>
#include <vector>
#include "ofMain.h"
#include "ofxOpenCv.h"
#include "cvblob.h"

class ColorClassifier;
class CalibratedKinect;


class ImageLabeler
{
public:
    ImageLabeler(int width, int height, std::string color = "yellow"); // color can be "yellow" or "blue"
    ~ImageLabeler();

    class ConnectedComponent
    {
    public:
        ConnectedComponent(ImageLabeler* il) : area(0), _il(il) { }
        ConnectedComponent(const ConnectedComponent& cc) : centerOfMass(cc.centerOfMass), area(cc.area), classCount(cc.classCount), pixels(cc.pixels), _il(cc._il) { }
        ~ConnectedComponent() { };

        typedef int Pixel;
        inline Pixel packPixel(int x, int y) const { return y * _il->_transformedColorImage->width + x; }
        inline void unpackPixel(Pixel px, int* x, int* y) const { *x = px % _il->_transformedColorImage->width; *y = px / _il->_transformedColorImage->width; }

        ofVec2f centerOfMass;
        int area;
        std::map<std::string, int> classCount; // counts the number of pixels of given classes: marker_blue, finger, etc.
        std::set<Pixel> pixels;

        inline std::string dominantClass() {
            int m = 0; std::string s;
            for(auto it = classCount.begin(); it != classCount.end(); ++it) {
                if(it->second > m) {
                    s = it->first;
                    m = it->second;
                }
            }
            return s;
        }

    private:
        ImageLabeler* _il;
    };
    friend class ConnectedComponent;

    void update(ofxCvColorImage* foreground, double theta = 7); // call every frame
    ConnectedComponent getBlobAt(ofVec2f position, std::set<std::string> acceptableClasses, int searchRadius = 20); // call after update()
    ConnectedComponent getBlobAt(ofVec2f position, std::string acceptableClass = "", int searchRadius = 20); // only one acceptable class?
    std::vector<ConnectedComponent> getAllBlobs(std::set<std::string> acceptableClasses, int searchRadius = 20); // call after update()
    std::vector<ConnectedComponent> getAllBlobs(std::string acceptableClass, int searchRadius = 20); // call after update()
    inline ofxCvGrayscaleImage* blobImage() const { return (ofxCvGrayscaleImage*)&_binaryImg; }

private:
    ColorClassifier* _colorClassifier;
    IplImage* _transformedColorImage;
    IplImage* _hsvImage;
    bool* _visited;
    int* _parent;
    double _theta;

    IplImage* _labelImg;
    ofxCvGrayscaleImage _binaryImg;
    cvb::CvBlobs _cvb;
    std::string _color;

    inline std::string _namefix(std::string s) const;
    inline std::string _pixelClass(int x, int y) const;
    ConnectedComponent _getBlobAt(ofVec2f position, std::set<std::string> acceptableClasses, int searchRadius = 20);
};

#endif
