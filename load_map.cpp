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

bool S_shaped_novel = false;

// overview
int overviewId = 0;

void rungui(SurfelMapping & core, GUI & gui)
{
    if(gui.getMode() == GUI::ShowMode::minimum)
    {

        bool run_gui = true;
        while(run_gui)
        {
            Eigen::Matrix4f pose = modelPoses[overviewId];

            bool follow_pose = gui.followPose->Get();

            if(follow_pose)
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


            //float backColor[4] = {0.05, 0.05, 0.3, 0.0f};
            float backColor[4] = {0, 0, 0, 0};
            gui.preCall(backColor);

            //====== Enter Path Mode until Complete
            bool initView = true;
            int totalNovelViewNum = 0;
            if(gui.pathMode->Get())
            {

                //=== draw all history frame
                std::vector<Eigen::Vector3f> positionVerts, positionVertNovel;
                for(auto & p : modelPoses)
                {
                    gui.drawFrustum(p);
                    positionVerts.emplace_back(p.topRightCorner<3, 1>());
                }
                glColor3f(1.0f,1.0f,0.0f);
                pangolin::glDrawVertices(positionVerts, GL_LINE_STRIP);
                glColor3f(1.0f,1.0f,1.0f);

                //=== draw all novel frame (if exist)
                float frameColor[3] = {1.f, 0.f, 0.0f};
                for(auto & p : novelViews)
                {
                    gui.drawFrustum(p, frameColor);
                    positionVertNovel.emplace_back(p.topRightCorner<3, 1>());
                }
                if(S_shaped_novel)
                {

                    glColor3f(1.0f,0.0f,0.0f);
                    pangolin::glDrawVertices(positionVertNovel, GL_LINE_STRIP);
                    glColor3f(1.0f,1.0f,1.0f);
                }


                //=== If acquire images
                if(pangolin::Pushed(*gui.acquirePairedImage))
                {
                    std::string data_path = "../output/paired";  // todo

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

                    S_shaped_novel = false;
                }

                //=== generate "S"-shaped novel views
                if(pangolin::Pushed(*gui.generate_S_views))
                {
                    novelViews.clear();

                    // get novel view sinusoidal period
                    int novelViewNum = gui.novelViewNum->Get() * 3;

                    std::vector<Eigen::Matrix4f> views;
                    gui.getViews(views, modelPoses);  // todo
                    novelViews.reserve(views.size());

                    auto curr_v = views[0];
                    auto last_v = curr_v;
                    double total_dist = 0;
                    float max_trans_offset = 2;
                    float max_theta_offset = 15 * M_PI / 180;
                    for(int i = 0; i < views.size(); ++i)
                    {
                        curr_v = views[i];
                        auto step_trans = curr_v.topRightCorner<3, 1>() - last_v.topRightCorner<3, 1>();
                        double step_dist = step_trans.norm();
                        total_dist += step_dist;
                        //printf("step: %f, total: %f\n", step_dist, total_dist);

                        float x_off = sin(total_dist / novelViewNum) * max_trans_offset;
                        float z_off = 0;
                        float theta_off = -cos(total_dist / novelViewNum) * max_theta_offset;

                        auto rotation = Eigen::AngleAxis<float>(theta_off, Eigen::Vector3f(0, -1, 0));
                        auto translation = Eigen::Translation3f(x_off, 0, z_off);

                        Eigen::Transform<float, 3, Eigen::Affine> T;
                        T = translation * rotation;
                        novelViews.emplace_back(curr_v * T.matrix());

                        last_v = curr_v;
                    }

                    S_shaped_novel = true;
                }

                //=== If acquire novel images
                if(pangolin::Pushed(*gui.acquireNovelImage))
                {
                    std::string data_path = "../output/novel";  // todo

                    if(S_shaped_novel)
                    {
                        // remove the start 4 frames
                        std::vector<Eigen::Matrix4f> render_views(novelViews.begin() + 4, novelViews.end());

                        core.acquireImages(data_path, render_views,
                                           Config::W(), Config::H(),
                                           Config::fx(), Config::fy(),
                                           Config::cx(), Config::cy(),
                                           lastRestartId + 4);
                        printf("|==== Novel images from frame %d to %d are saved. ====|\n", lastRestartId + 4, globalId);
                    }
                    else
                    {
                        core.acquireImages(data_path, novelViews,
                                           Config::W(), Config::H(),
                                           Config::fx(), Config::fy(),
                                           Config::cx(), Config::cy(),
                                           totalNovelViewNum);
                        printf("|==== Novel images from frame %d to %d are saved. ====|\n", totalNovelViewNum, totalNovelViewNum + novelViews.size() - 1);

                        totalNovelViewNum += novelViews.size();
                    }

                    usleep(10000);
                }


            }


            //============ overview
            if(!follow_pose && gui.overview->Get())
            {
                pangolin::OpenGlMatrix mv;

                Eigen::Matrix3f currRot = pose.topLeftCorner(3, 3);

                Eigen::Vector3f forwardVector(0, 0, 1);
                Eigen::Vector3f upVector(0, -1, 0);

                Eigen::Vector3f forward = (currRot * forwardVector).normalized();
                Eigen::Vector3f up = (currRot * upVector).normalized();

                Eigen::Vector3f viewAt(pose(0, 3), pose(1, 3) - 5, pose(2, 3));

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

                overviewId = ++overviewId % modelPoses.size();

                usleep(100000);
            }



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