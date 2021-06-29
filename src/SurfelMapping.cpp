//
// Created by zhijun on 2021/5/24.
//

#include "SurfelMapping.h"

SurfelMapping::SurfelMapping()
: tick(0),
  currPose(Eigen::Matrix4f::Identity()),
  nearClipDepth(Config::nearClip()),
  farClipDepth(Config::farClip()),
  checker(new Checker)
{
    createTextures();
    createCompute();
    createFeedbackBuffers();
}

SurfelMapping::~SurfelMapping()
{
    for(auto it = textures.begin(); it != textures.end(); ++it)
    {
        delete it->second;
    }

    textures.clear();

    for(auto it = computePacks.begin(); it != computePacks.end(); ++it)
    {
        delete it->second;
    }

    computePacks.clear();

    for(auto it = feedbackBuffers.begin(); it != feedbackBuffers.end(); ++it)
    {
        delete it->second;
    }

    feedbackBuffers.clear();
}

void SurfelMapping::createTextures()
{
    int w = Config::W();
    int h = Config::H();

    textures[GPUTexture::RGB] = new GPUTexture(w, h,
                                               GL_RGBA,
                                               GL_RGB,
                                               GL_UNSIGNED_BYTE,
                                               true);

    textures[GPUTexture::DEPTH_RAW] = new GPUTexture(w, h,
                                                     GL_R16UI,
                                                     GL_RED_INTEGER,
                                                     GL_UNSIGNED_SHORT);

    textures[GPUTexture::DEPTH_FILTERED] = new GPUTexture(w, h,
                                                          GL_R16UI,
                                                          GL_RED_INTEGER,
                                                          GL_UNSIGNED_SHORT);

    textures[GPUTexture::DEPTH_METRIC] = new GPUTexture(w, h,
                                                        GL_R32F,
                                                        GL_RED,
                                                        GL_FLOAT);

    textures[GPUTexture::DEPTH_METRIC_FILTERED] = new GPUTexture(w, h,
                                                                 GL_R32F,
                                                                 GL_RED,
                                                                 GL_FLOAT);

}

void SurfelMapping::createCompute()
{
    computePacks[ComputePack::FILTER] = new ComputePack(loadProgramFromFile("empty.vert", "quad.geom", "depth_bilateral.frag"),
                                                        textures[GPUTexture::DEPTH_FILTERED]->texture);

    computePacks[ComputePack::METRIC] = new ComputePack(loadProgramFromFile("empty.vert", "quad.geom", "depth_metric.frag"),
                                                        textures[GPUTexture::DEPTH_METRIC]->texture);

    computePacks[ComputePack::METRIC_FILTERED] = new ComputePack(loadProgramFromFile("empty.vert", "quad.geom", "depth_metric.frag"),
                                                                 textures[GPUTexture::DEPTH_METRIC_FILTERED]->texture);
}

void SurfelMapping::createFeedbackBuffers()
{
    feedbackBuffers[FeedbackBuffer::RAW] = new FeedbackBuffer(loadProgramGeomFromFile("surfel_feedback.vert", "surfel_feedback.geom"));
    feedbackBuffers[FeedbackBuffer::FILTERED] = new FeedbackBuffer(loadProgramGeomFromFile("surfel_feedback.vert", "surfel_feedback.geom"));
}

