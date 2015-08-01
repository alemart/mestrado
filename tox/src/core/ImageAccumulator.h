#ifndef _IMAGEACCUMULATOR_H
#define _IMAGEACCUMULATOR_H

#include <vector>
#include <string>
#include "ofxOpenCv.h"

class ImageAccumulator
{
public:
    ImageAccumulator();
    ImageAccumulator(const std::string& sequenceName);
    virtual ~ImageAccumulator();

    // load & save to disk
    void load(const std::string& sequenceName);
    void save(const std::string& sequenceName);

    // utils
    void add(const IplImage* image);
    int count() const;
    void clear();

    // playback
    void rewind();
    const IplImage* next();
    const IplImage* current() const;

    // fast access
    inline const IplImage* image(int idx) const { return _images[idx]; }

private:
    std::vector<IplImage*> _images;
    int _ptr, _n;
};

#endif
