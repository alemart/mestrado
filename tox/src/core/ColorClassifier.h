#ifndef _COLORCLASSIFIER_H
#define _COLORCLASSIFIER_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include "ofMain.h"
#include "ofxOpenCv.h"

// this class classifies a cluster of
// ChromaPairs (i.e., assigns a color
// name to it)
//
// see also: elliptical boundary model

class ColorClassifier
{
public:
    typedef std::pair<unsigned char, unsigned char> ChromaPair; // chroma pair: 0 <= a <= 255 && 0 <= b <= 255
    typedef std::map<std::string, std::vector<ChromaPair> > KnownSampleSet; // (colorName, correspondingSamples)



    ColorClassifier();
    ~ColorClassifier();

    std::vector<std::string> knownColors(); // known colors (after training)
    void train(KnownSampleSet& trainingSet); // train the classifier
    std::string classify(const std::vector<ChromaPair>& cluster, double theta); // theta is a manually defined threshold
    std::string classify(ChromaPair color, double theta);
    std::vector<std::string> feasibleClasses(ChromaPair color, double theta);
    bool belongsToClass(std::string className, ChromaPair color, double theta);


    // auxiliary class: k-fold cross validation
    // for training & testing
    class CrossValidator
    {
    public:
        CrossValidator(int numberOfPartitions = 3) : _k(numberOfPartitions) { _loadDataFromDisk(); }
        ~CrossValidator() { }

        inline int numberOfPartitions() const { return _k; } // how many divisions does the set have?
        inline int partitionSize(const std::string& className) { return int(_data[className].size() / _k); }
        void retrieveTrainingSamples(KnownSampleSet& trainingSet, int partitionId = 0); // 0 <= partitionId < numberOfPartitions()
        void retrieveTestSamples(KnownSampleSet& testSet, int partitionId = 0); // 0 <= partitionId < numberOfPartitions()

    private:
        int _k;
        KnownSampleSet _data;

        void _loadDataFromDisk();
        void _loadChromaPairsFromFile(const std::string& path, std::vector<ChromaPair>& out);
    };




private:
    class ColorModel;

    // color models (the result of the training procedure)
    std::map<std::string, ColorModel*> _colorModels;
    void _clearModels();
    void _loadModelsFromDisk();
    void _deleteModelsFromDisk();

    // training
    void _trainModels(KnownSampleSet& ts);

    // cache
    std::vector<std::string> _knownColorsCache;







    // color models
    class ColorModel
    {
    public:
        ColorModel();
        ~ColorModel();

        void load(const std::string& modelName);
        void save(const std::string& modelName);

        void train(std::vector<ChromaPair>& samples, float percentageToBeRemoved = 0.05); // will remove low frequency samples...
        double evaluateChrominanceVector(ChromaPair vec) const;
        double detLambda() const; // determinant of lambda

    private:
        CvMat* _psi; // 2x1
        CvMat* _lambdaInv; // 2x2 (this is the inverse of lambda, lambda^(-1))
        int* _cnt; // absolute frequency of a training sample... this is an 1D array containing 256x256 entries

        void _clear();
        int _updateCntMatrix(const std::vector<ChromaPair>& samples); // returns the number of DISTINCTIVE training color vectors
        void _removeLowFrequencyEntries(std::vector<ChromaPair>& samples, float percentageToBeRemoved);

        struct Comparator {
            int *_cnt;
            Comparator(int* cnt) : _cnt(cnt) { }
            bool operator()(const ChromaPair& a, const ChromaPair& b) const {
                return _cnt[a.first * 256 + a.second] >= _cnt[b.first * 256 + b.second]; // low freq elements at the end
            }
        };
    };
};

#endif
