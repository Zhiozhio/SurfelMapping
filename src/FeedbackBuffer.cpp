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

#include "FeedbackBuffer.h"

const std::string FeedbackBuffer::RAW = "RAW";
const std::string FeedbackBuffer::FILTERED = "FILTERED";

FeedbackBuffer::FeedbackBuffer(std::shared_ptr<Shader> program)
 : program(program),
   drawPointProgram(loadProgramFromFile("draw_feedback.vert", "draw_feedback.frag")),
   drawSurfelProgram(loadProgramFromFile("draw_surface.vert", "draw_surface.geom", "draw_surface.frag")),
   bufferSize(Config::numPixels() * Config::vertexSize()),
   count(0)
{
    float * vertices = new float[bufferSize];

    memset(&vertices[0], 0, bufferSize);

    glGenTransformFeedbacks(1, &fid);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, &vertices[0], GL_STREAM_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    delete [] vertices;

    std::vector<Eigen::Vector2f> uv;
    int width = Config::W();
    int height = Config::H();

    for(int i = 0; i < width; i++)
    {
        for(int j = 0; j < height; j++)
        {
            uv.push_back(Eigen::Vector2f(((float)i / (float)width) + 1.0 / (2 * (float)width),
                                         ((float)j / (float)height) + 1.0 / (2 * (float)height)));
        }
    }

    glGenBuffers(1, &uvo);
    glBindBuffer(GL_ARRAY_BUFFER, uvo);
    glBufferData(GL_ARRAY_BUFFER, uv.size() * sizeof(Eigen::Vector2f), &uv[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    program->Bind();

    int loc[3] =
    {
        glGetVaryingLocationNV(program->programId(), "vPosition0"),
        glGetVaryingLocationNV(program->programId(), "vColor0"),
        glGetVaryingLocationNV(program->programId(), "vNormRad0"),
    };

    glTransformFeedbackVaryingsNV(program->programId(), 3, loc, GL_INTERLEAVED_ATTRIBS);

    program->Unbind();

    glGenQueries(1, &countQuery);
}

FeedbackBuffer::~FeedbackBuffer()
{
    glDeleteTransformFeedbacks(1, &fid);
    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &uvo);
    glDeleteQueries(1, &countQuery);
}

void FeedbackBuffer::compute(pangolin::GlTexture * color,
                             pangolin::GlTexture * depth,
                             pangolin::GlTexture * semantic,
                             const int & time,
                             const float depthCutoff)
{
    program->Bind();

    Eigen::Vector4f cam(Config::cx(),
                        Config::cy(),
                        1.0f / Config::fx(),
                        1.0f / Config::fy());

    program->setUniform(Uniform("cam", cam));
    program->setUniform(Uniform("threshold", 0.0f));
    program->setUniform(Uniform("cols", (float)Config::W()));
    program->setUniform(Uniform("rows", (float)Config::H()));
    program->setUniform(Uniform("time", time));
    program->setUniform(Uniform("gSampler", 0));
    program->setUniform(Uniform("cSampler", 1));
    program->setUniform(Uniform("sSampler", 2));
    program->setUniform(Uniform("maxDepth", depthCutoff));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, uvo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, fid);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbo);

    glBeginTransformFeedback(GL_POINTS);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depth->tid);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color->tid);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, semantic->tid);

    glDrawArrays(GL_POINTS, 0, Config::numPixels());

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    glEndTransformFeedback();

    glDisable(GL_RASTERIZER_DISCARD);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    program->Unbind();

    glFinish();
}

void FeedbackBuffer::render(pangolin::OpenGlMatrix mvp,
                            const Eigen::Matrix4f & pose,
                            const bool drawNormals,
                            const bool drawColors,
                            const bool drawSemantic,
                            const bool drawSurfel)
{
    std::shared_ptr<Shader> program = drawSurfel ? drawSurfelProgram : drawPointProgram;

    program->Bind();

    program->setUniform(Uniform("MVP", mvp));
    program->setUniform(Uniform("pose", pose));
    program->setUniform(Uniform("colorType", (drawNormals ? 1 : drawColors ? 2 : drawSemantic ? 3 : drawSurfel ? 4 : 0)));

    program->setUniform(Uniform("class0", Eigen::Vector3f(128,128,128)));
    program->setUniform(Uniform("class1", Eigen::Vector3f(0,255,0)));
    program->setUniform(Uniform("class2", Eigen::Vector3f(0,0,255)));
    program->setUniform(Uniform("class3", Eigen::Vector3f(255,255,0)));
    program->setUniform(Uniform("class4", Eigen::Vector3f(128,0,0)));
    program->setUniform(Uniform("class5", Eigen::Vector3f(255,0,255)));
    program->setUniform(Uniform("class6", Eigen::Vector3f(128,128,0)));
    program->setUniform(Uniform("class7", Eigen::Vector3f(0,128,0)));
    program->setUniform(Uniform("class8", Eigen::Vector3f(128,0,128)));
    program->setUniform(Uniform("class9", Eigen::Vector3f(0,128,128)));
    program->setUniform(Uniform("class10", Eigen::Vector3f(0,255,255)));
    program->setUniform(Uniform("class11", Eigen::Vector3f(0,0,128)));
    program->setUniform(Uniform("class12", Eigen::Vector3f(245,222,179)));
    program->setUniform(Uniform("class13", Eigen::Vector3f(255,0,0)));
    program->setUniform(Uniform("class14", Eigen::Vector3f(210,105,30)));
    program->setUniform(Uniform("class15", Eigen::Vector3f(244,164,96)));
    program->setUniform(Uniform("class16", Eigen::Vector3f(119,136,153)));
    program->setUniform(Uniform("class17", Eigen::Vector3f(255,20,147)));
    program->setUniform(Uniform("class18", Eigen::Vector3f(138,43,226)));

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 1));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    glDrawTransformFeedback(GL_POINTS, fid);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    drawPointProgram->Unbind();
}
