

#include "GlobalModel.h"
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

GlobalModel::GlobalModel()
 : TEXTURE_DIMENSION(Config::maxSqrtVertices()),
   MAX_VERTICES(TEXTURE_DIMENSION * TEXTURE_DIMENSION),
   bufferSize(MAX_VERTICES * Config::vertexSize()),
   count(0),
   offset(0),
   dataCount(0),
   conflictCount(0),
   unstableCount(0),
   texturePtr(nullptr),
   initProgram(loadProgramFromFile("init_unstable.vert")),
   modelProgram(loadProgramFromFile("map.vert", "map.frag")),
   dataProgram(loadProgramGeomFromFile("data.vert", "data.geom")),
   conflictProgram(loadProgramGeomFromFile("conflict.vert", "conflict.geom")),
   fuseProgram(loadProgramFromFile("fuse.vert", "fuse.frag")),
   updateConflictProgram(loadProgramFromFile("update_conf.vert", "update_conf.frag")),
   backMappingProgram(loadProgramGeomFromFile("back_map.vert", "back_map.geom")),
   unstableProgram(loadProgramGeomFromFile("unstable.vert", "unstable.geom")),
   drawPointProgram(loadProgramFromFile("draw_feedback.vert", "draw_feedback.frag")),
   drawSurfelProgram(loadProgramFromFile("draw_surface.vert", "draw_surface_adaptive.geom", "draw_surface.frag")),
   drawImageProgram(loadProgramFromFile("draw_image.vert", "draw_image.geom", "draw_image.frag")),
   //drawImageProgram(loadProgramFromFile("empty.vert", "quad.geom", "draw_image.frag")),
   modelMapRenderBuffer(TEXTURE_DIMENSION, TEXTURE_DIMENSION),
   vertConfRenderBuffer(TEXTURE_DIMENSION, TEXTURE_DIMENSION),
   modelMapVertsConfs(TEXTURE_DIMENSION, TEXTURE_DIMENSION, GL_RGBA32F, GL_RED, GL_FLOAT),
   modelMapColorsTime(TEXTURE_DIMENSION, TEXTURE_DIMENSION, GL_RGBA32F, GL_RED, GL_FLOAT),
   modelMapNormsRadii(TEXTURE_DIMENSION, TEXTURE_DIMENSION, GL_RGBA32F, GL_RED, GL_FLOAT)
{
    glGenTransformFeedbacks(1, &modelFid);
    glGenBuffers(1, &modelVbo);
    glBindBuffer(GL_ARRAY_BUFFER, modelVbo);
    glBufferData(GL_ARRAY_BUFFER, bufferSize, nullptr, GL_DYNAMIC_COPY);  // only allocate space
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTransformFeedbacks(1, &dataFid);
    glGenBuffers(1, &dataVbo);
    glBindBuffer(GL_ARRAY_BUFFER, dataVbo);
    glBufferData(GL_ARRAY_BUFFER, Config::numPixels() * Config::vertexSize(), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTransformFeedbacks(1, &conflictFid);
    glGenBuffers(1, &conflictVbo);
    glBindBuffer(GL_ARRAY_BUFFER, conflictVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 Config::numPixels() * (sizeof(float) + (int)(Config::vertexSize() / 3)), // id + posConf
                 nullptr,
                 GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &unstableVbo);
    glBindBuffer(GL_ARRAY_BUFFER, unstableVbo);
    glBufferData(GL_ARRAY_BUFFER, Config::numPixels() * Config::vertexSize(), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::vector<Eigen::Vector2f> uv;
    uv.reserve(Config::numPixels());
    for(int i = 0; i < Config::W(); ++i)
    {
        for(int j = 0; j < Config::H(); ++j)
        {
            uv.push_back(Eigen::Vector2f((i + 0.5) / (float)Config::W(),
                                         (j + 0.5) / (float)Config::H() ) );
        }
    }
    uvSize = uv.size();
    glGenBuffers(1, &uvo);
    glBindBuffer(GL_ARRAY_BUFFER, uvo);
    glBufferData(GL_ARRAY_BUFFER, uvSize * sizeof(Eigen::Vector2f), &uv[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    uv.clear();
    uv.reserve(MAX_VERTICES);
    /// big uvo must be row major to fit the model map
    for(int j = 0; j < TEXTURE_DIMENSION; ++j)
    {
        for(int i = 0; i < TEXTURE_DIMENSION; ++i)
        {
            uv.push_back(Eigen::Vector2f(((float)i / (float)TEXTURE_DIMENSION) + 1.0 / (2 * (float)TEXTURE_DIMENSION),
                                         ((float)j / (float)TEXTURE_DIMENSION) + 1.0 / (2 * (float)TEXTURE_DIMENSION)));
        }
    }
    bigUvSize = uv.size();
    glGenBuffers(1, &bigUvo);
    glBindBuffer(GL_ARRAY_BUFFER, bigUvo);
    glBufferData(GL_ARRAY_BUFFER, bigUvSize * sizeof(Eigen::Vector2f), &uv[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    modelMapFramebuffer.AttachColour(*modelMapVertsConfs.texture);
    modelMapFramebuffer.AttachColour(*modelMapColorsTime.texture);
    modelMapFramebuffer.AttachColour(*modelMapNormsRadii.texture);
    modelMapFramebuffer.AttachDepth(modelMapRenderBuffer);

    vertConfFrameBuffer.AttachColour(*modelMapVertsConfs.texture);
    vertConfFrameBuffer.AttachDepth(modelMapRenderBuffer);

//    setImageSize(Config::W(), Config::H(), Config::fx(), Config::fy(), Config::cx(), Config::cy());
//    imageFramebuffer.AttachColour(imageTexture);
//    imageFramebuffer.AttachColour(semanticTexture);
//    imageFramebuffer.AttachColour(depthTexture);
//    imageFramebuffer.AttachDepth(imageRenderBuffer);

    //------------- specify the retrieved varyings
    initProgram->Bind();
    int locInit[3] =
            {
                    glGetVaryingLocationNV(initProgram->programId(), "vPosition0"),
                    glGetVaryingLocationNV(initProgram->programId(), "vColor0"),
                    glGetVaryingLocationNV(initProgram->programId(), "vNormRad0")
            };
    glTransformFeedbackVaryingsNV(initProgram->programId(), 3, locInit, GL_INTERLEAVED_ATTRIBS);
    initProgram->Unbind();

    dataProgram->Bind();
    int dataUpdate[3] =
            {
                    glGetVaryingLocationNV(dataProgram->programId(), "vPosition0"),
                    glGetVaryingLocationNV(dataProgram->programId(), "vColor0"),
                    glGetVaryingLocationNV(dataProgram->programId(), "vNormRad0")
            };
    glTransformFeedbackVaryingsNV(dataProgram->programId(), 3, dataUpdate, GL_INTERLEAVED_ATTRIBS);
    dataProgram->Unbind();

    conflictProgram->Bind();
    int conflictUpdate[2] =
            {
                    glGetVaryingLocationNV(conflictProgram->programId(), "conflictId0"),
                    glGetVaryingLocationNV(conflictProgram->programId(), "vPosition0")
            };
    glTransformFeedbackVaryingsNV(conflictProgram->programId(), 2, conflictUpdate, GL_INTERLEAVED_ATTRIBS);
    conflictProgram->Unbind();

    backMappingProgram->Bind();
    int locUpdate[3] =
    {
        glGetVaryingLocationNV(backMappingProgram->programId(), "vPosition0"),
        glGetVaryingLocationNV(backMappingProgram->programId(), "vColorTime0"),
        glGetVaryingLocationNV(backMappingProgram->programId(), "vNormRad0"),
    };
    glTransformFeedbackVaryingsNV(backMappingProgram->programId(), 3, locUpdate, GL_INTERLEAVED_ATTRIBS);
    backMappingProgram->Unbind();

    unstableProgram->Bind();
    int unstableUpdate[3] =
    {
        glGetVaryingLocationNV(unstableProgram->programId(), "vPosition0"),
        glGetVaryingLocationNV(unstableProgram->programId(), "vColorTime0"),
        glGetVaryingLocationNV(unstableProgram->programId(), "vNormRad0"),
    };
    glTransformFeedbackVaryingsNV(unstableProgram->programId(), 3, unstableUpdate, GL_INTERLEAVED_ATTRIBS);
    unstableProgram->Unbind();


    glGenQueries(1, &countQuery);

    CheckGlDieOnError();
}

GlobalModel::~GlobalModel()
{
    glDeleteTransformFeedbacks(1, &modelFid);
    glDeleteBuffers(1, &modelVbo);

    glDeleteTransformFeedbacks(1, &dataFid);
    glDeleteBuffers(1, &dataVbo);

    glDeleteQueries(1, &countQuery);

    glDeleteBuffers(1, &uvo);
}

void GlobalModel::initialize(const FeedbackBuffer & rawFeedback, const Eigen::Matrix4f &pose)
{
    // just simply copy
    initProgram->Bind();

    initProgram->setUniform(Uniform("pose", pose));

    glBindBuffer(GL_ARRAY_BUFFER, rawFeedback.vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, modelFid);

    //glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, modelVbo);
    glTransformFeedbackBufferBase(modelFid, 0, modelVbo);

    glBeginTransformFeedback(GL_POINTS);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);

    //It's ok to use either fid because both raw and filtered have the same amount of vertices
    glDrawTransformFeedback(GL_POINTS, rawFeedback.fid);

    glEndTransformFeedback();

    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);

    glGetQueryObjectuiv(countQuery, GL_QUERY_RESULT, &count);

    glDisable(GL_RASTERIZER_DISCARD);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    initProgram->Unbind();

    glFinish();


    CheckGlDieOnError();
}

void GlobalModel::dataAssociate(const Eigen::Matrix4f &pose,
                                const int &time,
                                GPUTexture *rgb,
                                GPUTexture *depthRaw,
                                GPUTexture * semantic,
                                GPUTexture *indexMap,
                                GPUTexture *vertConfMap,
                                GPUTexture *colorTimeMap,
                                GPUTexture *normRadMap,
                                const float depthMin,
                                const float depthMax)
{
    TICK("Data::Association");

    //This part computes new vertices the vertices to merge with, storing
    //in an array that sets which vertices to update by index
    dataProgram->Bind();

    dataProgram->setUniform(Uniform("cSampler", 0));
    dataProgram->setUniform(Uniform("drSampler", 1));
    dataProgram->setUniform(Uniform("sSampler", 2));
    dataProgram->setUniform(Uniform("indexSampler", 3));
    dataProgram->setUniform(Uniform("vertConfSampler", 4));
    dataProgram->setUniform(Uniform("colorTimeSampler", 5));
    dataProgram->setUniform(Uniform("normRadSampler", 6));
    dataProgram->setUniform(Uniform("time", (float)time));

    dataProgram->setUniform(Uniform("cam", Eigen::Vector4f(Config::cx(),
                                                           Config::cy(),
                                                           1.0 / Config::fx(),
                                                           1.0 / Config::fy())));
    dataProgram->setUniform(Uniform("cols", (float)Config::W()));
    dataProgram->setUniform(Uniform("rows", (float)Config::H()));
    dataProgram->setUniform(Uniform("scale", IndexMap::FACTOR));
    dataProgram->setUniform(Uniform("texDim", (float)TEXTURE_DIMENSION));
    dataProgram->setUniform(Uniform("pose", pose));
    dataProgram->setUniform(Uniform("minDepth", depthMin));
    dataProgram->setUniform(Uniform("maxDepth", depthMax));
    dataProgram->setUniform(Uniform("fuseThresh", Config::surfelFuseDistanceThresh()));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, uvo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, dataFid);
    //glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dataVbo);
    glTransformFeedbackBufferBase(dataFid, 0, dataVbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rgb->texture->tid);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthRaw->texture->tid);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, semantic->texture->tid);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, indexMap->texture->tid);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, vertConfMap->texture->tid);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, colorTimeMap->texture->tid);

    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, normRadMap->texture->tid);

    glBeginTransformFeedback(GL_POINTS);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);
    dataCount = 0;

    glDrawArrays(GL_POINTS, 0, uvSize);

    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glGetQueryObjectuiv(countQuery, GL_QUERY_RESULT, &dataCount);

    glEndTransformFeedback();

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    glDisable(GL_RASTERIZER_DISCARD);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    dataProgram->Unbind();

    glFinish();

    TOCK("Data::Association");

    CheckGlDieOnError()
}

