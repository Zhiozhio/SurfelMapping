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


    void retrieveTexture1u(std::string name, pangolin::GlTexture * img, int num = 0);

    void retrieveTexture1f(std::string name, pangolin::GlTexture * img, int num = 0);

    void retrieveTexture2f(std::string name, pangolin::GlTexture * img, int num = 0);

    void retrieveTexture4f(std::string name, pangolin::GlTexture * img, int num = 0);

    void retrieveTexture1i(std::string name, pangolin::GlTexture * img);

    void retrieveVertexf(std::string name, GLuint vbo, int num);

    void genRandomIds(int num, int min, int max);

    void showIds();

    void showTexturefRandom(std::string name, int channel);

    void showTextureuRandom(std::string name, int channel);

    void showTexturebyID(std::string name, std::vector<int> &IDs);

    void showVertexfRandom(std::string name, int stride, std::vector<int> & layout);

    void showVertexfbyID(std::string name, std::vector<int> &IDs);

    void checkTexture4f(std::string name, int lbound = 0, int hbound = 0);

    void checkVertexf(const std::string& name, int lbound = 0, int hbound = 0);

    void histogramTexturef(const std::string& name, float lbound, float hbound, int numSections, bool countBound, int numChannel, int channel);





    std::vector<unsigned int> ids;
    std::set<unsigned int> ids_set;

    std::map<std::string, int> texNum;
    std::map<std::string, float *> texturefs;
    std::map<std::string, int *> textureis;
    std::map<std::string, unsigned int *> textureus;

    std::map<std::string, int> vertexNums;
    std::map<std::string, float *> vertexfs;

    std::map<std::string, int *> vertexis;

    std::vector<int> negativeIds;
    std::vector<int> zeroIds;

private:
    void print_layout(float a);
    void print_layout(float a, float b);
    void print_layout(float a, float b, float c, float d);

    void print_layout(unsigned int a);
    void print_layout(unsigned int a, unsigned int b);
    void print_layout(unsigned int a, unsigned int b, unsigned int c, unsigned int d);

    void print_layout(int a);
    void print_layout(int a, int b);
    void print_layout(int a, int b, int c, int d);


};

#endif //SURFELMAPPING_CHECKER_H
