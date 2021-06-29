#include <utility>

//
// Created by zhijun on 2021/5/22.
//

#ifndef SURFELMAPPING_DATASETREADER_H
#define SURFELMAPPING_DATASETREADER_H

#include <iostream>
#include <vector>

class DatasetReader
{
public:
    DatasetReader(std::string datasetDir, bool estimateDepth);

    virtual ~DatasetReader();

    /**
     * Get next frame write to depth & rgb. If not return false.
     * @return if successfully get next
     */
    virtual bool getNext() = 0;

    /**
     * Get last frame write to depth & rgb. If not return false.
     * @return if successfully get next
     */
    virtual bool getLast() = 0;

    /**
     * Load calibration file.
     * @return if load successfully
     */
    virtual bool loadCalibration() = 0;

    /**
     * Load ground truth file. (Add proper member to store ground truth yourself)
     * @return if load successfully
     */
    virtual bool loadGroundTruth();

    int W();

    int H();

    int numPixels();

    float fx();

    float fy();

    float cx();

    float cy();


    unsigned short * depth;
    unsigned char * rgb;
    int currentFrameId;
    double time;


protected:
    unsigned char * depthReadBuffer;
    unsigned char * imageReadBuffer;

    const std::string datasetDir_;
    unsigned int width_;
    unsigned int height_;
    unsigned int numPixels_;
    float fx_, fy_, cx_, cy_;

    bool estimate_depth;
    std::vector<double> times;
};

#endif //SURFELMAPPING_DATASETREADER_H
