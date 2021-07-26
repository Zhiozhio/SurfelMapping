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
        pangolin::GlTexture *depth = core.getTexture(GPUTexture::DEPTH_METRIC);
        pangolin::GlTexture *semantic = core.getTexture("SEMANTIC");

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

//            cout << gui.s_cam.GetModelViewMatrix() << endl;

            gui.drawCapacity(core.getGlobalModel().getModelMapNR());

            gui.drawFrustum(core.getCurrPose());



            //============ draw raw image data (must be after "cam")
            gui.displayImg("rgb", rgb);
            //gui.normalizeDepth(rgb, 1.0f, 60.0f);
            //gui.displayImg("rgb", gui.getDepthNorm());

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

    CheckGlDieOnError();


    int lastModel = 0;
    while (reader.getNext())
    {
        // III. test ground truth
        if(false)
            cout << reader.gtPose << endl;

        //============ Process Current Frame ============//
        cout << reader.currentFrameId << '\n';

        core.processFrame(reader.rgb, reader.depth, reader.semantic, &reader.gtPose);


/*
        if(reader.currentFrameId > 0)
        {
            core.checker->genRandomIds(20, 0, core.checker->vertexNums["Conf"]);
            core.checker->showIds();
            vector<int> layout{1, 4};
            core.checker->showVertexfRandom("Conf", 5, layout);

            vector<int> vertex_ids;
            float * vert = core.checker->vertexfs["Conf"];
            for(auto id : core.checker->ids)
            {
                unsigned int base = id * 5;
                int * the_value = reinterpret_cast<int *>(vert + base);
                vertex_ids.push_back(*the_value);
                //cout << *the_value << " | ";
            }

            core.checker->showTexturebyID("VertConf", vertex_ids);



        }
*/  // debug Conflict


/*
        core.checker->genRandomIds(20, 0, core.checker->texNum["sem"]);
        core.checker->showTextureuRandom("sem", 1);
*/  // debug Semantic

/*
        cout << "dataProgram get vertices (fuse | new): " << core.getGlobalModel().getData().second << '\n';
        cout << "conflict vertices: " << core.getGlobalModel().getConflict().second << '\n';
        cout << "the offset (old remainder): " << core.getGlobalModel().getOffset() << " "
             << "(so the removed is: " << lastModel - core.getGlobalModel().getOffset() << ")" <<'\n';
        cout << "the new vertices: " << core.getGlobalModel().getUnstable().second << " "
             << "(so the fused is: " << core.getGlobalModel().getData().second - core.getGlobalModel().getUnstable().second << ")" << '\n';
        cout << "==> model vbo vertices:" << core.getGlobalModel().getModel().second << endl;

        lastModel = core.getGlobalModel().getModel().second;


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



//            cout << "Unstable VBO: ";
//            core.checker->checkVertexf("Unstable");

            cout << "---- Data VBO - The Fused ---- " << endl;
            showIds(updateSample);
            showIds(updateId);
            core.checker->showVertexfbyID("Data", updateSample);

            cout << "Model map II - The fused: " << endl;
            core.checker->showTexturebyID("PosConfII", updateId);
            core.checker->showTexturebyID("ColorTimeII", updateId);
            core.checker->showTexturebyID("NormRadII", updateId);
//
//            cout << "Removal: " << endl;
//            showIds(removeId);
//            core.checker->showVertexfbyID(core.getGlobalModel().getData().first, removeSample);


            cout << "---- Conflict VBO ---- " << endl;
            core.checker->genRandomIds(20, 0, core.getGlobalModel().getConflict().second);
            //            core.checker->showIds();
            vector<int> layout{1, 4};
            core.checker->showVertexfRandom("Conflict", 5, layout);


            cout << "Model map II - The Removal: " << endl;
            removeId.clear();
            for(auto id : core.checker->ids)
            {
                unsigned int stride = 5;
                unsigned int offset = 0;
                removeId.push_back(static_cast<int>(core.checker->vertexfs["Conflict"][id * stride + offset]));
            }
            core.checker->showTexturebyID("PosConfII", removeId);
            core.checker->showTexturebyID("ColorTimeII", removeId);
            core.checker->showTexturebyID("NormRadII", removeId);



            //cout << "---- Model map II ---- " << endl;
            //core.checker->checkTexture4f("ColorTimeII");

        }

*/  // debug GlobalModel

        // show what you want
        rungui(core, gui);

        usleep(50000);

    }

    // show after loop
    rungui(core, gui);


}