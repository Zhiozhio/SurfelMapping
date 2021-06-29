//
// Created by zhijun on 2021/6/15.
//

#ifndef SURFELMAPPING_CHECKER_H
#define SURFELMAPPING_CHECKER_H

#include "Config.h"

#include <pangolin/pangolin.h>

#include <map>

class Checker
{
public:
    Checker();

    ~Checker();


    void retrieveTexture4f(std::string name, pangolin::GlTexture * img, int num);

    void retrieveTexture1i(std::string name, pangolin::GlTexture * img);

    void retrieveVertexf(std::string name, GLuint vbo, int num);

    void genRandomIds(int num, int min, int max);

    void showIds();

    void showTexture4fRandom(std::string name);

    void showTexture1iRandom(std::string name);

    void showTexturebyID(std::string name, std::vector<int> &IDs);

    void showVertexfRandom(std::string name);

    void showVertexfbyID(std::string name, std::vector<int> &IDs);

    void checkTexture4f(std::string name, int lbound = 0, int hbound = 0);

    void checkVertexf(const std::string& name, int lbound = 0, int hbound = 0);





    std::vector<unsigned int> ids;
    std::set<unsigned int> ids_set;

    std::map<std::string, int> texNum;
    std::map<std::string, float *> texturefs;
    std::map<std::string, int *> textureis;

    std::map<std::string, int> vertexNums;
    std::map<std::string, float *> vertexfs;

    std::map<std::string, int *> vertexis;

    std::vector<int> negativeIds;
    std::vector<int> zeroIds;


};

#endif //SURFELMAPPING_CHECKER_H
