//
// Created by zhijun on 2021/6/15.
//

#include "Checker.h"

#include <ctime>
#include <cstdlib>

Checker::Checker()
{}

Checker::~Checker()
{
    for(auto & item : texturefs)
    {
        delete [] item.second;
    }
}

void Checker::genRandomIds(int num, int min, int max)
{
    if(ids.size())
    {
        ids.clear();
        ids_set.clear();
    }

    srand((unsigned)time(NULL));

    while (num-- > 0)
    {
        unsigned int id = rand() % (max - min) + min;
        if(ids_set.find(id) == ids_set.end())
        {
            ids_set.insert(id);
            ids.push_back(id);
        }
    }
}

void Checker::retrieveTexture4f(std::string name, pangolin::GlTexture *img, int num)
{
    auto iter = texturefs.find(name);
    if(iter != texturefs.end())
    {
        // update
        img->Download(iter->second, GL_RGBA, GL_FLOAT);

        texNum[name] = num;
    }
    else
    {
        // new
        float * texture = new float[img->height * img->width * 4];

        img->Download(texture, GL_RGBA, GL_FLOAT);

        texturefs[name] = texture;
        texNum[name] = num;

        CheckGlDieOnError();
    }
}

void Checker::retrieveTexture1i(std::string name, pangolin::GlTexture *img)
{
    auto iter = textureis.find(name);
    if(iter != textureis.end())
    {
        // update
        img->Download(iter->second, GL_RED, GL_INT);
    }
    else
    {
        // new
        int * texture = new int[img->height * img->width];

        img->Download(texture, GL_RED_INTEGER, GL_INT);

        textureis[name] = texture;
        texNum[name] = img->width * img->height;

        CheckGlDieOnError();
    }
}

void Checker::retrieveVertexf(std::string name, GLuint vbo, int num)
{
    auto iter = vertexfs.find(name);
    if(iter != vertexfs.end())
    {
        // update
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo);
        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, num * Config::vertexSize(), iter->second);

        vertexNums[name] = num;
    }
    else
    {
        // new
        float * vertex = new float[Config::numPixels() * 4 * 12];

        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo);
        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, num * Config::vertexSize(), vertex);

        vertexfs[name] = vertex;
        vertexNums[name] = num;
    }

    CheckGlDieOnError();
}

void Checker::showIds()
{
    for(int i = 0; i < ids.size(); ++i)
    {
        printf("| %51d |", ids[i]);
    }
    printf("\n");
}

void Checker::showTexture4fRandom(std::string name)
{
    float * img = texturefs[name];
    int c = 4;

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * c;
        printf("| %12.2f %12.2f %12.2f %12.2f |", img[base], img[base + 1], img[base + 2], img[base + 3]);
    }
    printf("\n");
}

void Checker::showTexture1iRandom(std::string name)
{
    int * img = textureis[name];
    int c = 1;

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * c;
        printf("| %10d |", img[base]);
    }
    printf("\n");
}

void Checker::showTexturebyID(std::string name, std::vector<int> &IDs)
{
    float * img = texturefs[name];
    int c = 4;

    for(int i = 0; i < IDs.size(); ++i)
    {
        unsigned int base = IDs[i] * c;
        printf("| %12.2f %12.2f %12.2f %12.2f |", img[base], img[base + 1], img[base + 2], img[base + 3]);
    }
    printf("\n");
}

void Checker::showVertexfRandom(std::string name)
{
    float * vert = vertexfs[name];
    int stride = 4 * 3;

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * stride;
        printf("| %12.2f %12.2f %12.2f %12.2f |", vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
    }
    printf("\n");

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * stride + 4;
        printf("| %12.2f %12.2f %12.2f %12.2f |", vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
    }
    printf("\n");

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * stride + 8;
        printf("| %12.2f %12.2f %12.2f %12.2f |", vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
    }
    printf("\n");
}

void Checker::showVertexfbyID(std::string name, std::vector<int> &IDs)
{
    float * vert = vertexfs[name];
    int stride = 4 * 3;

    for(int i = 0; i < IDs.size(); ++i)
    {
        unsigned int base = IDs[i] * stride;
        if(IDs[i] >= 0)
            printf("| %12.2f %12.2f %12.2f %12.2f |", vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
        else
            printf("| %12.2f %12.2f %12.2f %12.2f |", -3., -3., -3., -3.);
    }
    printf("\n");

    for(int i = 0; i < IDs.size(); ++i)
    {
        unsigned int base = IDs[i] * stride + 4;
        if(IDs[i] >= 0)
            printf("| %12.2f %12.2f %12.2f %12.2f |", vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
        else
            printf("| %12.2f %12.2f %12.2f %12.2f |", -2., -2., -2., -2.);
    }
    printf("\n");

    for(int i = 0; i < IDs.size(); ++i)
    {
        unsigned int base = IDs[i] * stride + 8;
        if(IDs[i] >= 0)
            printf("| %12.2f %12.2f %12.2f %12.2f |", vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
        else
            printf("| %12.2f %12.2f %12.2f %12.2f |", -1., -1., -1., -1.);
    }
    printf("\n");
}

void Checker::checkTexture4f(std::string name, int lbound, int hbound)
{
    if(hbound == 0)
        hbound = texNum[name];

    negativeIds.clear();
    zeroIds.clear();

    float * buffer = texturefs[name];

    int negative(0);
    int negativeMinId = 10000000;
    int negativeMaxId = 0;

    int zero(0);
    int zeroMinId = 10000000;
    int zeroMaxId = 0;

    for(int i = lbound; i < hbound; ++i)
    {
        float color_y = buffer[i * 4 + 1];
        if (color_y < 0)
        {
            ++negative;
            if (i > negativeMaxId) negativeMaxId = i;
            if (i < negativeMinId) negativeMinId = i;

            negativeIds.push_back(i);
        }
        if (color_y == 0.0)
        {
            ++zero;
            if (i > zeroMaxId) zeroMaxId = i;
            if (i < zeroMinId) zeroMinId = i;

            zeroIds.push_back(i);
        }
    }

    printf("Total: %d, abnormal negative: %d, zero: %d\n", hbound - lbound, negative, zero);
    if(negative)
        printf("abnormal ids from %d to %d, range %d\n", negativeMinId, negativeMaxId, negativeMaxId - negativeMinId + 1);
}

void Checker::checkVertexf(const std::string& name, int lbound, int hbound)
{
    if(hbound == 0)
        hbound = vertexNums[name];

    negativeIds.clear();
    zeroIds.clear();

    float * buffer = vertexfs[name];

    int negative(0);
    int negativeMinId = 10000000;
    int negativeMaxId = 0;

    int zero(0);
    int zeroMinId = 10000000;
    int zeroMaxId = 0;

    for(int i = lbound; i < hbound; ++i)
    {
        float color_y = buffer[i * 12 + 5];
        if (color_y < 0)
        {
            ++negative;
            if (i > negativeMaxId) negativeMaxId = i;
            if (i < negativeMinId) negativeMinId = i;

            negativeIds.push_back(i);
        }
        if (color_y == 0.0)
        {
            ++zero;
            if (i > zeroMaxId) zeroMaxId = i;
            if (i < zeroMinId) zeroMinId = i;

            zeroIds.push_back(i);
        }
    }

    printf("Total: %d, abnormal negative: %d, zero: %d\n", hbound - lbound, negative, zero);
    if(negative)
        printf("abnormal ids from %d to %d, range %d\n", negativeMinId, negativeMaxId, negativeMaxId - negativeMinId + 1);
}
