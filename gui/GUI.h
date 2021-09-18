//
// Created by zhijun on 2021/5/21.
//

#ifndef SURFELMAPPING_GUI_H
#define SURFELMAPPING_GUI_H

#include <map>
#include <algorithm>

#include <pangolin/pangolin.h>
#include <pangolin/gl/gl.h>
#include <pangolin/gl/gldraw.h>
// borrow some function form core
#include <Shaders.h>
#include <Config.h>

#define GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX 0x9049

class GUI
{
public:

    enum ShowMode {minimum=0, supervision};

    GUI(int rawWidth, int rawHeight, ShowMode mode=ShowMode::supervision);

    virtual ~GUI();


    void preCall(float *backgroundColor);

    /**
     * You can draw any thing here at the top right of the main screen
     */
    void drawDebug(pangolin::GlTexture * img);

    void drawCapacity(pangolin::GlTexture * img);


    void drawFrustum(const Eigen::Matrix4f & pose, float * color = nullptr);


    void displayImg(const std::string & id, pangolin::GlTexture * img);


    void postCall();


    void normalizeDepth(pangolin::GlTexture * img, const float & minVal, const float & maxVal);


    void processSemantic(pangolin::GlTexture * img);


    void drawFXAA(pangolin::OpenGlMatrix mvp,
                  pangolin::OpenGlMatrix mv,
                  const std::pair<GLuint, GLuint> & model,
                  const float threshold,
                  const int time,
                  const int timeDelta,
                  const bool invertNormals);


    /**
     * Set the raw depth and rgb display size
     */
    void setRawSize(int w, int h)
    {
        rawWidth = w;
        rawHeight = h;
    }

    ShowMode getMode() { return mode_; }

    int getViews(std::vector<Eigen::Matrix4f> & newViews, const std::vector<Eigen::Matrix4f> & originViews);


private:
    // window layout
    ShowMode mode_;
    int width;
    int height;
    int panel;
    int rawWidth;
    int rawHeight;


public:
    pangolin::Var<bool> * pause,
                        * step,
                        * save,
                        * reset,
                        * followPose,
                        * pathMode,
                        * acquirePairedImage,
                        * generateNovelViews,
                        * generate_S_views,
                        * acquireNovelImage,
                        * clean,
                        * overview;

    pangolin::Var<int> * gpuMem,
                       // define draw mode: 0 - none, 1 - mono, 2 - normal, 3 - color, 4 - semantic, 5 - surfel
                       * drawRawCloud,
                       * drawFilteredCloud,
                       // define draw mode: 0 - none, 1 - point, 2 - mono, 3 - normal, 4 - color, 5 - semantic
                       * drawGlobalModel,
                       * novelViewNum;

    pangolin::Var<std::string> * totalPoints,
                               * trackInliers;

    pangolin::Var<float> * depthCutoff;

    pangolin::OpenGlRenderState s_cam;

    pangolin::GlFramebuffer * modelFrameBuffer;
    pangolin::GlTexture * modelTexture;
    pangolin::GlRenderBuffer * modelRenderBuffer;
    std::shared_ptr<Shader> debugProgram;

    std::shared_ptr<Shader> modelProgram;
    std::shared_ptr<Shader> fxaaProgram;

    pangolin::GlFramebuffer * depthNormFrameBuffer;
    pangolin::GlTexture * depthNormTexture;
    pangolin::GlRenderBuffer * depthNormRenderBuffer;
    std::shared_ptr<Shader> normalizeProgram;

    pangolin::GlFramebuffer * semanticFrameBuffer;
    pangolin::GlTexture * semanticTexture;
    pangolin::GlRenderBuffer * semanticRenderBuffer;
    std::shared_ptr<Shader> showSemanticProgram;


    int renderedViewNum;
};


#endif //SURFELMAPPING_GUI_H