void GlobalModel::updateFuse()
{
    TICK("Update::Fuse");

    // Change model map - I
    modelMapFramebuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, modelMapRenderBuffer.width, modelMapRenderBuffer.height);

    glDisable(GL_DEPTH_TEST);

    fuseProgram->Bind();
    fuseProgram->setUniform(Uniform("texDim", TEXTURE_DIMENSION));

    glBindBuffer(GL_ARRAY_BUFFER, dataVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    //glDrawTransformFeedback(GL_POINTS, dataFid);
    glDrawArrays(GL_POINTS, 0, dataCount);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    fuseProgram->Unbind();

    glEnable(GL_DEPTH_TEST);

    glPopAttrib();

    modelMapFramebuffer.Unbind();

    glFinish();

    TOCK("Update::Fuse");

    CheckGlDieOnError();
}

void GlobalModel::processConflict(const Eigen::Matrix4f &pose, const int &time, GPUTexture *depthRaw, GPUTexture * semantic)
{
    conflictProgram->Bind();

    conflictProgram->setUniform(Uniform("drSampler", 0));
    conflictProgram->setUniform(Uniform("seSampler", 1));

    conflictProgram->setUniform(Uniform("cam", Eigen::Vector4f(Config::cx(),
                                                               Config::cy(),
                                                               Config::fx(),
                                                               Config::fy())));
    conflictProgram->setUniform(Uniform("cols", (float)Config::W()));
    conflictProgram->setUniform(Uniform("rows", (float)Config::H()));

    Eigen::Matrix4f t_inv = pose.inverse();  // T_w^c
    conflictProgram->setUniform(Uniform("t_inv", t_inv));
    conflictProgram->setUniform(Uniform("fuseThresh", Config::surfelFuseDistanceThresh()));

    glBindBuffer(GL_ARRAY_BUFFER, modelVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 1));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, conflictFid);

    //glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, conflictVbo);
    glTransformFeedbackBufferBase(conflictFid, 0, conflictVbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthRaw->texture->tid);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, semantic->texture->tid);

    glBeginTransformFeedback(GL_POINTS);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);
    conflictCount = 0;

    glDrawArrays(GL_POINTS, 0, count);

    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glGetQueryObjectuiv(countQuery, GL_QUERY_RESULT, &conflictCount);

    glEndTransformFeedback();

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    glDisable(GL_RASTERIZER_DISCARD);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    conflictProgram->Unbind();

    glFinish();

    CheckGlDieOnError();

}

