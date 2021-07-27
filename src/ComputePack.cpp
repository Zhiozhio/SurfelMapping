/*
 * This file is part of SurfelMapping.
 *
 * Copyright (C) 2015 Imperial College London
 * 
 * The use of the code within this file and all code within files that 
 * make up the software that is SurfelMapping is permitted for
 * non-commercial purposes only.  The full terms and conditions that 
 * apply to the code within this file are detailed within the LICENSE.txt 
 * file and at <http://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/elastic-fusion-license/> 
 * unless explicitly stated.  By downloading this file you agree to 
 * comply with these terms.
 *
 * If you wish to use any of this code for commercial purposes then 
 * please email researchcontracts.engineering@imperial.ac.uk.
 *
 */

#include "ComputePack.h"

const std::string ComputePack::FILTER = "FILTER";
const std::string ComputePack::METRIC = "METRIC";
const std::string ComputePack::SMOOTH = "SMOOTH";

ComputePack::ComputePack(std::shared_ptr<Shader> program)
 : program(program)
{

}

ComputePack::~ComputePack()
{

}

void ComputePack::compute(pangolin::GlTexture * target, pangolin::GlTexture * input, const std::vector<Uniform> * const uniforms)
{
    pangolin::GlFramebuffer frameBuffer;
    pangolin::GlRenderBuffer renderBuffer(target->width, target->height);
    frameBuffer.AttachColour(*target);
    frameBuffer.AttachDepth(renderBuffer);

    input->Bind();

    frameBuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, renderBuffer.width, renderBuffer.height);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    program->Bind();

    if(uniforms)
    {
        for(size_t i = 0; i < uniforms->size(); i++)
        {
            program->setUniform(uniforms->at(i));
        }
    }

    glDrawArrays(GL_POINTS, 0, 1);

    program->Unbind();

    frameBuffer.Unbind();

    glBindTexture(GL_TEXTURE_2D, 0);

    glPopAttrib();

    glFinish();

    CheckGlDieOnError()
}

void ComputePack::compute(pangolin::GlTexture *target, const std::vector<pangolin::GlTexture *> &inputs, const std::vector<Uniform> *uniforms)
{
    pangolin::GlFramebuffer frameBuffer;
    pangolin::GlRenderBuffer renderBuffer(target->width, target->height);
    frameBuffer.AttachColour(*target);
    frameBuffer.AttachDepth(renderBuffer);

    glPushAttrib(GL_VIEWPORT_BIT);
    glViewport(0, 0, renderBuffer.width, renderBuffer.height);

    program->Bind();
    if(uniforms)
    {
        for(size_t i = 0; i < uniforms->size(); i++)
        {
            program->setUniform(uniforms->at(i));
        }
    }

    GLenum textureNum = GL_TEXTURE0;
    for(unsigned int i = 0; i < inputs.size(); ++i)
    {
        glActiveTexture(textureNum + i);
        inputs[i]->Bind();
    }

    frameBuffer.Bind();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDrawArrays(GL_POINTS, 0, 1);

    frameBuffer.Unbind();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    program->Unbind();

    glPopAttrib();

    glFinish();

    CheckGlDieOnError()
}
