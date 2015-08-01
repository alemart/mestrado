#include <iostream>
#include <sstream>
#include "ColorClassifier.h"

ColorClassifier::ColorClassifier()
{
    _loadModelsFromDisk();
}

ColorClassifier::~ColorClassifier()
{
    _clearModels();
}

std::vector<std::string> ColorClassifier::knownColors()
{
    std::vector<std::string> &v = _knownColorsCache;

    if(v.size() == 0) {
        ofDirectory dir("color/");
        dir.allowExt("yaml");
        dir.listDir();

        for(int i=0; i<dir.numFiles(); i++) {
            std::string s(dir.getName(i));
            size_t idx = s.find("_psi");
            if(idx != std::string::npos)
                v.push_back(s.substr(0, idx));
        }
    }

    return v;
}

void ColorClassifier::train(KnownSampleSet& trainingSet)
{
    _clearModels();
    _deleteModelsFromDisk();
    _trainModels(trainingSet);
    _loadModelsFromDisk();
}

std::string ColorClassifier::classify(const std::vector<ColorClassifier::ChromaPair>& cluster, double theta)
{
    static std::vector<std::string> theKnownColors(knownColors());
    std::string color = "null";

    // initializing color densities...
    std::map<std::string, double> colorDensities;
    for(std::vector<std::string>::const_iterator it = theKnownColors.begin(); it != theKnownColors.end(); ++it)
        colorDensities.insert( std::pair<std::string, double>(*it, 0.0) );

    // for each known color class,
    for(std::vector<std::string>::const_iterator it = theKnownColors.begin(); it != theKnownColors.end(); ++it) {
        // check how many chroma pairs belong to that color class
        for(std::vector<ChromaPair>::const_iterator it2 = cluster.begin(); it2 != cluster.end(); ++it2) {
            // using mahalanobis distance
            if(_colorModels[*it]->evaluateChrominanceVector(*it2) < theta)
                colorDensities[*it] += 1.0;
        }
    }

    // compute the color densities
    for(std::vector<std::string>::const_iterator it = theKnownColors.begin(); it != theKnownColors.end(); ++it)
        colorDensities[*it] /= fabs( _colorModels[*it]->detLambda() ); // it's very important to take the absolute value!!!

    // what's the color having the largest density?
    double val = 0.0;
    for(std::vector<std::string>::const_iterator it = theKnownColors.begin(); it != theKnownColors.end(); ++it) {
        if(colorDensities[*it] > val) {
            val = colorDensities[*it];
            color = *it;
        }
    }

    // done!
    return color;
}

std::string ColorClassifier::classify(ColorClassifier::ChromaPair color, double theta)
{
    static std::vector<std::string> theKnownColors(knownColors());
    std::string result = "null";

    // initializing color densities...
    std::map<std::string, double> colorDensities;
    for(const std::string& knownColor : theKnownColors)
        colorDensities.insert( std::pair<std::string, double>(knownColor, 0.0) );

    // for each known color class,
    for(const std::string& knownColor : theKnownColors) {
        // add one unit to the density of the color; use mahalanobis distance
        if(_colorModels[knownColor]->evaluateChrominanceVector(color) < theta)
            colorDensities[knownColor] += 1.0;
    }

    // compute the color densities
    for(const std::string& knownColor : theKnownColors)
        colorDensities[knownColor] /= fabs( _colorModels[knownColor]->detLambda() ); // it's very important to take the absolute value!!!

    // what's the color having the largest density?
    double val = 0.0;
    for(const std::string& knownColor : theKnownColors) {
        if(colorDensities[knownColor] > val) {
            val = colorDensities[knownColor];
            result = knownColor;
        }
    }

    // done!
//std::cout << result << std::endl;
    return result;
}

