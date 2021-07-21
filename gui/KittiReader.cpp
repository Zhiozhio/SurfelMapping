//
// Created by zhijun on 2021/4/22.
//

#include "KittiReader.h"

#include <sys/stat.h>
#include <fstream>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iomanip>

KittiReader::KittiReader(std::string datasetDir, bool estimateDepth, int subLevel, bool groundTruth)
: DatasetReader(datasetDir, estimateDepth),
  sub_level(subLevel)
{
    struct stat info;
    assert(stat(datasetDir_.c_str(), &info) == 0 && info.st_mode & S_IFDIR && "Dataset directory not exist.");  // judge if the directory exists

    // read timestamps
    std::ifstream timesIn(datasetDir_ + "/times.txt");
    double time_tmp;
    while(timesIn >> time_tmp)
        times.push_back(time_tmp);

    depthDir = datasetDir_ + "/PSMNet";
    rgbDir = datasetDir_ + "/image_2";

    bool tmp = loadCalibration();
    assert(tmp && "load calibration file failed!");

    if(groundTruth)
    {
        tmp = loadGroundTruth();
        assert(tmp && "load ground truth failed!");
    }
}

KittiReader::~KittiReader()
{
    delete [] depthReadBuffer;
    delete [] imageReadBuffer;

    delete groundTruth;
}

bool KittiReader::getNext()
{
    ++currentFrameId;

    time = times[currentFrameId];
    gtPose = (*groundTruth)[currentFrameId];

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(6) << currentFrameId;

    std::string current_depth_file(depthDir + "/" + ss.str() + ".png");
    std::string current_rgb_file(rgbDir + "/" + ss.str() + ".png");

    return getCore(current_depth_file, current_rgb_file);
}

bool KittiReader::getLast()
{
    --currentFrameId;

    time = times[currentFrameId];
    gtPose = (*groundTruth)[currentFrameId];

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(6) << currentFrameId;

    std::string current_depth_file(depthDir + "/" + ss.str() + ".png");
    std::string current_rgb_file(rgbDir + "/" + ss.str() + ".png");

    return getCore(current_depth_file, current_rgb_file);
}

bool KittiReader::getCore(const std::string &depth_file, const std::string &rgb_file)
{
    // read depth & rgb images
    cv::Mat dep_mat = cv::imread(depth_file, cv::IMREAD_ANYDEPTH);
    cv::Mat rgb_mat = cv::imread(rgb_file, cv::IMREAD_COLOR);
    if(dep_mat.data == nullptr || rgb_mat.data == nullptr)
        return false;

    assert(height_origin == dep_mat.rows && width_origin == dep_mat.cols && "depth sizes do not match!");
    assert(height_origin == rgb_mat.rows && width_origin == rgb_mat.cols && "RGB sizes do not match!");

    if(!depthReadBuffer && numPixels_origin > 0)
        depthReadBuffer = new unsigned char[numPixels_origin * 2];

    if(!imageReadBuffer && numPixels_origin > 0)
        imageReadBuffer = new unsigned char[numPixels_origin * 3];

    auto tmp = memcpy(depthReadBuffer, dep_mat.data, numPixels_origin * 2);
    assert(tmp && "depth copy failed!");
    depth = (unsigned short *)depthReadBuffer;

    tmp = memcpy(imageReadBuffer, rgb_mat.data, numPixels_origin * 3);
    assert(tmp && "image copy failed!");
    rgb = (unsigned char *)imageReadBuffer;

    if(sub_level)
    {
        unsigned short * depth_tmp = depth;
        unsigned char * rgb_tmp = rgb;

        depth = new unsigned short[numPixels_];
        rgb = new unsigned char[numPixels_ * 3];

        for(int row = 0; row < height_; ++row)
        {
            for(int col = 0; col < width_; ++col)
            {
                depth[row * width_ + col] = depth_tmp[2 * row * width_origin + 2 * col];

                rgb[(row * width_ + col) * 3 + 0] = rgb_tmp[(2 * row * width_origin + 2 * col) * 3 + 0];
                rgb[(row * width_ + col) * 3 + 1] = rgb_tmp[(2 * row * width_origin + 2 * col) * 3 + 1];
                rgb[(row * width_ + col) * 3 + 2] = rgb_tmp[(2 * row * width_origin + 2 * col) * 3 + 2];
            }
        }
    }

    // flip color as opencv use BGR format. Try to use other loader to save this cost.
    for(int i = 0; i < numPixels_ * 3; i += 3)
    {
        std::swap(rgb[i + 0], rgb[i + 2]);
    }

    return true;
}

bool KittiReader::loadCalibration()
{
    std::ifstream file(datasetDir_ + "/calibration.txt");
    std::string line;

    if(!file)
        return false;

    std::getline(file, line);

    if(!file)
        return false;

    int n = sscanf(line.c_str(), "%f %f %f %f", &fx_, &fy_, &cx_, &cy_);
    assert(n == 4 && "Ooops, your calibration file should contain the 1st line with fx fy cx cy!");

    std::getline(file, line);

    if(!file)
        return false;

    n = sscanf(line.c_str(), "%d %d", &width_, &height_);
    assert(n == 2 && "Ooops, your calibration file should contain the 2nd line with width height!");

    numPixels_ = width_ * height_;

    height_origin = height_;
    width_origin = width_;
    numPixels_origin = numPixels_;

    if(sub_level)
    {
        height_ = height_ >> sub_level;
        width_ = width_ >> sub_level;
        numPixels_ = height_ * width_;

        // todo that is not correct to odd W or H
        fx_ /= 2;
        fy_ /= 2;
        cx_ /= 2;
        cy_ /= 2;
    }

    return true;
}

bool KittiReader::loadGroundTruth()
{
    groundTruth = new std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>>;

    std::ifstream file(datasetDir_ + "/pose.txt");
    std::string line;
    float word;
    std::vector<float> results;
    Eigen::Matrix4f gtCam = Eigen::Matrix4f::Identity();

    while(std::getline(file, line))
    {
        std::stringstream ss(line);

        while(ss >> word)
            results.push_back(word);

        for(int i = 0; i < 3; ++i)
        {
            for(int j = 0; j < 4; ++j)
            {
                gtCam(i,j) = results[4*i + j];
            }
        }
        results.clear();

        Eigen::Matrix4f T20;
        T20 << 1, 0, 0, -0.06,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1;

        gtCam = gtCam * T20;

        groundTruth->push_back(gtCam);

    }

    assert(groundTruth->size() == times.size() && "ground truth and times not matched!");

    return true;
}

std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f> > * KittiReader::getGroundTruth()
{
    return groundTruth;
}
