//
// Created by zhijun on 2021/5/23.
//

#include "GUI.h"

GUI::GUI(int rawWidth, int rawHeight, ShowMode mode)
: mode_(mode)
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


    // internally render model at 3840x2160
    int internal_render_width = 3840;
    int internal_render_height = 2160;

//    modelRenderBuffer = new pangolin::GlRenderBuffer(internal_render_width, internal_render_height);
//    modelTexture = new pangolin::GlTexture(internal_render_width, internal_render_height, GL_RGBA32F, true, 0, GL_LUMINANCE, GL_FLOAT, NULL);
//
//    modelFrameBuffer = new pangolin::GlFramebuffer;
//    modelFrameBuffer->AttachColour(*modelTexture);
//    modelFrameBuffer->AttachDepth(*modelRenderBuffer);

//    modelProgram = std::shared_ptr<Shader>(loadProgramFromFile("draw_global_surface.vert", "draw_global_surface_phong.frag", "draw_global_surface.geom"));
//    fxaaProgram = std::shared_ptr<Shader>(loadProgramFromFile("empty.vert", "fxaa.frag", "quad.geom"));


    s_cam = pangolin::OpenGlRenderState(pangolin::ProjectionMatrix(640, 480, 420, 420, 320, 240, 0.1, 1000),
                                        pangolin::ModelViewLookAt(0, 0, -1, 0, 0, 1, pangolin::AxisNegY));

    // "cam" is the main view
    pangolin::Display("cam").SetBounds(0, 1.0f, 0, 1.0f, -640 / 480.0)
            .SetHandler(new pangolin::Handler3D(s_cam));

    pangolin::Display("rgb").SetAspect(rawWidth / rawHeight).SetLock(pangolin::LockLeft, pangolin::LockBottom);

    pangolin::Display("depth").SetAspect(rawWidth / rawHeight).SetLock(pangolin::LockRight, pangolin::LockBottom);


    if(mode == ShowMode::supervision)
    {
        int multi_height = std::min(rawHeight * 2 * rawWidth / width, height / 4);
        pangolin::CreatePanel("ui").SetBounds(0.0, 1.0, 0.0, pangolin::Attach::Pix(panel));
        pangolin::Display("multi").SetBounds(pangolin::Attach::Pix(0), pangolin::Attach::Pix(multi_height), pangolin::Attach::Pix(panel), 1.0)
                .SetLayout(pangolin::LayoutEqualHorizontal)
                .AddDisplay(pangolin::Display("rgb"))
                .AddDisplay(pangolin::Display("depth"));


        // show raw depth image
        depthNormFrameBuffer = new pangolin::GlFramebuffer;
        depthNormTexture = new pangolin::GlTexture(width, height, GL_RGBA, true, 0, GL_RGBA, GL_FLOAT);
        depthNormRenderBuffer = new pangolin::GlRenderBuffer(width, height);
        depthNormFrameBuffer->AttachColour(*depthNormTexture);
        depthNormFrameBuffer->AttachDepth(*depthNormRenderBuffer);

        normalizeProgram = loadProgramFromFile("empty.vert", "quad.geom", "depth_norm_float.frag");
        debugProgram = loadProgramFromFile("empty.vert", "debug_quad.geom", "debug.frag");

        // set panel
        pause = new pangolin::Var<bool>("ui.Pause", true, true);
        step = new pangolin::Var<bool>("ui.Step", false, false);
        //    save = new pangolin::Var<bool>("ui.Save", false, false);
        //    reset = new pangolin::Var<bool>("ui.Reset", false, false);

        //    depthCutoff = new pangolin::Var<float>("ui.Depth cutoff", 3.0, 0.0, 12.0);
        //
        followPose = new pangolin::Var<bool>("ui.Follow pose", true, true);
        drawRawCloud = new pangolin::Var<int>("ui.Draw raw", 0, 0, 4);
        //drawFilteredCloud = new pangolin::Var<int>("ui.Draw filtered", 0, 0, 4);
        drawGlobalModel = new pangolin::Var<int>("ui.Draw global model", 2, 0, 5);
        //    drawColors = new pangolin::Var<bool>("ui.Draw colors", showcaseMode, true);
        //    drawFxaa = new pangolin::Var<bool>("ui.Draw FXAA", showcaseMode, true);
        //    drawNormals = new pangolin::Var<bool>("ui.Draw normals", false, true);
        //
        //    gpuMem = new pangolin::Var<int>("ui.GPU memory free", 0);
        //    totalPoints = new pangolin::Var<std::string>("ui.Total points", "0");
        //    trackInliers = new pangolin::Var<std::string>("ui.Inliers", "0");
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
    delete drawFxaa;
    delete followPose;
    delete drawRawCloud;
    delete totalPoints;
    delete drawFilteredCloud;
    delete drawGlobalModel;
    delete gpuMem;

    delete modelRenderBuffer;
    delete modelFrameBuffer;
    delete modelTexture;

    delete depthNormRenderBuffer;
    delete depthNormFrameBuffer;
    delete depthNormTexture;
}

void GUI::preCall()
{
    glClearColor(0.05, 0.05, 0.3, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    width = pangolin::DisplayBase().v.w;
    height = pangolin::DisplayBase().v.h;

    pangolin::Display("cam").Activate(s_cam);
}

void GUI::postCall()
{
    //    GLint cur_avail_mem_kb = 0;
    //    glGetIntegerv(GL_GPU_MEM_INFO_CURRENT_AVAILABLE_MEM_NVX, &cur_avail_mem_kb);
    //
    //    int memFree = cur_avail_mem_kb / 1024;
    //
    //    gpuMem->operator=(memFree);

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

void GUI::drawFrustum(const Eigen::Matrix4f &pose)
{
    Eigen::Matrix3f K = Eigen::Matrix3f::Identity();
    K(0, 0) = Config::fx();
    K(1, 1) = Config::fy();
    K(0, 2) = Config::cx();
    K(1, 2) = Config::cy();

    Eigen::Matrix3f Kinv = K.inverse();

    glColor3f(1.0f,1.0f,0.0f);

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