std::vector<std::string> ColorClassifier::feasibleClasses(ColorClassifier::ChromaPair color, double theta)
{
    std::vector<std::string> v;
    static std::vector<std::string> theKnownColors( knownColors() );

    for(std::vector<std::string>::const_iterator it = theKnownColors.begin(); it != theKnownColors.end(); ++it) {
        if(_colorModels[*it]->evaluateChrominanceVector(color) < theta)
            v.push_back(*it);
    }

    return v;
}

bool ColorClassifier::belongsToClass(std::string className, ColorClassifier::ChromaPair color, double theta)
{
    if(_colorModels[className])
        return _colorModels[className]->evaluateChrominanceVector(color) < theta;
    else
        return false;
}





// ==============================================================





void ColorClassifier::_trainModels(KnownSampleSet& ts)
{
    for(KnownSampleSet::iterator it = ts.begin(); it != ts.end(); ++it) {
        ColorModel cm;
        std::cout << "training color model \"" << it->first << "\"... " << std::flush;
        cm.train(it->second);
        std::cout << "done! saving... " << std::flush;
        cm.save(it->first);
        std::cout << "done!" << std::endl;
    }
}

void ColorClassifier::_loadModelsFromDisk()
{
    _clearModels();
    std::vector<std::string> v = knownColors();
    for(std::vector<std::string>::iterator it = v.begin(); it != v.end(); ++it) {
        ColorModel* cm = new ColorModel();
        cm->load(*it);
        _colorModels.insert( std::pair<std::string,ColorModel*>( *it, cm ) );
    }
}

void ColorClassifier::_deleteModelsFromDisk()
{
    ofDirectory dir("color/");
    dir.allowExt("yaml");
    dir.listDir();

    for(int i=0; i<dir.numFiles(); i++)
        ofFile::removeFile(dir.getPath(i));
}

void ColorClassifier::_clearModels()
{
    for(std::map<std::string,ColorModel*>::iterator it = _colorModels.begin(); it != _colorModels.end(); ++it)
        delete it->second;
    _colorModels.clear();
}








//
// CrossValidator
//
void ColorClassifier::CrossValidator::retrieveTrainingSamples(KnownSampleSet& trainingSet, int partitionId)
{
    trainingSet.clear();

    if(partitionId < 0 || partitionId >= numberOfPartitions())
        return;

    for(KnownSampleSet::const_iterator it = _data.begin(); it != _data.end(); ++it) {
        std::string className(it->first);
        for(int j = partitionId * partitionSize(className); j < (1 + partitionId) * partitionSize(className); j++)
            trainingSet[className].push_back( (it->second).at(j) );

        std::cout << "class '" << className << "' got " << trainingSet[className].size() << " (out of " << it->second.size() << ") training samples." << std::endl;
    }
}

void ColorClassifier::CrossValidator::retrieveTestSamples(KnownSampleSet& testSet, int partitionId)
{
    testSet.clear();

    if(partitionId < 0 || partitionId >= numberOfPartitions())
        return;

    for(KnownSampleSet::const_iterator it = _data.begin(); it != _data.end(); ++it) {
        std::string className(it->first);
        testSet[className] = it->second;
        testSet[className].erase(
            testSet[className].begin() + partitionId * partitionSize(className),
            testSet[className].begin() + (1 + partitionId) * partitionSize(className)
        );

        std::cout << "class '" << className << "' got " << testSet[className].size() << " (out of " << it->second.size() << ") test samples." << std::endl;
    }
}

void ColorClassifier::CrossValidator::_loadDataFromDisk()
{
    _data.clear();

    ofDirectory dir("color/samples/");
    dir.allowExt("txt");
    dir.listDir();

    for(int i=0; i<dir.numFiles(); i++) {
        std::string s(dir.getName(i));
        unsigned idx = s.find(".");
        if(idx != std::string::npos) {
            std::string className(s.substr(0, idx));
            std::vector<ChromaPair> contents;

            _loadChromaPairsFromFile(std::string("data/color/samples/") + s, contents);
            for(int j=0; j<int(contents.size()); j++)
                _data[className].push_back( contents[j] );
        }
    }
    std::cout << "found " << _data.size() << " color classes." << std::endl;
}