void GlobalModel::updateConflict()
{
    vertConfFrameBuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, vertConfRenderBuffer.width, vertConfRenderBuffer.height);

    glDisable(GL_DEPTH_TEST);

    updateConflictProgram->Bind();
    updateConflictProgram->setUniform(Uniform("texDim", TEXTURE_DIMENSION));

    glBindBuffer(GL_ARRAY_BUFFER, conflictVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(float) + (int)(Config::vertexSize() / 3), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(float) + (int)(Config::vertexSize() / 3), reinterpret_cast<GLvoid*>(sizeof(float)));

    glDrawArrays(GL_POINTS, 0, conflictCount);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    updateConflictProgram->Unbind();

    glEnable(GL_DEPTH_TEST);

    glPopAttrib();

    vertConfFrameBuffer.Unbind();

    glFinish();

    CheckGlDieOnError()
}

void GlobalModel::backMapping()
{
    TICK("Back::Clean");

    // First we copy modelMap* texturefs to modelVbo
    backMappingProgram->Bind();
    backMappingProgram->setUniform(Uniform("vertConfSampler", 0));
    backMappingProgram->setUniform(Uniform("colorTimeSampler", 1));
    backMappingProgram->setUniform(Uniform("normRadSampler", 2));

    glEnable(GL_RASTERIZER_DISCARD);

    // set receiver
    //glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, modelVbo);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, modelFid);
    glTransformFeedbackBufferBase(modelFid, 0, modelVbo);

    glBeginTransformFeedback(GL_POINTS);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);
    offset = 0;

    glBindBuffer(GL_ARRAY_BUFFER, bigUvo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, modelMapVertsConfs.texture->tid);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, modelMapColorsTime.texture->tid);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, modelMapNormsRadii.texture->tid);

    glDrawArrays(GL_POINTS, 0, count);   // only partial pixels are valid

    glEndTransformFeedback();

    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glGetQueryObjectuiv(countQuery, GL_QUERY_RESULT, &offset);

    glDisable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    backMappingProgram->Unbind();

    glFinish();

    TOCK("Back::Clean");

    count = offset;


    CheckGlDieOnError();
}

