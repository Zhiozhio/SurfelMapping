

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

    void initialize(const FeedbackBuffer & rawFeedback, const Eigen::Matrix4f &pose);

    static const int TEXTURE_DIMENSION;
    static const int MAX_VERTICES;
    //        static const int NODE_TEXTURE_DIMENSION;
    //        static const int MAX_NODES;

    void renderModel(pangolin::OpenGlMatrix mvp,
                     pangolin::OpenGlMatrix mv,
                     float threshold,
                     bool drawUnstable,
                     bool drawNormals,
                     bool drawColors,
                     bool drawPoints,
                     bool drawWindow,
                     bool drawSemantic,
                     int time,
                     int timeDelta);

    void dataAssociate(const Eigen::Matrix4f & pose,
                       const int & time,
                       GPUTexture * rgb,
                       GPUTexture * depthRaw,
                       GPUTexture * semantic,
                       GPUTexture * indexMap,
                       GPUTexture * vertConfMap,
                       GPUTexture * colorTimeMap,
                       GPUTexture * normRadMap,
                       float depthMin,
                       float depthMax);

    void update();

    void backMapping();

    void concatenate();

    /**
     * Fill in the modelMap* texturefs, which are model aligned in pixels by their index.
     * modelMap* must be synchronized with model.
     */
    void buildModelMap();

    void processConflict(const Eigen::Matrix4f & pose,
                         const int & time,
                         GPUTexture * depthRaw,
                         GPUTexture * semantic);


    pangolin::GlTexture * getModelMapVC();
    pangolin::GlTexture * getModelMapCT();
    pangolin::GlTexture * getModelMapNR();

    Eigen::Vector4f * downloadMap();

    std::pair<GLuint, GLuint> getModel();

    std::pair<GLuint, GLuint> getData();

    std::pair<GLuint, GLuint> getConflict();

    std::pair<GLuint, GLuint> getUnstable();

    unsigned int getOffset();

private:
    GLuint modelVbo, modelFid;;                    // whole surfel buffer & its feedback ID
    // standby bits holds the ID
    GLuint dataVbo, dataFid;                       // including updated surfel and new unstable surfel

    GLuint conflictVbo, conflictFid;

    GLuint unstableVbo;

    const int bufferSize;

    GLuint countQuery;
    unsigned int count;           // current model num
    unsigned int offset;
    unsigned int dataCount;
    unsigned int conflictCount;
    unsigned int unstableCount;

    std::shared_ptr<Shader> initProgram;
    std::shared_ptr<Shader> modelProgram;
    std::shared_ptr<Shader> dataProgram;            // data association
    std::shared_ptr<Shader> conflictProgram;        // check conflict
    std::shared_ptr<Shader> fuseProgram;            // update fused model
    std::shared_ptr<Shader> updateConflictProgram; // update conflict model
    std::shared_ptr<Shader> backMappingProgram;     // re-mapping modelMap back to vbo
    std::shared_ptr<Shader> unstableProgram;        // concatenate new model


    std::shared_ptr<Shader> drawPointProgram;
    std::shared_ptr<Shader> drawSurfelProgram;


    //We render whole vertices into 3 texturefs. Must be synchronize with modelVbo
    GPUTexture modelMapVertsConfs;
    GPUTexture modelMapColorsTime;
    GPUTexture modelMapNormsRadii;

    pangolin::GlFramebuffer frameBuffer;
    pangolin::GlRenderBuffer renderBuffer;

    pangolin::GlFramebuffer vertConfFrameBuffer;
    pangolin::GlRenderBuffer vertConfRenderBuffer;

    GLuint uvo;
    int uvSize;

    GLuint bigUvo;
    int bigUvSize;
};

#endif /* GLOBALMODEL_H_ */
