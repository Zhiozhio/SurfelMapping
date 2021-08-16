//
// Created by zhijun on 2021/5/23.
//

#ifndef SURFELMAPPING_SURFELMAPPING_H
#define SURFELMAPPING_SURFELMAPPING_H

#include "Config.h"
#include "IndexMap.h"
#include "GlobalModel.h"
#include "Shaders.h"
#include "GPUTexture.h"
#include "FeedbackBuffer.h"
#include "ComputePack.h"
#include "Checker.h"


class SurfelMapping
{
public:
    SurfelMapping();

    virtual ~SurfelMapping();

    /**
     * Process an rgb/depth map pair
     * @param rgb unsigned char row major order
     * @param depth unsigned short z-depth in millimeters, invalid depths are 0
     * @param gtPose optional input SE3 pose (if provided, we don't attempt to perform tracking)
     */
    void processFrame(const unsigned char * rgb,
                      const unsigned short * depth = nullptr,
                      const unsigned char * semantic = nullptr,
                      const Eigen::Matrix4f * gtPose = 0);

    /**
     * Predicts the current view of the scene, updates the [vertex/normal/image]Tex() members
     * of the indexMap class
     */
    void predict();

    /**
     * This class contains all of the predicted renders
     * @return reference
     */
    IndexMap & getIndexMap();

    /**
     * This class contains the surfel map
     * @return
     */
    GlobalModel & getGlobalModel();

    /**
     * @return the specific GPUTexture with name @param
     */
    pangolin::GlTexture * getTexture(const std::string &textureType);

    /**
     * @return the specific FeedbackBuffer with name @param
     */
    FeedbackBuffer * getFeedbackBuffer(const std::string &feedbackType);

    /**
     * The current global camera pose estimate
     * @return SE3 pose
     */
    const Eigen::Matrix4f & getCurrPose();

    const std::vector<Eigen::Matrix4f> & getHistoryPoses();


    /**
     * Calculate the above for the current frame (only done on the first frame normally)
     */
    void computeFeedbackBuffers();

    /**
     * Saves out a .ply mesh file of the current model
     */
    void savePly();

    void acquireImages(std::string path, const std::vector<Eigen::Matrix4f> &views,
                       int w, int h, float fx, float fy, float cx, float cy, int startId = 0);

    void reset();

    void texcpy(pangolin::GlTexture * target, pangolin::GlTexture * source);

    void setBeginCleanPoints();

    bool getBeginCleanPoints();

    void cleanPoints(const unsigned short * depth, const unsigned char * semantic, const Eigen::Matrix4f * gtPose);

    Checker * checker;


private:
    int tick;
    Eigen::Matrix4f currPose;
    Eigen::Matrix4f lastPose;
    bool refFrameIsSet;

    IndexMap indexMap;
    GlobalModel globalModel;

    std::map<std::string, GPUTexture*> textures;
    std::map<std::string, ComputePack*> computePacks;
    std::map<std::string, FeedbackBuffer*> feedbackBuffers;

    float nearClipDepth;
    float farClipDepth;

    std::vector<Eigen::Matrix4f> historyPoses;

    bool beginCleanPoints;


    void createTextures();
    void createCompute();
    void createFeedbackBuffers();

    // pre-processing methods
    void filterDepth();
    void metriciseDepth();
    void removeMovings();
};

#endif //SURFELMAPPING_SURFELMAPPING_H
