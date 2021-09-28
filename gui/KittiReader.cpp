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

KittiReader::KittiReader(std::string datasetDir, bool estimateDepth, bool useSemantic, int subLevel, bool groundTruth)
: DatasetReader(datasetDir, estimateDepth, useSemantic),
  sub_level(subLevel)
{
    struct stat info;
    assert(stat(datasetDir_.c_str(), &info) == 0 && info.st_mode & S_IFDIR && "Dataset directory not exist.");  // judge if the directory exists

    // read timestamps
    std::ifstream timesIn(datasetDir_ + "/times.txt");
    double time_tmp;
    while(timesIn >> time_tmp)
        times.push_back(time_tmp);

    // depth path
    depthDir = datasetDir_ + "/PSMNet";
    // RGB path
    rgbDir = datasetDir_ + "/image_2";

    bool tmp = loadCalibration();
    assert(tmp && "load calibration file failed!");

    if(groundTruth)
    {
        tmp = loadGroundTruth();
        assert(tmp && "load ground truth failed!");
    }

    if(useSemantic)
    {
        // semantic path
        semanticDir = datasetDir_ + "/semantics";
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

    std::string current_rgb_file(rgbDir + "/" + ss.str() + ".png");

    std::string current_depth_file = estimate_depth ? "" : depthDir + "/" + ss.str() + ".png";

    std::string current_semantic_file = use_semantic ? semanticDir + "/" + ss.str() + ".png" : "";

    return getCore(current_depth_file, current_rgb_file, current_semantic_file);
}

bool KittiReader::getLast()
{
    --currentFrameId;

    time = times[currentFrameId];
    gtPose = (*groundTruth)[currentFrameId];

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(6) << currentFrameId;

    std::string current_rgb_file(rgbDir + "/" + ss.str() + ".png");

    std::string current_depth_file = estimate_depth ? "" : depthDir + "/" + ss.str() + ".png";

    std::string current_semantic_file = use_semantic ? semanticDir + "/" + ss.str() + ".png" : "";

    return getCore(current_depth_file, current_rgb_file, current_semantic_file);
}

bool KittiReader::getCore(const std::string &depth_file, const std::string &rgb_file, const std::string &semantic_file)
{
    // rgb images
    cv::Mat rgb_mat = cv::imread(rgb_file, cv::IMREAD_COLOR);
    if(rgb_mat.data == nullptr)
    {
        printf("CANNOT read RGB image from %s", rgb_file.c_str());
        return false;
    }

    assert(height_origin == rgb_mat.rows && width_origin == rgb_mat.cols && "RGB sizes do not match!");

    if(!imageReadBuffer && numPixels_origin > 0)
        imageReadBuffer = new unsigned char[numPixels_origin * 3];

    auto tmp = memcpy(imageReadBuffer, rgb_mat.data, numPixels_origin * 3);
    assert(tmp && "image copy failed!");
    rgb = (unsigned char *)imageReadBuffer;

    if(sub_level)
    {
        unsigned char * rgb_tmp = rgb;

        rgb = new unsigned char[numPixels_ * 3];

        for(int row = 0; row < height_; ++row)
        {
            for(int col = 0; col < width_; ++col)
            {
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

    // read depth file
    if(!depth_file.empty())
    {
        cv::Mat dep_mat = cv::imread(depth_file, cv::IMREAD_ANYDEPTH);
        if(dep_mat.data == nullptr)
        {
            printf("CANNOT read depth image from %s", depth_file.c_str());
            return false;
        }

        assert(height_origin == dep_mat.rows && width_origin == dep_mat.cols && "depth sizes do not match!");

        if(!depthReadBuffer && numPixels_origin > 0)
            depthReadBuffer = new unsigned char[numPixels_origin * 2];

        tmp = memcpy(depthReadBuffer, dep_mat.data, numPixels_origin * 2);
        assert(tmp && "depth copy failed!");
        depth = (unsigned short *)depthReadBuffer;

        if(sub_level)
        {
            unsigned short * depth_tmp = depth;

            depth = new unsigned short[numPixels_];

            for(int row = 0; row < height_; ++row)
            {
                for(int col = 0; col < width_; ++col)
                {
                    depth[row * width_ + col] = depth_tmp[2 * row * width_origin + 2 * col];
                }
            }
        }
    }

    // read semantic file
    if(!semantic_file.empty())
    {
        cv::Mat sem_mat = cv::imread(semantic_file, cv::IMREAD_ANYDEPTH);
        if(sem_mat.data == nullptr)
        {
            printf("CANNOT read semantic image from %s", semantic_file.c_str());
            return false;
        }

        assert(height_origin == sem_mat.rows && width_origin == sem_mat.cols && "semantic sizes do not match!");

//        for(int i = 0; i < numPixels_origin; ++i)
//        {
//            unsigned char a = sem_mat.data[i];
//            unsigned int b = (unsigned int)sem_mat.data[i];
//            sem_mat.data[i] *= 10;
//        }
//        cv::imshow("semantic", sem_mat);
//        cv::waitKey(0);

        if(!semanticReadBuffer && numPixels_origin > 0)
            semanticReadBuffer = new unsigned char[numPixels_origin];

        tmp = memcpy(semanticReadBuffer, sem_mat.data, numPixels_origin);
        assert(tmp && "semantic copy failed!");
        semantic = (unsigned char *)semanticReadBuffer;

        if(sub_level)
        {
            unsigned char * semantic_tmp = semantic;

            semantic = new unsigned char[numPixels_];

            for(int row = 0; row < height_; ++row)
            {
                for(int col = 0; col < width_; ++col)
                {
                    depth[row * width_ + col] = semantic_tmp[2 * row * width_origin + 2 * col];
                }
            }
        }
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
