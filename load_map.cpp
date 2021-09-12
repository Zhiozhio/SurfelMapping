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
#include <random>
#include <ctime>

using namespace std;


int globalId = 0;
int lastRestartId = 0;
std::vector<Eigen::Matrix4f> modelPoses;
std::vector<Eigen::Matrix4f> novelViews;

void rungui(SurfelMapping & core, GUI & gui)
{
    if(gui.getMode() == GUI::ShowMode::minimum)
    {
        Eigen::Matrix4f pose = modelPoses[0];


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
            int totalNovelViewNum = 0;
            while(gui.pathMode->Get())
            {
                float backColor[4] = {0, 0, 0, 0};
                gui.preCall(backColor);

//                //=== overlook tansform
//                pangolin::OpenGlMatrix mv;
//
//                Eigen::Matrix3f currRot;
//                currRot = Eigen::AngleAxis<float>(-M_PI_2, Eigen::Vector3f(1, 0, 0));
//
//                Eigen::Vector3f forwardVector(0, 0, 1);
//                Eigen::Vector3f upVector(0, -1, 0);
//
//                Eigen::Vector3f forward = (currRot * forwardVector).normalized();
//                Eigen::Vector3f up = (currRot * upVector).normalized();
//
//                Eigen::Vector3f viewAt;
//                if(initView)
//                {
//                    viewAt = Eigen::Vector3f(pose(0, 3), -15, pose(2, 3));
//                }
//                else
//                {
//                    pangolin::OpenGlMatrix currMVInv = gui.s_cam.GetModelViewMatrix().Inverse();
//                    viewAt = Eigen::Vector3f(currMVInv(0, 3), currMVInv(1, 3), currMVInv(2, 3));
//                }
//
//                Eigen::Vector3f eye = viewAt;
//
//                Eigen::Vector3f z = -forward;  // Forward, OpenGL camera z direction
//                Eigen::Vector3f x = up.cross(z).normalized();     // Right
//                Eigen::Vector3f y = z.cross(x);                   // Up
//
//                Eigen::Matrix4d m;                                // [R; U; F]_4x4 * [E; -eye]_4x4
//                m << x(0),  x(1),  x(2),  -(x.dot(eye)),
//                        y(0),  y(1),  y(2),  -(y.dot(eye)),
//                        z(0),  z(1),  z(2),  -(z.dot(eye)),
//                        0,     0,     0,      1;
//
//                memcpy(&mv.m[0], m.data(), sizeof(Eigen::Matrix4d));
//
//                gui.s_cam.SetModelViewMatrix(mv);


                //=== draw all history frame
                std::vector<Eigen::Vector3f> posVerts, posVertNovel;
                for(auto & p : modelPoses)
                {
                    gui.drawFrustum(p);
                    posVerts.emplace_back(p.topRightCorner<3, 1>());
                }
                glColor3f(1.0f,1.0f,0.0f);
                pangolin::glDrawVertices(posVerts, GL_LINE_STRIP);
                glColor3f(1.0f,1.0f,1.0f);

                //=== draw all novel frame (if exist)
                float frameColor[3] = {1.f, 0.f, 0.0f};
                for(auto & p : novelViews)
                {
                    gui.drawFrustum(p, frameColor);
                    //posVertNovel.emplace_back(p.topRightCorner<3, 1>());
                }

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

                //=== If acquire images
                if(pangolin::Pushed(*gui.acquirePairedImage))
                {
                    std::string data_path = "/home/zhijun/myProjects/SurfelMapping/output/paired";  // todo

                    std::vector<Eigen::Matrix4f> views;
                    gui.getViews(views, modelPoses);  // todo

                    core.acquireImages(data_path, views,
                                       Config::W(), Config::H(),
                                       Config::fx(), Config::fy(),
                                       Config::cx(), Config::cy(),
                                       lastRestartId);

                    printf("|==== Paired images from frame %d to %d are saved. ====|\n", lastRestartId, globalId);
                    usleep(10000);
                }

                //=== generate novel views
                if(pangolin::Pushed(*gui.generateNovelViews))
                {
                    // get novel view number
                    int novelViewNum = gui.novelViewNum->Get();

                    novelViews.clear();

                    std::vector<Eigen::Matrix4f> views;
                    gui.getViews(views, modelPoses);  // todo

                    // random frame generator
                    std::default_random_engine g;
                    g.seed(time(0));
                    std::uniform_int_distribution<int> uniFrame(0, views.size());
                    // random translation generator
                    std::uniform_real_distribution<float> uniTransX(-2, 2);
                    std::uniform_real_distribution<float> uniTransZ(-1, 1);
                    // random angle generator
                    std::uniform_real_distribution<float> uniAngle(-15, 15);
                    for(int i = 0; i < 100 * novelViewNum; ++i)
                    {
                        // choose a random view
                        int frame_num = uniFrame(g);
                        auto v = views[frame_num];

                        float x_off = uniTransX(g);
                        float z_off = uniTransZ(g);
                        float theta_off = uniAngle(g) * M_PI / 180;

                        auto rotation = Eigen::AngleAxis<float>(theta_off, Eigen::Vector3f(0, -1, 0));
                        auto translation = Eigen::Translation3f(x_off, 0, z_off);

                        Eigen::Transform<float, 3, Eigen::Affine> T;
                        T = translation * rotation;

                        v = v * T.matrix();

                        novelViews.push_back(v);
                    }
                }

                if(pangolin::Pushed(*gui.acquireNovelImage))
                {
                    std::string data_path = "/home/zhijun/myProjects/SurfelMapping/output/novel";  // todo

                    core.acquireImages(data_path, novelViews,
                                       Config::W(), Config::H(),
                                       Config::fx(), Config::fy(),
                                       Config::cx(), Config::cy(),
                                       totalNovelViewNum);

                    printf("|==== Novel images from frame %d to %d are saved. ====|\n", totalNovelViewNum, totalNovelViewNum + novelViews.size() - 1);

                    totalNovelViewNum += novelViews.size();

                    usleep(10000);
                }


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


            gui.drawFrustum(core.getCurrPose());


            gui.postCall();



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

    GUI gui(reader.W(), reader.H(), GUI::ShowMode::minimum);

    SurfelMapping core;

    vector<int> startEndIds;

    core.getGlobalModel().uploadMap(model_path, startEndIds);

    if(!startEndIds.empty())
    {
        lastRestartId = startEndIds[0];
        globalId = startEndIds[1];
        printf("Model from frame %d to %d.\n", lastRestartId, globalId);
    }

    int frame_id = lastRestartId - 1;
    reader.setState(frame_id);
    modelPoses.clear();
    while (reader.getNext())
    {
        if(reader.currentFrameId > globalId)
            break;

        modelPoses.push_back(reader.gtPose);
    }

    // show after loop
    while (true)
    {
        rungui(core, gui);
    }

}