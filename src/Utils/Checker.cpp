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

void Checker::retrieveTexture1u(std::string name, pangolin::GlTexture *img, int num)
{
    auto iter = texturefs.find(name);
    if(iter != texturefs.end())
    {
        // update
        img->Download(iter->second, GL_RED, GL_UNSIGNED_INT);

        if(num > 0)
            texNum[name] = num;
    }
    else
    {
        // new
        unsigned int * texture = new unsigned int[img->height * img->width];

        img->Download(texture, GL_RED_INTEGER, GL_UNSIGNED_INT);

        textureus[name] = texture;

        if(num == 0)
            texNum[name] = img->height * img->width;
        else
            texNum[name] = num;

        CheckGlDieOnError();
    }
}

void Checker::retrieveTexture1f(std::string name, pangolin::GlTexture *img, int num)
{
    auto iter = texturefs.find(name);
    if(iter != texturefs.end())
    {
        // update
        img->Download(iter->second, GL_RED, GL_FLOAT);

        if(num > 0)
            texNum[name] = num;
    }
    else
    {
        // new
        float * texture = new float[img->height * img->width];

        img->Download(texture, GL_RED, GL_FLOAT);

        texturefs[name] = texture;

        if(num == 0)
            texNum[name] = img->height * img->width;
        else
            texNum[name] = num;

        CheckGlDieOnError();
    }
}

void Checker::retrieveTexture2f(std::string name, pangolin::GlTexture *img, int num)
{
    auto iter = texturefs.find(name);
    if(iter != texturefs.end())
    {
        // update
        img->Download(iter->second, GL_RG, GL_FLOAT);

        if(num > 0)
            texNum[name] = num;
    }
    else
    {
        // new
        float * texture = new float[img->height * img->width * 2];

        img->Download(texture, GL_RG, GL_FLOAT);

        texturefs[name] = texture;

        if(num == 0)
            texNum[name] = img->height * img->width;
        else
            texNum[name] = num;

        CheckGlDieOnError();
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
        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, num * 5 * sizeof(float), iter->second);

        vertexNums[name] = num;
    }
    else
    {
        // new
        float * vertex = new float[Config::numPixels() * 12];

        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo);
        glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, num * 5 * sizeof(float), vertex);

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

void Checker::showTexturefRandom(std::string name, int channel)
{
    float * img = texturefs[name];

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * channel;
        switch(channel)
        {
            case 1:
                print_layout(img[base]);
                break;
            case 2:
                print_layout(img[base], img[base + 1]);
                break;
            case 4:
                print_layout(img[base], img[base + 1], img[base + 2], img[base + 3]);
                break;
            default:
                printf("[Checker::showTexturefRandom] Not valid layout!");
        }
    }
    printf("\n");
}

void Checker::showTextureuRandom(std::string name, int channel)
{
    unsigned int * img = textureus[name];

    for(int i = 0; i < ids.size(); ++i)
    {
        unsigned int base = ids[i] * channel;
        switch(channel)
        {
            case 1:
                print_layout(img[base]);
                break;
            case 2:
                print_layout(img[base], img[base + 1]);
                break;
            case 4:
                print_layout(img[base], img[base + 1], img[base + 2], img[base + 3]);
                break;
            default:
                printf("[Checker::showTexturefRandom] Not valid layout!");
        }
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

void Checker::showVertexfRandom(std::string name, int stride, std::vector<int> & layout)
{
    float * vert = vertexfs[name];

    unsigned int offset = 0;

    for(int num : layout)
    {
        for(int i = 0; i < ids.size(); ++i)
        {
            unsigned int base = ids[i] * stride + offset;
            switch(num)
            {
                case 1:
                {
                    int * the_value = reinterpret_cast<int *>(vert + base);
                    print_layout(*the_value);
                    break;
                }
                case 2:
                    print_layout(vert[base], vert[base + 1]);
                    break;
                case 4:
                    print_layout(vert[base], vert[base + 1], vert[base + 2], vert[base + 3]);
                    break;
                default:
                    printf("[Checker::showVertexfRandom] Not valid layout!");
            }
        }
        printf("\n");
        offset += num;
    }

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

void Checker::histogramTexturef(const std::string &name, float lbound, float hbound, int numSections, bool countBound, int numChannel, int channel)
{
    int sections[numSections];
    for(int i = 0; i < numSections; ++i)
        sections[i] = 0;
    int numLowBound = 0;
    int numUpBound = 0;
    int total = 0;

    float interval = (hbound - lbound) / numSections;
    float bounds[numSections + 1];
    for(int i = 0; i <= numSections; ++i)
        bounds[i] = lbound + i * interval;

    float * ptr = texturefs[name];
    for(int i = 0; i < texNum[name]; ++i)
    {
        float value = ptr[numChannel * i + channel];
        if(value < 0.001)
            value = 0;

        for(int j = 0; j < numSections; ++j)
        {
            if(value >= bounds[j] && value < bounds[j + 1])
            {
                sections[j]++;
                total++;
            }
        }
        if(value == hbound)
        {
            sections[numSections - 1]++;
            total++;
        }

        if(countBound)
        {
            if(value == lbound)
                numLowBound++;
            if(value == hbound)
                numUpBound++;
        }
    }

    for(int j = 0; j < numSections; ++j)
    {
        float frac = (float)sections[j] / total;
        printf("[%4.2f, %4.2f): %6d, %f%% ", bounds[j], bounds[j+1], sections[j], 100.0 * frac);
        for(int k = 0; k < int(100 * frac + 0.5); ++k)
        {
            printf("|");
        }
        printf("\n");
    }
    if(countBound)
    {
        printf("Left: %d, Right: %d\n", numLowBound, numUpBound);
    }
}


void Checker::print_layout(float a)
{
    printf("| %51.2f |", a);
}

void Checker::print_layout(float a, float b)
{
    printf("| %25.2f %25.2f |", a, b);
}

void Checker::print_layout(float a, float b, float c, float d)
{
    printf("| %12.2f %12.2f %12.2f %12.2f |", a, b, c, d);
}

void Checker::print_layout(unsigned int a)
{
    printf("| %53u |", a);
}

void Checker::print_layout(unsigned int a, unsigned int b)
{
    printf("| %27u %27u |", a, b);
}

void Checker::print_layout(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    printf("| %14u %14u %14u %14u |", a, b, c, d);
}

void Checker::print_layout(int a)
{
    printf("| %51d |", a);
}

void Checker::print_layout(int a, int b)
{
    printf("| %27d %27d |", a, b);
}

void Checker::print_layout(int a, int b, int c, int d)
{
    printf("| %14d %14d %14d %14d |", a, b, c, d);
}
