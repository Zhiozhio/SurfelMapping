//
// Created by zhijun on 2021/7/27.
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
    //============ Here is GUI ============//
    if(gui.getMode() == GUI::ShowMode::minimum)
        return;


    if(gui.getMode() == GUI::ShowMode::supervision)
    {
        pangolin::GlTexture *rgb = core.getTexture(GPUTexture::RGB);
        pangolin::GlTexture *depth = core.getTexture(GPUTexture::DEPTH_METRIC);
        pangolin::GlTexture *semantic = core.getTexture(GPUTexture::SEMANTIC);
        pangolin::GlTexture *filter = core.getTexture(GPUTexture::DEPTH_FILTERED);
        pangolin::GlTexture *last = core.getTexture("LAST");

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

                //=== If acquire images
                if(pangolin::Pushed(*gui.acquirePairedImage))
                {
                    std::string data_path = "../output/";  // todo

                    std::vector<Eigen::Matrix4f> views;
                    int start_id = gui.getViews(views, core.getHistoryPoses());  // todo

                    core.acquireImages(data_path, views, Config::W(), Config::H(),
                                                         Config::fx(), Config::fy(),
                                                         Config::cx(), Config::cy(),
                                                         lastRestartId);

                    printf("|==== %d frames are saved. ====|\n", views.size());
                    usleep(50000);
                }


                gui.postCall();

                initView = false;
            }

            //float backColor[4] = {0.05, 0.05, 0.3, 0.0f};
            float backColor[4] = {0.f, 0.f, 0.f, 0.0f};
            gui.preCall(backColor);

            //============ draw single frame surfel point cloud
            int cloud_mode = gui.drawRawCloud->Get();
            if(cloud_mode)
                core.getFeedbackBuffer(FeedbackBuffer::RAW)->render(gui.s_cam.GetProjectionModelViewMatrix(),
                                                                    pose,
                                                                    cloud_mode == 2,
                                                                    cloud_mode == 3,
                                                                    cloud_mode == 4,
                                                                    cloud_mode == 5);

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
            //gui.normalizeDepth(depth, Config::nearClip(), 50);
            gui.displayImg("depth", gui.depthNormTexture);
            //gui.displayImg("depth", mask);

            gui.processSemantic(semantic);
            gui.displayImg("semantic", gui.semanticTexture);


            gui.postCall();



            //====== clean model
            if(pangolin::Pushed(*gui.clean))
            {
                core.setBeginCleanPoints();
                return;
            }


            //====== Save model
            if(pangolin::Pushed(*gui.save))
            {
                std::string output_path = "../maps/";  // todo

                time_t rawtime;
                struct tm *info;

                time(&rawtime);
                info = gmtime(&rawtime);

                std::string file_name = "surfel_map_" + to_string(info->tm_year + 1900) + "_"
                                                      + to_string(info->tm_mon + 1) + "_"
                                                      + to_string(info->tm_mday) + "_"
                                                      + to_string(info->tm_hour)
                                                      + to_string(info->tm_min)
                                                      + to_string(info->tm_sec) + ".bin";

                output_path += file_name;

                core.getGlobalModel().downloadMap(output_path, lastRestartId, globalId);
            }

            //====== Reset
            if(pangolin::Pushed(*gui.reset))
            {
                lastRestartId = globalId + 1;
                core.reset();
                cout << "The whole model has been RESET!" << endl;
            }


            run_gui = gui.pause->Get() && !pangolin::Pushed(*gui.step);
        }


    }
}



int main(int argc, char ** argv)
{
    std::string kittiDir(argv[1]);

    KittiReader reader(kittiDir, false, true, 0, true);

    // Initialize the Config in first call with correct arguments
    Config::getInstance(reader.fx(), reader.fy(), reader.cx(), reader.cy(), reader.H(), reader.W());

    GUI gui(reader.W(), reader.H(), GUI::ShowMode::supervision);

    SurfelMapping core;

    CheckGlDieOnError();

    // if you want to start from middle, set this.
    //lastRestartId = 3282;
    reader.setState(lastRestartId);

    while (reader.getNext())
    {
        //============ Process Current Frame ============//
        cout << reader.currentFrameId << '\n';

        globalId = reader.currentFrameId;

        core.processFrame(reader.rgb, reader.depth, reader.semantic, &reader.gtPose);

        // show what you want
        rungui(core, gui);

        if(core.getBeginCleanPoints())
        {
            // clean points
            reader.saveState();

            while (reader.getLast())
            {
                cout << reader.currentFrameId << '\n';

                core.cleanPoints(reader.depth, reader.semantic, &reader.gtPose);  // here we unset the "beginCleanPoints"

                rungui(core, gui);

                if(!core.getBeginCleanPoints())
                    break;

                usleep(10000);
            }

            reader.resumeState();
        }

        usleep(10000);

    }

    // show after loop
    while(true)
    {
        rungui(core, gui);
    }


}