void ColorClassifier::CrossValidator::_loadChromaPairsFromFile(const std::string& path, std::vector<ChromaPair>& out)
{
    ifstream f(path.c_str());
    if(f.is_open()) {
        int a, b;
        out.clear();
        while(f >> a >> b)
            out.push_back( ChromaPair((unsigned char)a, (unsigned char)b) );
    }
    else
        std::cerr << "ColorClassifier::CrossValidator::_loadChromaPairsFromFile() error - can't load '" << path << "'..." << std::endl;
}










//
// ColorModel
//

ColorClassifier::ColorModel::ColorModel() : _psi(0), _lambdaInv(0)
{
    _cnt = new int[65536];
}

ColorClassifier::ColorModel::~ColorModel()
{
    _clear();
    delete[] _cnt;
}

void ColorClassifier::ColorModel::_clear()
{
    if(_psi)
        cvReleaseMat(&_psi);

    if(_lambdaInv)
        cvReleaseMat(&_lambdaInv);

    _psi = 0;
    _lambdaInv = 0;
    //std::fill(_cnt, _cnt + 65536, 0);
}

int ColorClassifier::ColorModel::_updateCntMatrix(const std::vector<ColorClassifier::ChromaPair>& samples)
{
    int sum = 0;

    std::fill(_cnt, _cnt + 65536, 0);
    for(std::vector<ChromaPair>::const_iterator it = samples.begin(); it != samples.end(); ++it) {
        if(0 == _cnt[it->first * 256 + it->second]++)
            sum++;
    }

    return sum;
}

void ColorClassifier::ColorModel::_removeLowFrequencyEntries(std::vector<ChromaPair>& samples, float percentageToBeRemoved)
{
    Comparator cmp(this->_cnt);

    _clear();
    _updateCntMatrix(samples);

    std::stable_sort(samples.begin(), samples.end(), cmp); // sort() da crash; pq??
    int n = int(0.5f + samples.size() * percentageToBeRemoved);
    samples.erase(samples.end() - n, samples.end());
}

void ColorClassifier::ColorModel::load(const std::string& modelName)
{
    _clear();
    std::cout << "loading color model \"" << modelName << "\"... " << std::flush;
    _psi = (CvMat*)cvLoad((std::string("data/color/") + modelName + "_psi.yaml").c_str());
    _lambdaInv = (CvMat*)cvLoad((std::string("data/color/") + modelName + "_lambdaInv.yaml").c_str());
    std::cout << "done!" << std::endl;
    if(!_psi || !_lambdaInv)
        std::cerr << "ColorClassifier::ColorModel::load() error - can't load model \"" << modelName << "\"" << std::endl;
}

void ColorClassifier::ColorModel::save(const std::string& modelName)
{
    if(_psi && _lambdaInv) {
        cvSave((std::string("data/color/") + modelName + "_psi.yaml").c_str(), _psi);
        cvSave((std::string("data/color/") + modelName + "_lambdaInv.yaml").c_str(), _lambdaInv);
    }
    else
        std::cerr << "ColorClassifier::ColorModel::save() error - can't save model \"" << modelName << "\"" << std::endl;
}





double ColorClassifier::ColorModel::evaluateChrominanceVector(ChromaPair vec) const
{
    if(_psi && _lambdaInv) {
        static CvMat* v = cvCreateMat(2, 1, CV_32FC1);
        CV_MAT_ELEM(*v, float, 0, 0) = float(vec.first);
        CV_MAT_ELEM(*v, float, 1, 0) = float(vec.second);
        double d = cvMahalanobis(v, _psi, _lambdaInv);
        //cvReleaseMat(&v);
        return d;
    }
    else {
        std::cerr << "ColorClassifier::ColorModel::evaluateChrominanceVector() error - the color model isn't available!" << std::endl;
        return 0.0;
    }
}

