//
// Created by zhijun on 2021/5/23.
//

#include "GUI.h"

GUI::GUI(int rawWidth, int rawHeight, ShowMode mode)
: mode_(mode),
  renderedViewNum(0)
{
    // whole GUI window size
    width = 1280;
    height = 980;
    panel = 205;

    setRawSize(rawWidth, rawHeight);


    // create window system (which is necessary) for OpenGL
    //        pangolin::Params windowParams;
    //
    //        windowParams.Set("SAMPLE_BUFFERS", 0);
    //        windowParams.Set("SAMPLES", 0);

    pangolin::CreateWindowAndBind("Main", width + panel, height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);


//    modelRenderBuffer = new pangolin::GlRenderBuffer(internal_render_width, internal_render_height);
//    modelTexture = new pangolin::GlTexture(internal_render_width, internal_render_height, GL_RGBA32F, true, 0, GL_LUMINANCE, GL_FLOAT, NULL);
//
//    modelFrameBuffer = new pangolin::GlFramebuffer;
//    modelFrameBuffer->AttachColour(*modelTexture);
//    modelFrameBuffer->AttachDepth(*modelRenderBuffer);

//    modelProgram = std::shared_ptr<Shader>(loadProgramFromFile("draw_surface.vert", "draw_surface_phong.frag", "draw_surface.geom"));
//    fxaaProgram = std::shared_ptr<Shader>(loadProgramFromFile("empty.vert", "fxaa.frag", "quad.geom"));


    s_cam = pangolin::OpenGlRenderState(pangolin::ProjectionMatrix(640, 480, 420, 420, 320, 240, 0.1, 1000),
                                        pangolin::ModelViewLookAt(0, 0, -1, 0, 0, 1, pangolin::AxisNegY));

    // "cam" is the main view
    pangolin::Display("cam").SetBounds(0, 1.0f, 0, 1.0f, -640 / 480.0)
            .SetHandler(new pangolin::Handler3D(s_cam));




    if(mode == ShowMode::supervision)
    {
        pangolin::Display("rgb").SetAspect(rawWidth / rawHeight).SetLock(pangolin::LockLeft, pangolin::LockBottom);

        pangolin::Display("depth").SetAspect(rawWidth / rawHeight).SetLock(pangolin::LockRight, pangolin::LockBottom);

        int multi_height = std::min(rawHeight * 2 * rawWidth / width, height / 4);
        pangolin::CreatePanel("ui").SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(panel));
        pangolin::Display("multi").SetBounds(pangolin::Attach::Pix(0), pangolin::Attach::Pix(multi_height), pangolin::Attach::Pix(panel), 1.0)
                .SetLayout(pangolin::LayoutEqualHorizontal)
                .AddDisplay(pangolin::Display("rgb"))
                .AddDisplay(pangolin::Display("depth"));

        pangolin::Display("semantic").SetAspect(rawWidth / rawHeight)
                .SetLock(pangolin::LockLeft, pangolin::LockTop)
                .SetBounds(pangolin::Attach::Pix(height - multi_height / 3), 1.0, pangolin::Attach::Pix(panel), 1.0);


        // show raw depth image
        depthNormFrameBuffer = new pangolin::GlFramebuffer;
        depthNormTexture = new pangolin::GlTexture(rawWidth, rawHeight, GL_RGBA, true, 0, GL_RGBA, GL_FLOAT);
        depthNormRenderBuffer = new pangolin::GlRenderBuffer(rawWidth, rawHeight);
        depthNormFrameBuffer->AttachColour(*depthNormTexture);
        depthNormFrameBuffer->AttachDepth(*depthNormRenderBuffer);

        semanticFrameBuffer = new pangolin::GlFramebuffer;
        semanticTexture = new pangolin::GlTexture(rawWidth, rawHeight, GL_RGBA, true, 0, GL_RGBA, GL_FLOAT);
        semanticRenderBuffer = new pangolin::GlRenderBuffer(rawWidth, rawHeight);
        semanticFrameBuffer->AttachColour(*semanticTexture);
        semanticFrameBuffer->AttachDepth(*semanticRenderBuffer);

        normalizeProgram = loadProgramFromFile("empty.vert", "quad.geom", "depth_norm_float.frag");
        debugProgram = loadProgramFromFile("empty.vert", "debug_quad.geom", "debug.frag");
        showSemanticProgram = loadProgramFromFile("empty.vert", "quad.geom", "show_semantic.frag");

        // set panel
        pause = new pangolin::Var<bool>("ui.Pause", true, true);
        step = new pangolin::Var<bool>("ui.Step", false, false);

        //    depthCutoff = new pangolin::Var<float>("ui.Depth cutoff", 3.0, 0.0, 12.0);
        //
        followPose = new pangolin::Var<bool>("ui.Follow pose", true, true);
        drawRawCloud = new pangolin::Var<int>("ui.Draw raw", 0, 0, 5);
        //drawFilteredCloud = new pangolin::Var<int>("ui.Draw filtered", 0, 0, 4);
        drawGlobalModel = new pangolin::Var<int>("ui.Draw global model", 2, 0, 5);
        clean = new pangolin::Var<bool>("ui.Clean Pointcloud", false, false);
        save = new pangolin::Var<bool>("ui.Save", false, false);
        reset = new pangolin::Var<bool>("ui.Reset", false, false);

        pathMode = new pangolin::Var<bool>("ui.Path Mode", false, true);
        acquirePairedImage = new pangolin::Var<bool>("ui.Acquire Paired Images", false, false);

        //    drawColors = new pangolin::Var<bool>("ui.Draw colors", showcaseMode, true);
        //    drawFxaa = new pangolin::Var<bool>("ui.Draw FXAA", showcaseMode, true);
        //    drawNormals = new pangolin::Var<bool>("ui.Draw normals", false, true);
        //
        //    gpuMem = new pangolin::Var<int>("ui.GPU memory free", 0);
        //    totalPoints = new pangolin::Var<std::string>("ui.Total points", "0");
        //    trackInliers = new pangolin::Var<std::string>("ui.Inliers", "0");
    }


    if(mode == ShowMode::minimum)
    {
        pangolin::CreatePanel("ui").SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(panel));

        // set panel
        followPose = new pangolin::Var<bool>("ui.Follow pose", true, true);

        drawGlobalModel = new pangolin::Var<int>("ui.Draw global model", 2, 0, 5);
        clean = new pangolin::Var<bool>("ui.Clean Pointcloud", false, false);

        pathMode = new pangolin::Var<bool>("ui.Path Mode", false, true);
        acquirePairedImage = new pangolin::Var<bool>("ui.Acquire Paired Images", false, false);
        novelViewNum = new pangolin::Var<int>("ui.Novel Views Num", 0, 0, 5);
        generateNovelViews = new pangolin::Var<bool>("ui.Generate Novel Views", false, false);
        generate_S_views = new pangolin::Var<bool>("ui.generate \"S\"-shaped path", false, false);
        acquireNovelImage = new pangolin::Var<bool>("ui.Acquire Novel Images", false, false);
        overview = new pangolin::Var<bool>("ui.Overview", false, false);
    }


    //        if(showcaseMode)
    //        {
    //            pangolin::RegisterKeyPressCallback(' ', pangolin::SetVarFunctor<bool>("ui.Reset", true));
    //        }
}