void GlobalModel::concatenate()
{
    TICK("Concatenate");

    unstableProgram->Bind();

    glEnable(GL_RASTERIZER_DISCARD);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, unstableVbo);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);
    unstableCount = 0;

    glBeginTransformFeedback(GL_POINTS);

    glBindBuffer(GL_ARRAY_BUFFER, dataVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    glDrawArrays(GL_POINTS, 0, dataCount);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glEndTransformFeedback();

    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glGetQueryObjectuiv(countQuery, GL_QUERY_RESULT, &unstableCount);

    glDisable(GL_RASTERIZER_DISCARD);

    unstableProgram->Unbind();

    glFinish();

    //-----------------------------------------------------

    glBindBuffer(GL_COPY_READ_BUFFER, unstableVbo);
    glBindBuffer(GL_COPY_WRITE_BUFFER, modelVbo);

    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, offset * Config::vertexSize(), unstableCount * Config::vertexSize());

    count = offset + unstableCount;

    glFinish();

    TOCK("Concatenate");


    CheckGlDieOnError();
}

void GlobalModel::buildModelMap()
{
    modelMapFramebuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, modelMapRenderBuffer.width, modelMapRenderBuffer.height);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    modelProgram->Bind();

    modelProgram->setUniform(Uniform("texDim", TEXTURE_DIMENSION));

    glBindBuffer(GL_ARRAY_BUFFER, modelVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 1));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    //glDrawTransformFeedback(GL_POINTS, modelFid);
    glDrawArrays(GL_POINTS, 0, count);

    modelMapFramebuffer.Unbind();

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // build model map each time modelVbo is updated
    modelProgram->Unbind();

    glPopAttrib();

    glFinish();


    CheckGlDieOnError();
}

