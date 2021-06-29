//
// Created by zhijun on 2021/4/24.
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

using namespace std;


vector<int> unstableSample, updateSample, removeSample;
vector<int> unstableId,     updateId,     removeId;
vector<int> matchedId, matchedIdBuffer;
vector<int> abnormalNegativeId;

int myid;


void showIds(vector<int> &IDs)
{
    for(int i = 0; i < IDs.size(); ++i)
    {
        printf("| %51d |", IDs[i]);
    }
    printf("\n");
}

void checkDataTypes(float * data, int num)
{
    int unstable(0), update(0), remove(0);

    unstableSample.clear();
    updateSample.clear();
    removeSample.clear();

    unstableId.clear();
    updateId.clear();
    removeId.clear();

    for(int i = 0; i < num; ++i)
    {
        float color_y = data[i * 12 + 5];  // colorTime.y
        float pos_w = data[i * 12 + 3];    // posConf.w
        if(color_y >= 0.0)
            if(pos_w <= 0.0)
            {
                ++remove;
                if(removeSample.size() < 20)
                {
                    removeSample.push_back(i);
                    removeId.push_back(static_cast<int>(color_y));
                }
            }
            else
            {
                ++update;
                if(updateSample.size() < 20)
                {
                    updateSample.push_back(i);
                    updateId.push_back(static_cast<int>(color_y));
                }
            }
        else
        {
            ++unstable;
            unstableId.push_back(i);
        }
    }

    printf("unstable: %d, update: %d, remove: %d\n", unstable, update, remove);
}

/// model is model II/New, not texture map
void checkBackMapping(float * model, unsigned int lbound, unsigned int hbound, vector<unsigned int> & ids)
{
    matchedId.clear();
    abnormalNegativeId.clear();

    // time consuming
    //auto s2(s);  // a copy

    int abnormalNegative(0);
    int abnormalMinId = 1000000;
    int abnormalMaxId = 0;

    matchedId.insert(matchedId.end(), ids.size(), -1);

    for(int i = lbound; i < hbound; ++i)
    {
        float color_y = model[i * 12 + 5];
        if(color_y < 0)
        {
            ++abnormalNegative;
            if(i > abnormalMaxId) abnormalMaxId = i;
            if(i < abnormalMinId) abnormalMinId = i;

            if(abnormalNegativeId.size() < 20)
                abnormalNegativeId.push_back(i);
        }

        unsigned int id = static_cast<unsigned int>(color_y);  // find the model map II ID
        auto it = find(ids.begin(), ids.end(), id);
        if(it != ids.end())
        {
            std::size_t n = it - ids.begin();
            matchedId[n] = i;
            //s2.erase(id);
        }
    }

    printf("abnormal negative: %d\n", abnormalNegative);
    if(abnormalNegative)
        printf("abnormal ids from %d to %d, range %d\n", abnormalMinId, abnormalMaxId, abnormalMaxId - abnormalMinId + 1);

    myid = abnormalMaxId;
}


