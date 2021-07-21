//
// Created by zhijun on 2021/4/22.
//

#ifndef ELASTICFUSION_DATASETREADER_H
#define ELASTICFUSION_DATASETREADER_H


#include "DatasetReader.h"

#include <Eigen/Core>

/**
 * DatasetReader for load data from KITTI dataset.
 * the depth in uint16 represents (mm) of the absolute depth.
 * depth range is clipped to [0, 60]
 */
class KittiReader : public DatasetReader
{
public:
    KittiReader(std::string datasetDir, bool estimateDepth, bool useSemantic, int subLevel, bool groundTruth);

    ~KittiReader() override;

    bool getNext() override;

    bool getLast() override;

    bool loadCalibration() final;

    bool loadGroundTruth() final;


    Eigen::Matrix4f gtPose;

    std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>> * getGroundTruth();

private:
    unsigned int sub_level;
    unsigned int height_origin;
    unsigned int width_origin;
    unsigned int numPixels_origin;

    std::string depthDir;
    std::string rgbDir;
    std::string semanticDir;
    std::vector<Eigen::Matrix4f, Eigen::aligned_allocator<Eigen::Matrix4f>> * groundTruth;

    bool getCore(const std::string &depth_file, const std::string &rgb_file, const std::string &semantic_file);
};

#endif //ELASTICFUSION_DATASETREADER_H