double ColorClassifier::ColorModel::detLambda() const
{
    if(_lambdaInv) {
        double det = cvDet(_lambdaInv); // this is fast because _lambdaInv is 2x2
        if(fabs(det) > 1e-7)
            return 1.0 / det; // det(lambda) = 1 / det(lambda^(-1))
    }

    std::cerr << "ColorClassifier::ColorModel::detLambda() error - invalid lambda matrix!" << std::endl;
    return 0.0;
}

void ColorClassifier::ColorModel::train(std::vector<ChromaPair>& samples, float percentageToBeRemoved)
{
    int i, j, f_i;

    // pre-processing ...
    _clear();
    _removeLowFrequencyEntries(samples, percentageToBeRemoved);

    // creating the matrices...
    _psi = cvCreateMat(2, 1, CV_32FC1);
    _lambdaInv = cvCreateMat(2, 2, CV_32FC1);
    CvMat* mu = cvCreateMat(2, 1, CV_32FC1);
    CvMat* tmp2x2 = cvCreateMat(2, 2, CV_32FC1);
    CvMat* tmp2x1 = cvCreateMat(2, 1, CV_32FC1);

    // neat numbers
    int N = int(samples.size()); // N = sum f_i is the total number of samples in the preprocessed training data set
    if(N == 0) {
        std::cerr << "ColorClassifier::ColorModel::train() error - the preprocessed training data set is empty!" << std::endl;
        return;
    }

    int n = _updateCntMatrix(samples); // number of distinctive color vectors
    if(n == 0) {
        std::cerr << "ColorClassifier::ColorModel::train() error - n is zero!" << std::endl;
        return;
    }

    // computing mu
    cvZero(mu);
    for(i=0; i<256; i++) {
        for(j=0; j<256; j++) {
            if(0 < (f_i = _cnt[i * 256 + j])) {
                CV_MAT_ELEM(*mu, float, 0, 0) += float(f_i * i);
                CV_MAT_ELEM(*mu, float, 1, 0) += float(f_i * j);
            }
        }
    }
    CV_MAT_ELEM(*mu, float, 0, 0) /= float(N);
    CV_MAT_ELEM(*mu, float, 1, 0) /= float(N);

    // computing psi
    cvZero(_psi);
    for(i=0; i<256; i++) {
        for(j=0; j<256; j++) {
            if(0 < _cnt[i * 256 + j]) {
                CV_MAT_ELEM(*_psi, float, 0, 0) += float(i);
                CV_MAT_ELEM(*_psi, float, 1, 0) += float(j);
            }
        }
    }
    CV_MAT_ELEM(*_psi, float, 0, 0) /= float(n);
    CV_MAT_ELEM(*_psi, float, 1, 0) /= float(n);

    // computing lambda^(-1)
    cvZero(_lambdaInv);
    for(i=0; i<256; i++) {
        for(j=0; j<256; j++) {
            if(0 < (f_i = _cnt[i * 256 + j])) {
                // tmp2x1 := X_i - mu
                CV_MAT_ELEM(*tmp2x1, float, 0, 0) = float(i) - CV_MAT_ELEM(*mu, float, 0, 0);
                CV_MAT_ELEM(*tmp2x1, float, 1, 0) = float(j) - CV_MAT_ELEM(*mu, float, 1, 0);

                // tmp2x2 := f_i * (X_i - mu) * (X_i - mu)^t;
                cvGEMM(tmp2x1, tmp2x1, f_i, NULL, 0, tmp2x2, CV_GEMM_B_T);

                // lambda += tmp2x2
                cvAdd(_lambdaInv, tmp2x2, _lambdaInv, NULL);
            }
        }
    }
    cvScale(_lambdaInv, _lambdaInv, 1.0f / float(N));
    cvInvert(_lambdaInv, _lambdaInv, CV_SVD);

    // done!
    cvReleaseMat(&tmp2x1);
    cvReleaseMat(&tmp2x2);
    cvReleaseMat(&mu);
}
