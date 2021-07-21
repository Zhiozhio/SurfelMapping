//
// Created by zhijun on 2021/5/23.
//

#include "DatasetReader.h"


DatasetReader::DatasetReader(std::string datasetDir, bool estimateDepth, bool useSemantic)
: depth(nullptr),
  rgb(nullptr),
  depthReadBuffer(nullptr),
  imageReadBuffer(nullptr),
  semanticReadBuffer(nullptr),
  datasetDir_(std::move(datasetDir)),
  estimate_depth(estimateDepth),
  use_semantic(useSemantic),
  currentFrameId(-1),
  width_(0),
  height_(0),
  numPixels_(0),
  fx_(0.0),
  fy_(0.0),
  cx_(0.0),
  cy_(0.0)
{}

DatasetReader::~DatasetReader()
{
    delete [] depthReadBuffer;
    delete [] imageReadBuffer;
}

int DatasetReader::W()
{
    if(width_ == 0)
        std::cout << "WARNING: width may not be initialized!" << std::endl;
    return width_;
}

int DatasetReader::H()
{
    if(height_ == 0)
        std::cout << "WARNING: height may not be initialized!" << std::endl;
    return height_;
}

int DatasetReader::numPixels()
{
    if(numPixels_ == 0)
        std::cout << "WARNING: size may not be initialized!" << std::endl;
    return numPixels_;
}

float DatasetReader::fx()
{
    if(fx_ < 0.01)
        std::cout << "WARNING: camera intrinsics may not be initialized!" << std::endl;
    return fx_;
}

float DatasetReader::fy()
{
    if(fy_ < 0.01)
        std::cout << "WARNING: camera intrinsics may not be initialized!" << std::endl;
    return fy_;
}

float DatasetReader::cx()
{
    if(cx_ < 0.01)
        std::cout << "WARNING: camera intrinsics may not be initialized!" << std::endl;
    return cx_;
}

float DatasetReader::cy()
{
    if(cy_ < 0.01)
        std::cout << "WARNING: camera intrinsics may not be initialized!" << std::endl;
    return cy_;
}

bool DatasetReader::loadGroundTruth()
{}
