//
// Created by zhijun on 2021/8/23.
//

#include "KittiReader.h"
#include "SurfelMapping.h"
#include "GUI.h"
#include "Checker.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <vector>
#include <set>
#include <ctime>

using namespace std;


int globalId = 0;
int lastRestartId = 0;

void rungui(SurfelMapping & core, GUI & gui)
{
    if(gui.getMode() == GUI::ShowMode::supervision)
    {
        pangolin::GlTexture *rgb = core.getTexture(GPUTexture::RGB);
        pangolin::GlTexture *depth = core.getTexture(GPUTexture::DEPTH_METRIC);
        pangolin::GlTexture *semantic = core.getTexture(GPUTexture::SEMANTIC);

        Eigen::Matrix4f pose = core.getCurrPose();


        bool run_gui = true;
        while(run_gui)
        {
            if(gui.followPose->Get())
            {
                pangolin::OpenGlMatrix mv;

                Eigen::Matrix3f currRot = pose.topLeftCorner(3, 3);

                Eigen::Vector3f forwardVector(0, 0, 1);
                Eigen::Vector3f upVector(0, -1, 0);

                Eigen::Vector3f forward = (currRot * forwardVector).normalized();
                Eigen::Vector3f up = (currRot * upVector).normalized();

                Eigen::Vector3f viewAt(pose(0, 3), pose(1, 3), pose(2, 3));

                Eigen::Vector3f eye = viewAt - forward;

                Eigen::Vector3f z = (eye - viewAt).normalized();  // Forward, OpenGL camera z direction
                Eigen::Vector3f x = up.cross(z).normalized();     // Right
                Eigen::Vector3f y = z.cross(x);                   // Up

                Eigen::Matrix4d m;                                // [R; U; F]_4x4 * [E; -eye]_4x4
                m << x(0),  x(1),  x(2),  -(x.dot(eye)),
                        y(0),  y(1),  y(2),  -(y.dot(eye)),
                        z(0),  z(1),  z(2),  -(z.dot(eye)),
                        0,     0,     0,      1;

                memcpy(&mv.m[0], m.data(), sizeof(Eigen::Matrix4d));

                gui.s_cam.SetModelViewMatrix(mv);
            }


            //====== Enter Path Mode until Complete
            bool initView = true;
            while(gui.pathMode->Get())
            {
                float backColor[4] = {0, 0, 0, 0};
                gui.preCall(backColor);

                //=== overlook tansform
                pangolin::OpenGlMatrix mv;

                Eigen::Matrix3f currRot;
                currRot = Eigen::AngleAxis<float>(-M_PI_2, Eigen::Vector3f(1, 0, 0));

                Eigen::Vector3f forwardVector(0, 0, 1);
                Eigen::Vector3f upVector(0, -1, 0);

                Eigen::Vector3f forward = (currRot * forwardVector).normalized();
                Eigen::Vector3f up = (currRot * upVector).normalized();

                Eigen::Vector3f viewAt;
                if(initView)
                {
                    viewAt = Eigen::Vector3f(pose(0, 3), -15, pose(2, 3));
                }
                else
                {
                    pangolin::OpenGlMatrix currMVInv = gui.s_cam.GetModelViewMatrix().Inverse();
                    viewAt = Eigen::Vector3f(currMVInv(0, 3), currMVInv(1, 3), currMVInv(2, 3));
                }

                Eigen::Vector3f eye = viewAt;

                Eigen::Vector3f z = -forward;  // Forward, OpenGL camera z direction
                Eigen::Vector3f x = up.cross(z).normalized();     // Right
                Eigen::Vector3f y = z.cross(x);                   // Up

                Eigen::Matrix4d m;                                // [R; U; F]_4x4 * [E; -eye]_4x4
                m << x(0),  x(1),  x(2),  -(x.dot(eye)),
                        y(0),  y(1),  y(2),  -(y.dot(eye)),
                        z(0),  z(1),  z(2),  -(z.dot(eye)),
                        0,     0,     0,      1;

                memcpy(&mv.m[0], m.data(), sizeof(Eigen::Matrix4d));

                gui.s_cam.SetModelViewMatrix(mv);


                //=== draw all history frame
                std::vector<Eigen::Vector3f, Eigen::aligned_allocator<Eigen::Vector3f>> posVerts;
                for(auto & p : core.getHistoryPoses())
                {
                    gui.drawFrustum(p);
                    posVerts.emplace_back(p.topRightCorner<3, 1>());
                }
                glColor3f(1.0f,1.0f,0.0f);
                pangolin::glDrawVertices(posVerts, GL_LINE_STRIP);
                glColor3f(1.0f,1.0f,1.0f);

                //=== draw model
                core.getGlobalModel().renderModel(gui.s_cam.GetProjectionModelViewMatrix(),
                                                  gui.s_cam.GetModelViewMatrix(),
                                                  0.0,
                                                  true,
                                                  false,
                                                  true,
                                                  false,
                                                  false,
                                                  false,
                                                  3,
                                                  3);


                gui.postCall();

                initView = false;
            }

            float backColor[4] = {0.05, 0.05, 0.3, 0.0f};
            gui.preCall(backColor);


            //============ draw global model
            int surfel_mode = gui.drawGlobalModel->Get();

            if(surfel_mode)
                core.getGlobalModel().renderModel(gui.s_cam.GetProjectionModelViewMatrix(),
                                                  gui.s_cam.GetModelViewMatrix(),
                                                  0.0,
                                                  true,
                                                  surfel_mode == 3,
                                                  surfel_mode == 4,
                                                  surfel_mode == 1,
                                                  false,
                                                  surfel_mode == 5,
                                                  3,
                                                  3);



            gui.drawCapacity(core.getGlobalModel().getModelMapNR());

            gui.drawFrustum(core.getCurrPose());



            //============ draw raw image data (must be after "cam")
            gui.displayImg("rgb", rgb);

            gui.normalizeDepth(depth, Config::nearClip(), Config::farClip());
            gui.displayImg("depth", gui.depthNormTexture);

            gui.processSemantic(semantic);
            gui.displayImg("semantic", gui.semanticTexture);


            gui.postCall();


            run_gui = gui.pause->Get() && !pangolin::Pushed(*gui.step);
        }


    }
}



int main(int argc, char ** argv)
{
    std::string kittiDir(argv[1]);

    KittiReader reader(kittiDir, false, true, 0, true);

    std::string model_path(argv[2]);

    // Initialize the Config in first call with correct arguments
    Config::getInstance(reader.fx(), reader.fy(), reader.cx(), reader.cy(), reader.H(), reader.W());

    GUI gui(reader.W(), reader.H(), GUI::ShowMode::supervision);

    SurfelMapping core;

    vector<int> startEndIds;

    core.getGlobalModel().uploadMap(model_path, startEndIds);

    if(!startEndIds.empty())
    {
        lastRestartId = startEndIds[0];
        globalId = startEndIds[1];
        printf("Model from frame %d to %d.\n", lastRestartId, globalId);
    }

    CheckGlDieOnError();

    // show after loop
    while (true)
    {
        rungui(core, gui);
    }

}