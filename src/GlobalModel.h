

#ifndef GLOBALMODEL_H_
#define GLOBALMODEL_H_

#include "Shaders.h"
#include "Uniform.h"
#include "FeedbackBuffer.h"
#include "GPUTexture.h"
#include "IndexMap.h"
#include "Utils/Stopwatch.h"
#include <pangolin/gl/gl.h>
#include <Eigen/LU>

class GlobalModel
{
public:
    GlobalModel();
    virtual ~GlobalModel();

    void initialize(const FeedbackBuffer & rawFeedback);

    static const int TEXTURE_DIMENSION;
    static const int MAX_VERTICES;
    //        static const int NODE_TEXTURE_DIMENSION;
    //        static const int MAX_NODES;

    void renderModel(pangolin::OpenGlMatrix mvp,
                     const float threshold,
                     const bool drawUnstable,
                     const bool drawNormals,
                     const bool drawColors,
                     const bool drawPoints,
                     const bool drawWindow,
                     const bool drawTimes,
                     const int time,
                     const int timeDelta);

    void fuse(const Eigen::Matrix4f & pose,
              const int & time,
              GPUTexture * rgb,
              GPUTexture * depthRaw,
              GPUTexture * indexMap,
              GPUTexture * vertConfMap,
              GPUTexture * colorTimeMap,
              GPUTexture * normRadMap,
              const float depthCutoff);

    void backMapping();

    void concatenate();

    void clean(const Eigen::Matrix4f & pose,
               const int & time,
               GPUTexture * indexMap,
               GPUTexture * vertConfMap,
               GPUTexture * colorTimeMap,
               GPUTexture * normRadMap,
               GPUTexture * depthMap,
               const float confThreshold,
               std::vector<float> & graph,
               const int timeDelta,
               const float maxDepth,
               const bool isFern);

    /**
     * Fill in the modelMap* texturefs, which are model aligned in pixels by their index.
     * modelMap* must be synchronized with model.
     */
    void buildModelMap();

    unsigned int getCount();

    unsigned int getOffset();

    unsigned int getDataCount();

    unsigned int getUnstableCount();

    pangolin::GlTexture * getModelMapVC();
    pangolin::GlTexture * getModelMapCT();
    pangolin::GlTexture * getModelMapNR();

    Eigen::Vector4f * downloadMap();

    std::pair<GLuint, GLuint> getModel();

    std::pair<GLuint, GLuint> getData();

    std::pair<GLuint, GLuint> getUnstable();

private:
    GLuint modelVbo, modelFid;;                    // whole surfel buffer & its feedback ID
    // standby bits holds the ID
    GLuint dataVbo, dataFid;                       // including updated surfel and new unstable surfel

    GLuint unstableVbo;

    const int bufferSize;

    GLuint countQuery;
    unsigned int count;           // current model num
    unsigned int offset;
    unsigned int dataCount;
    unsigned int unstableCount;

    std::shared_ptr<Shader> initProgram;
    std::shared_ptr<Shader> modelProgram;
    std::shared_ptr<Shader> drawPointProgram;
    std::shared_ptr<Shader> drawSurfelProgram;

    //For supersample fusing
    std::shared_ptr<Shader> dataProgram;            /// data association
    std::shared_ptr<Shader> updateProgram;          /// update model
    std::shared_ptr<Shader> backMappingProgram;     /// mapping modelMap back to vbo
    std::shared_ptr<Shader> unstableProgram;        /// concatenate new model

    //We render whole vertices into 3 texturefs. Must be synchronize with modelVbo
    GPUTexture modelMapVertsConfs;
    GPUTexture modelMapColorsTime;
    GPUTexture modelMapNormsRadii;

    pangolin::GlFramebuffer frameBuffer;
    pangolin::GlRenderBuffer renderBuffer;

    GLuint uvo;
    int uvSize;

    GLuint bigUvo;
    int bigUvSize;
};

#endif /* GLOBALMODEL_H_ */