GUI::~GUI()
{
    delete pause;
    delete reset;
    delete step;
    delete save;
    delete trackInliers;
    delete depthCutoff;
    delete followPose;
    delete drawRawCloud;
    delete totalPoints;
    delete drawFilteredCloud;
    delete drawGlobalModel;
    delete gpuMem;
    delete pathMode;
    delete acquirePairedImage;
    delete clean;

    delete modelRenderBuffer;
    delete modelFrameBuffer;
    delete modelTexture;

    delete depthNormRenderBuffer;
    delete depthNormFrameBuffer;
    delete depthNormTexture;
}

void GUI::preCall(float *backgroundColor)
{
    glClearColor(backgroundColor[0], backgroundColor[1], backgroundColor[2], backgroundColor[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    width = pangolin::DisplayBase().v.w;
    height = pangolin::DisplayBase().v.h;

    pangolin::Display("cam").Activate(s_cam);
}

void GUI::postCall()
{
    pangolin::FinishFrame();

    glFinish();
}

void GUI::displayImg(const std::string &id, pangolin::GlTexture *img)
{
    glDisable(GL_DEPTH_TEST);

    pangolin::Display(id).Activate();
    img->RenderToViewport(true);

    glEnable(GL_DEPTH_TEST);
}

void GUI::normalizeDepth(pangolin::GlTexture * img, const float &minVal, const float &maxVal)
{
    img->Bind();

    depthNormFrameBuffer->Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, depthNormRenderBuffer->width, depthNormRenderBuffer->height);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    normalizeProgram->Bind();

    normalizeProgram->setUniform(Uniform("minVal", minVal));
    normalizeProgram->setUniform(Uniform("maxVal", maxVal));

    glDrawArrays(GL_POINTS, 0, 1);

    depthNormFrameBuffer->Unbind();

    normalizeProgram->Unbind();

    glPopAttrib();

    glFinish();
}

void GUI::processSemantic(pangolin::GlTexture *img)
{
    img->Bind();

    semanticFrameBuffer->Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, semanticRenderBuffer->width, semanticRenderBuffer->height);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    showSemanticProgram->Bind();

    showSemanticProgram->setUniform(Uniform("eSampler", 0));
    showSemanticProgram->setUniform(Uniform("class0", Eigen::Vector3f(128,128,128)));
    showSemanticProgram->setUniform(Uniform("class1", Eigen::Vector3f(0,255,0)));
    showSemanticProgram->setUniform(Uniform("class2", Eigen::Vector3f(0,0,255)));
    showSemanticProgram->setUniform(Uniform("class3", Eigen::Vector3f(255,255,0)));
    showSemanticProgram->setUniform(Uniform("class4", Eigen::Vector3f(128,0,0)));
    showSemanticProgram->setUniform(Uniform("class5", Eigen::Vector3f(255,0,255)));
    showSemanticProgram->setUniform(Uniform("class6", Eigen::Vector3f(128,128,0)));
    showSemanticProgram->setUniform(Uniform("class7", Eigen::Vector3f(0,128,0)));
    showSemanticProgram->setUniform(Uniform("class8", Eigen::Vector3f(128,0,128)));
    showSemanticProgram->setUniform(Uniform("class9", Eigen::Vector3f(0,128,128)));
    showSemanticProgram->setUniform(Uniform("class10", Eigen::Vector3f(0,255,255)));
    showSemanticProgram->setUniform(Uniform("class11", Eigen::Vector3f(0,0,128)));
    showSemanticProgram->setUniform(Uniform("class12", Eigen::Vector3f(245,222,179)));
    showSemanticProgram->setUniform(Uniform("class13", Eigen::Vector3f(255,0,0)));
    showSemanticProgram->setUniform(Uniform("class14", Eigen::Vector3f(210,105,30)));
    showSemanticProgram->setUniform(Uniform("class15", Eigen::Vector3f(244,164,96)));
    showSemanticProgram->setUniform(Uniform("class16", Eigen::Vector3f(119,136,153)));
    showSemanticProgram->setUniform(Uniform("class17", Eigen::Vector3f(255,20,147)));
    showSemanticProgram->setUniform(Uniform("class18", Eigen::Vector3f(138,43,226)));

    glDrawArrays(GL_POINTS, 0, 1);

    showSemanticProgram->Unbind();

    glPopAttrib();

    semanticFrameBuffer->Unbind();

    glFinish();
}