void SurfelMapping::processFrame(const unsigned char *rgb, const unsigned short *depth, const Eigen::Matrix4f *gtPose)
{
    TICK("Run");

    textures[GPUTexture::DEPTH_RAW]->texture->Upload(depth, GL_RED_INTEGER, GL_UNSIGNED_SHORT);
    textures[GPUTexture::RGB]->texture->Upload(rgb, GL_RGB, GL_UNSIGNED_BYTE);

    TICK("Preprocess");  // todo: here can be optimized: depth_filtered maybe deprecated
    filterDepth();
    metriciseDepth();
    TOCK("Preprocess");

    //First run
    if(tick == 0)
    {
        currPose = *gtPose;

        // compute surfel in current frame
        computeFeedbackBuffers();

        globalModel.initialize(*feedbackBuffers[FeedbackBuffer::RAW], *feedbackBuffers[FeedbackBuffer::FILTERED]);

        globalModel.buildModelMap();  // build model map each time modelVbo is updated

//        checker->retrieveVertexf("Model Old", globalModel.getModel().first, globalModel.getModel().second);
//
//        checker->retrieveTexture4f("PosConf", globalModel.getModelMapVC(), );
//        checker->retrieveTexture4f("ColorTime", globalModel.getModelMapCT());
//        checker->retrieveTexture4f("NormRad", globalModel.getModelMapNR());

        //frameToModel.initFirstRGB(texturefs[GPUTexture::RGB]);
    }
    else
    {
        Eigen::Matrix4f lastPose = currPose;

        computeFeedbackBuffers();  // todo: move to outside

        currPose = *gtPose;

        //TICK("autoFill");
        //resize.image(indexMap.imageTex(), imageBuff);
        //bool shouldFillIn = !denseEnough(imageBuff);
        //TOCK("autoFill");

        //Eigen::Vector3f trans = currPose.topRightCorner(3, 1);
        //Eigen::Matrix<float, 3, 3, Eigen::RowMajor> rot = currPose.topLeftCorner(3, 3);

        TICK("indexMap");
        indexMap.predictIndices(currPose, tick, globalModel.getModel(), farClipDepth, 200);
        TOCK("indexMap");

//        checker->retrieveTexture1i("IndexMap", indexMap.indexTex()->texture);

        globalModel.fuse(currPose,
                         tick,
                         textures[GPUTexture::RGB],
                         textures[GPUTexture::DEPTH_METRIC],
                         textures[GPUTexture::DEPTH_METRIC_FILTERED],
                         indexMap.indexTex(),
                         indexMap.vertConfTex(),
                         indexMap.colorTimeTex(),
                         indexMap.normalRadTex(),
                         farClipDepth);

//        checker->retrieveVertexf("Data", globalModel.getData().first, globalModel.getData().second);


//        checker->retrieveTexture4f("PosConfII", globalModel.getModelMapVC(), globalModel.getCount());
//        checker->retrieveTexture4f("ColorTimeII", globalModel.getModelMapCT(), globalModel.getCount());
//        checker->retrieveTexture4f("NormRadII", globalModel.getModelMapNR(), globalModel.getCount());

        globalModel.backMapping();

//        checker->retrieveVertexf("ModelII", globalModel.getModel().first, globalModel.getOffset());

        globalModel.concatenate();

//        checker->retrieveVertexf("Unstable", globalModel.getUnstable().first, globalModel.getUnstable().second);

//        checker->retrieveVertexf("Model New", globalModel.getModel().first, globalModel.getModel().second);

        globalModel.buildModelMap();  // build model map each time modelVbo is updated

//        checker->retrieveVertexf(globalModel.getModel().first, globalModel.getModel().second);
//
//        checker->retrieveTexture4f("PosConf", globalModel.getModelMapVC());
//        checker->retrieveTexture4f("ColorTime", globalModel.getModelMapCT());
//        checker->retrieveTexture4f("NormRad", globalModel.getModelMapNR());

        CheckGlDieOnError();
//
//        TICK("indexMap");
//        indexMap.predictIndices(currPose, tick, globalModel.model(), maxDepthProcessed, timeDelta);
//        TOCK("indexMap");
//
//        // todo: seems not reasonable of this shader
//        globalModel.clean(currPose,
//                          tick,
//                          indexMap.indexTex(),
//                          indexMap.vertConfTex(),
//                          indexMap.colorTimeTex(),
//                          indexMap.normalRadTex(),
//                          indexMap.depthTex(),
//                          confidenceThreshold,
//                          rawGraph,
//                          timeDelta,
//                          maxDepthProcessed,
//                          fernAccepted);

    }

    //TICK("Predict");  No need for odometry temporarily. (predict is for odometry)
    //predict();
    //TOCK("Predict");

    ++tick;

    TOCK("Run");
}

void SurfelMapping::metriciseDepth()
{
    std::vector<Uniform> uniforms;

    uniforms.emplace_back("minD", nearClipDepth);
    uniforms.emplace_back("maxD", farClipDepth);

    computePacks[ComputePack::METRIC]->compute(textures[GPUTexture::DEPTH_RAW]->texture, &uniforms);
    computePacks[ComputePack::METRIC_FILTERED]->compute(textures[GPUTexture::DEPTH_FILTERED]->texture, &uniforms);
}

void SurfelMapping::filterDepth()
{
    std::vector<Uniform> uniforms;

    // set bilateral covariance
    float sigma_pixel = 4.5;
    float sigma_intensity = 30.;
    float sigma_pixel2_inv_half = 0.5f / (sigma_pixel * sigma_pixel);                 // 1 / 2*sigma^2
    float sigma_intensity2_inv_half = 0.5f / (sigma_intensity * sigma_intensity);

    uniforms.emplace_back("cols", (float)Config::W() );
    uniforms.emplace_back("rows", (float)Config::H() );
    uniforms.emplace_back("minD", nearClipDepth);
    uniforms.emplace_back("maxD", farClipDepth );
    uniforms.emplace_back("sigPix", sigma_pixel2_inv_half);
    uniforms.emplace_back("sigIntensity", sigma_intensity2_inv_half);

    computePacks[ComputePack::FILTER]->compute(textures[GPUTexture::DEPTH_RAW]->texture, &uniforms);
}

void SurfelMapping::computeFeedbackBuffers()
{
    TICK("feedbackBuffers");
    feedbackBuffers[FeedbackBuffer::RAW]->compute(textures[GPUTexture::RGB]->texture,
                                                  textures[GPUTexture::DEPTH_METRIC]->texture,
                                                  tick,
                                                  farClipDepth);

    feedbackBuffers[FeedbackBuffer::FILTERED]->compute(textures[GPUTexture::RGB]->texture,
                                                       textures[GPUTexture::DEPTH_METRIC_FILTERED]->texture,
                                                       tick,
                                                       farClipDepth);
    TOCK("feedbackBuffers");
}

pangolin::GlTexture * SurfelMapping::getTexture(const std::string &textureType)
{
    auto iter = textures.find(textureType);
    assert(iter != textures.end() && "there is no such texture type");

    return textures[textureType]->texture;
}

FeedbackBuffer * SurfelMapping::getFeedbackBuffer(const std::string &feedbackType)
{
    auto iter = feedbackBuffers.find(feedbackType);
    assert(iter != feedbackBuffers.end() && "there is no such FeedbackBuffer type");

    return feedbackBuffers[feedbackType];
}

const Eigen::Matrix4f & SurfelMapping::getCurrPose()
{
    return currPose;
}

GlobalModel & SurfelMapping::getGlobalModel()
{
    return globalModel;
}

IndexMap & SurfelMapping::getIndexMap()
{
    return indexMap;
}