void GlobalModel::renderModel(pangolin::OpenGlMatrix mvp,
                              pangolin::OpenGlMatrix mv,
                              const float threshold,
                              const bool drawUnstable,
                              const bool drawNormals,
                              const bool drawColors,
                              const bool drawPoints,
                              const bool drawWindow,
                              const bool drawSemantic,
                              const int time,
                              const int timeDelta)
{
    std::shared_ptr<Shader> program = drawPoints ? drawPointProgram : drawSurfelProgram;

    program->Bind();

    program->setUniform(Uniform("MVP", mvp));

    program->setUniform(Uniform("MVINV", mv.Inverse()));

    program->setUniform(Uniform("threshold", threshold));

    program->setUniform(Uniform("colorType", (drawNormals ? 1 : drawColors ? 2 : drawSemantic ? 3 : 0)));

    program->setUniform(Uniform("unstable", drawUnstable));

    program->setUniform(Uniform("drawWindow", drawWindow));

    program->setUniform(Uniform("time", time));

    program->setUniform(Uniform("timeDelta", timeDelta));

    Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();
    program->setUniform(Uniform("pose", pose));

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

    glBindBuffer(GL_ARRAY_BUFFER, modelVbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 1));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    //glDrawTransformFeedback(GL_POINTS, modelFid);
    glDrawArrays(GL_POINTS, 0, count);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    program->Unbind();
}

void GlobalModel::resetBuffer()
{
    clearBuffer(modelVbo, 0.f);
    count = 0;
    clearBuffer(dataVbo, 0.f);
    dataCount = 0;
    clearBuffer(conflictVbo, 0.f);
    conflictCount = 0;
    clearBuffer(unstableVbo, 0.f);
    unstableCount = 0;
}

void GlobalModel::setImageSize(int w, int h, float fx, float fy, float cx, float cy)
{
    imageRenderBuffer.Reinitialise(w, h);
    imageTexture.Reinitialise(w, h, GL_RGB8UI, true, 0, GL_RGB_INTEGER, GL_UNSIGNED_BYTE);
    semanticTexture.Reinitialise(w, h, GL_R8UI, true, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE);
    depthTexture.Reinitialise(w, h, GL_R16UI, true, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT);

    imageCam << cx, cy, fx, fy;

    assert(texturePtr == nullptr && "Please call endRenderImage() after finish renderImage()!");

    texturePtr = new unsigned char [w * h * 3];
}

