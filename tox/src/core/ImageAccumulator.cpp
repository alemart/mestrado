#include <sstream>
#include <iostream>
#include <sys/stat.h>
#include "ImageAccumulator.h"
#include <opencv/highgui.h>



static bool mkpath(std::string path);


// ----------


ImageAccumulator::ImageAccumulator() : _ptr(0)
{
}

ImageAccumulator::ImageAccumulator(const std::string& sequenceName) : _ptr(0)
{
    load(sequenceName);
}

ImageAccumulator::~ImageAccumulator()
{
    clear();
}

void ImageAccumulator::clear()
{
    for(std::vector<IplImage*>::iterator it = _images.begin(); it != _images.end(); ++it) {
        IplImage *img = *it;
        cvReleaseImage(&img);
    }

    _images.clear();
    _ptr = 0;
}

void ImageAccumulator::add(const IplImage* image)
{
    _images.push_back( cvCloneImage(image) );
}

void ImageAccumulator::save(const std::string& sequenceName)
{
    int cnt = 0;

    std::cerr << "ImageAccumulator: saving sequence '" << sequenceName << "'..." << std::endl;

    for(std::vector<IplImage*>::iterator it = _images.begin(); it != _images.end(); ++it) {
        std::stringstream ss;
        ss << "data/capture/" << sequenceName << "/";
        mkpath(ss.str());
        ss << (++cnt);

        if(!((*it)->nChannels == 3 && cvSaveImage((ss.str() + ".jpg").c_str(), *it))) {
            if(!((*it)->nChannels == 3 && cvSaveImage((ss.str() + ".png").c_str(), *it)))
                cvSave((ss.str() + ".yaml").c_str(), *it);
        }
    }
}

void ImageAccumulator::load(const std::string& sequenceName)
{
    IplImage* img;
    int cnt = 0;

    std::cerr << "ImageAccumulator: loading sequence '" << sequenceName << "'..." << std::endl;

    clear();
    for(;;) {
        std::stringstream ss;
        if(sequenceName[0] != '/' && sequenceName[0] != '.') // not(absolute or relative path) ?
            ss << "data/capture/" << sequenceName << "/";
        else
            ss << sequenceName << "/";
        ss << (++cnt);

        if(!(img = cvLoadImage((ss.str() + ".jpg").c_str()))) {
            if(!(img = cvLoadImage((ss.str() + ".png").c_str())))
                img = (IplImage*)cvLoad((ss.str() + ".yaml").c_str());
        }

        if(img) {
            add(img);
            cvReleaseImage(&img);
        }
        else {
            std::cerr << "Can't load image \"" << ss.str() << ".[jpg|png|yaml]\"..." << std::endl;
            break;
        }
    }

    if(count() == 0)
        std::cerr << "ImageAccumulator: can't load sequence '" << sequenceName << "'." << std::endl;
}

int ImageAccumulator::count() const
{
    return _images.size();
}


void ImageAccumulator::rewind()
{
    _ptr = 0;
}

const IplImage* ImageAccumulator::next()
{
    if(_ptr >= 0 && _ptr < int(_images.size())) {
        const IplImage* img = _images[_ptr];
        _ptr = (_ptr + 1) % _images.size();
        return img;
    }
    else
        return 0;
}

const IplImage* ImageAccumulator::current() const
{
    if(_ptr >= 0 && _ptr < int(_images.size()))
        return _images[_ptr];
    else
        return 0;
}





// --------------------

// the following function...
// is from...
// http://stackoverflow.com/questions/675039/how-can-i-create-directory-tree-in-c-linux
bool mkpath( std::string path )
{
    bool bSuccess = false;
    int nRC = ::mkdir( path.c_str(), 0775 );
    if( nRC == -1 )
    {
        switch( errno )
        {
            case ENOENT:
                //parent didn't exist, try to create it
                if( mkpath( path.substr(0, path.find_last_of('/')) ) )
                    //Now, try to create again.
                    bSuccess = 0 == ::mkdir( path.c_str(), 0775 );
                else
                    bSuccess = false;
                break;
            case EEXIST:
                //Done!
                bSuccess = true;
                break;
            default:
                bSuccess = false;
                break;
        }
    }
    else
        bSuccess = true;
    return bSuccess;
}