void GUI::drawCapacity(pangolin::GlTexture *img)
{
    float h = this->height / 2;
    float w = h * 0.02;

    float h_frag = h / this->height;
    float w_frag = w / this->width;

    glDisable(GL_DEPTH_TEST);

    debugProgram->Bind();

    debugProgram->setUniform(Uniform("heightFrag", h_frag));
    debugProgram->setUniform(Uniform("widthFrag", w_frag));
    debugProgram->setUniform(Uniform("fboAttachment", 0));

    img->Bind();

    glDrawArrays(GL_POINTS, 0, 1);

    img->Unbind();

    debugProgram->Unbind();

    glEnable(GL_DEPTH_TEST);
}

void GUI::drawDebug(pangolin::GlTexture *img)
{
    float debug_ratio = (float)img->height / img->width;
    float debug_h = std::min(img->height, this->height / 2);
    float debug_w = std::min(img->width, this->width / 2);

    if( (debug_h / debug_w) < debug_ratio)
        debug_w = debug_h / debug_ratio;
    else
        debug_h = debug_w * debug_ratio;

    float h_frag = debug_h / this->height;
    float w_frag = debug_w / this->width;

    glDisable(GL_DEPTH_TEST);

    debugProgram->Bind();

    debugProgram->setUniform(Uniform("heightFrag", h_frag));
    debugProgram->setUniform(Uniform("widthFrag", w_frag));
    debugProgram->setUniform(Uniform("fboAttachment", 0));

    img->Bind();

    glDrawArrays(GL_POINTS, 0, 1);

    img->Unbind();

    debugProgram->Unbind();

    glEnable(GL_DEPTH_TEST);
}

void GUI::drawFrustum(const Eigen::Matrix4f &pose, float * color)
{
    Eigen::Matrix3f K = Eigen::Matrix3f::Identity();
    K(0, 0) = Config::fx();
    K(1, 1) = Config::fy();
    K(0, 2) = Config::cx();
    K(1, 2) = Config::cy();

    Eigen::Matrix3f Kinv = K.inverse();

    if(color == nullptr)
        glColor3f(1.0f,1.0f,0.0f);
    else
        glColor3f(color[0], color[1], color[2]);

    pangolin::glDrawFrustum(Kinv,
                            Config::W(),
                            Config::H(),
                            pose,
                            0.1f);

    glColor3f(1.0f, 1.0f, 1.0f);
}

void GUI::drawFXAA(pangolin::OpenGlMatrix mvp,
                   pangolin::OpenGlMatrix mv,
                   const std::pair<GLuint, GLuint> &model,
                   const float threshold, const int time, const int timeDelta, const bool invertNormals)
{

}

int GUI::getViews(std::vector<Eigen::Matrix4f> &newViews, const std::vector<Eigen::Matrix4f> &originViews)
{
    newViews = originViews;
    int lastRenderedViewNum = renderedViewNum;
    renderedViewNum += newViews.size();
    return lastRenderedViewNum;
}