void rungui(SurfelMapping & core, GUI & gui)
{
//    cv::Mat debug(Config::H(), Config::W(), CV_16UC1);

    //============ Here is GUI ============//
    if(gui.getMode() == GUI::ShowMode::minimum)
        return;


    if(gui.getMode() == GUI::ShowMode::supervision)
    {
        pangolin::GlTexture *rgb = core.getTexture(GPUTexture::RGB);
        pangolin::GlTexture *depth = core.getTexture(GPUTexture::DEPTH_METRIC_FILTERED);
        pangolin::GlTexture *index = core.getIndexMap().indexTex()->texture;

        Eigen::Matrix4f pose = core.getCurrPose();



        bool step = pangolin::Pushed(*gui.step);

        bool run_gui = true;
        while(run_gui)
        {
            if(gui.followPose->Get())
            {
                pangolin::OpenGlMatrix mv;

                Eigen::Matrix3f currRot = pose.topLeftCorner(3, 3);

                Eigen::Quaternionf currQuat(currRot);
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



            gui.preCall();

            //============ draw single frame surfel point cloud
            int cloud_mode = gui.drawRawCloud->Get();
            if(cloud_mode)
                core.getFeedbackBuffer(FeedbackBuffer::RAW)->render(gui.s_cam.GetProjectionModelViewMatrix(),
                                                                    pose,
                                                                    cloud_mode == 2,
                                                                    cloud_mode == 3,
                                                                    cloud_mode == 4);
            cloud_mode = gui.drawFilteredCloud->Get();
            if(cloud_mode)
                core.getFeedbackBuffer(FeedbackBuffer::FILTERED)->render(gui.s_cam.GetProjectionModelViewMatrix(),
                                                                         pose,
                                                                         cloud_mode == 2,
                                                                         cloud_mode == 3,
                                                                         cloud_mode == 4);

            //============ draw global model
            int surfel_mode = gui.drawGlobalModel->Get();

            if(surfel_mode)
                core.getGlobalModel().renderModel(gui.s_cam.GetProjectionModelViewMatrix(),
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

            //            gui.normalizeDepth(rgb, 1.0f, 60.0f);
            //            gui.displayImg("rgb", gui.getDepthNorm());

            gui.normalizeDepth(depth, 1.0f, 60.0f);


            //                int * index_buffer = new int[Config::numPixels()];
            //
            //                depth->Download(index_buffer, GL_RED, GL_R16UI);
            //
            //                for(int i = 0; i < 100; ++i)
            //                {
            //                    printf("%d ", index_buffer[rand() % Config::numPixels()]);
            //                }
            //                printf("\n");
            //
            //                cv::imshow("DEBUG", debug);
            //                cv::waitKey(0);


            gui.displayImg("depth", gui.depthNormTexture);

            gui.postCall();

            run_gui = gui.pause->Get() && !pangolin::Pushed(*gui.step);
        }


    }
}



int main(int argc, char ** argv)
{
    std::string kittiDir(argv[1]);

    KittiReader reader(kittiDir, false, 0, true);

    // I. test reader
    if(false)
    {
        printf("%f, %f, %f, %f\n", reader.fx(), reader.fy(), reader.cx(), reader.cy());
        printf("%d, %d, %d\n",reader.H(), reader.W(), reader.numPixels());

        for(int i = 0; i < 10; ++i)
        {
            cout << (*reader.getGroundTruth())[i] << endl;
        }
    }

    // Initialize the Config in first call with correct arguments
    Config::getInstance(reader.fx(), reader.fy(), reader.cx(), reader.cy(), reader.H(), reader.W());

    // II. test Config
    if(false)
    {
        printf("%f, %f, %f, %f\n", Config::fx(), Config::fy(), Config::cx(), Config::cy());
        printf("%d, %d, %d\n",Config::W(), Config::H(), Config::numPixels());
        printf("%d, %f, %s\n", Config::vertexSize(), Config::farClip(), Config::shaderDir().c_str());
    }

    GUI gui(reader.W(), reader.H(), GUI::ShowMode::supervision);

    CheckGlDieOnError();

    SurfelMapping core;


    while (reader.getNext())
    {
        // III. test ground truth
        if(false)
            cout << reader.gtPose << endl;

        //============ Process Current Frame ============//
        core.processFrame(reader.rgb, reader.depth, &reader.gtPose);




        cout << reader.currentFrameId << '\n';
        cout << "dataProgram get vertices: " << core.getGlobalModel().getDataCount() << '\n';
        cout << "the offset (old remainder): " << core.getGlobalModel().getOffset() << '\n';
        cout << "==> model vbo vertices:" << core.getGlobalModel().getCount() << endl;

/*
        // Checker analysis
//        core.checker->genRandomIds(20, core.getGlobalModel().getCount());

//        core.checker->showIds();
//        core.checker->showVertexfRandom(core.getGlobalModel().getModel().first);
//        core.checker->showTexture4fRandom("PosConf");
//        core.checker->showTexture4fRandom("ColorTime");
//        core.checker->showTexture4fRandom("NormRad");


        if(reader.currentFrameId > 0)
        {
//            core.checker->genRandomIds(20, Config::numPixels());
//            cout << "Index Map: " << endl;
//            core.checker->showIds();
//            core.checker->showTexture1iRandom("IndexMap");

            cout << "Data VBO:  ";
            checkDataTypes(core.checker->vertexfs["Data"], core.getGlobalModel().getData().second);

            cout << "Unstable VBO: ";
            core.checker->checkVertexf("Unstable");

//            cout << "Update: " << endl;
//            showIds(updateId);
//            core.checker->showVertexfbyID(core.getGlobalModel().getData().first, updateSample);
//
//            cout << "Model map II - The Updated: " << endl;
//            core.checker->showTexturebyID("PosConfII", updateId);
//            core.checker->showTexturebyID("ColorTimeII", updateId);
//            core.checker->showTexturebyID("NormRadII", updateId);
//
//            cout << "Removal: " << endl;
//            showIds(removeId);
//            core.checker->showVertexfbyID(core.getGlobalModel().getData().first, removeSample);
//
//            cout << "Model map II - The Removal: " << endl;
//            core.checker->showTexturebyID("PosConfII", removeId);
//            core.checker->showTexturebyID("ColorTimeII", removeId);
//            core.checker->showTexturebyID("NormRadII", removeId);



            //cout << "---- Model map II ---- " << endl;
            //core.checker->checkTexture4f("ColorTimeII");


            cout << "---- Model II VBO ----  " << endl;
            core.checker->checkVertexf("ModelII");

            cout << "---- Model New VBO ----  " << endl;
            core.checker->checkVertexf("Model New");

            cout << "Model New the updated part: " << endl;
            core.checker->checkVertexf("Model New", 0, core.getGlobalModel().getOffset());

            cout << "Model New the new part: " << endl;
            core.checker->checkVertexf("Model New", core.getGlobalModel().getOffset(), core.getGlobalModel().getCount());


            cout << "==== New part from Data VBO: " << endl;
            //vector<int> dataNew(unstableId.begin(), unstableId.begin() + 20);
            //vector<int> dataNew(unstableId.begin() + 3002 - 44 - 20, unstableId.begin() + 3002 - 44);
            vector<int> dataNew(unstableId.begin() + 3002 - 44 - 1, unstableId.begin() + 3002 - 44 - 1 + 20);
            core.checker->showVertexfbyID("Data", dataNew);

            cout << "==== New part in Unstable VBO: " << endl;
            vector<int> unstableNew;
            for(int i = 0; i < 20; ++i)
            {
                //unstableNew.push_back(i);
                //modelNew.push_back(i + core.getGlobalModel().getOffset() + 3002 - 44 - 20);
                unstableNew.push_back(i + 3002 - 44 - 1);
            }
            core.checker->showVertexfbyID("Unstable", unstableNew);

            cout << "==== New part from Model New: " << endl;
            vector<int> modelNew;
            for(int i = 0; i < 20; ++i)
            {
                //modelNew.push_back(i + core.getGlobalModel().getOffset());
                //modelNew.push_back(i + core.getGlobalModel().getOffset() + 3002 - 44 - 20);
                modelNew.push_back(i + core.getGlobalModel().getOffset() + 3002 - 44 - 1);
            }
            core.checker->showVertexfbyID("Model New", modelNew);

        }

*/

        // show what you want
        rungui(core, gui);

        usleep(50000);

    }

    // show after loop
    rungui(core, gui);


}