void GlobalModel::renderImage(const Eigen::Matrix4f &view, std::string file)
{
    pangolin::GlFramebuffer imageFramebuffer;
    imageFramebuffer.AttachColour(imageTexture);
    //imageFramebuffer.AttachColour(semanticTexture);
    //imageFramebuffer.AttachColour(depthTexture);
    imageFramebuffer.AttachDepth(imageRenderBuffer);

    imageFramebuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, imageRenderBuffer.width, imageRenderBuffer.height);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    drawImageProgram->Bind();

    Eigen::Matrix4f t_inv = view.inverse();
    drawImageProgram->setUniform(Uniform("t_inv", t_inv));
    drawImageProgram->setUniform(Uniform("cam", imageCam));
    drawImageProgram->setUniform(Uniform("cols", (float)imageRenderBuffer.width));
    drawImageProgram->setUniform(Uniform("rows", (float)imageRenderBuffer.height));
    drawImageProgram->setUniform(Uniform("maxDepth", 200.f));
    drawImageProgram->setUniform(Uniform("threshold", 0.f));

    glBindBuffer(GL_ARRAY_BUFFER, modelVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 1));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    glDrawArrays(GL_POINTS, 0, count);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    drawImageProgram->Unbind();

    glPopAttrib();

    imageFramebuffer.Unbind();

    glFinish();

    CheckGlDieOnError()

    //---------------------------------------------


    imageTexture.Download(texturePtr, GL_RGB_INTEGER, GL_UNSIGNED_BYTE);

    CheckGlDieOnError()

    cv::Mat image(imageRenderBuffer.height, imageRenderBuffer.width, CV_8UC3);
    memcpy(image.data, texturePtr, imageRenderBuffer.width * imageRenderBuffer.height * 3);

    cv::imwrite(file, image);

    usleep(100000);
}

void GlobalModel::endRenderImage()
{
    delete [] texturePtr;
    texturePtr = nullptr;
}

pangolin::GlTexture * GlobalModel::getModelMapVC()
{
    return modelMapVertsConfs.texture;
}

pangolin::GlTexture * GlobalModel::getModelMapCT()
{
    return modelMapColorsTime.texture;
}

pangolin::GlTexture * GlobalModel::getModelMapNR()
{
    return modelMapNormsRadii.texture;
}

std::pair<GLuint, GLuint> GlobalModel::getModel()
{
    return {modelVbo, count};
}

std::pair<GLuint, GLuint> GlobalModel::getData()
{
    return {dataVbo, dataCount};
}

std::pair<GLuint, GLuint> GlobalModel::getConflict()
{
    return {conflictVbo, conflictCount};
}

std::pair<GLuint, GLuint> GlobalModel::getUnstable()
{
    return {unstableVbo, unstableCount};
}

unsigned int GlobalModel::getOffset()
{
    return offset;
}

void GlobalModel::clearBuffer(GLuint buffer, GLfloat value)
{
    glBindBuffer(GL_ARRAY_BUFFER, buffer);

    glClearBufferData(GL_ARRAY_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &value);

    glBindBuffer(GL_ARRAY_BUFFER, 0);

    CheckGlDieOnError();
}

bool GlobalModel::downloadMap(const std::string &path)
{
    glFinish();

    Eigen::Vector4f * vertices = new Eigen::Vector4f[count * 3];

    memset(&vertices[0], 0, count * Config::vertexSize());

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, modelVbo);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, count * Config::vertexSize(), vertices);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

    glFinish();

    CheckGlDieOnError()

    Eigen::IOFormat CSVFormat(Eigen::FullPrecision, 0, ",", ",");

    std::ofstream file(path);

    //std::cout << vertices[0].format(CSVFormat) << ','
    //          << vertices[1].format(CSVFormat) << ','
    //          << vertices[2].format(CSVFormat) << std::endl;

    try
    {
        if(file.is_open())
        {
            for(int i = 0; i < count; ++i)
            {
                file << vertices[3 * i + 0].format(CSVFormat) << ","
                     << vertices[3 * i + 1].format(CSVFormat) << ","
                     << vertices[3 * i + 2].format(CSVFormat) << '\n';
            }
            file.close();

            printf("%s is saved!", path.c_str());
        }
        else
        {
            printf("%s is not open!", path.c_str());
        }
    }
    catch (std::ofstream::failure &e)
    {
        std::cerr << path << " saved err!!" << " " << e.what();
    }

}
