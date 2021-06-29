

#include "GlobalModel.h"

const int GlobalModel::TEXTURE_DIMENSION = 3072;
const int GlobalModel::MAX_VERTICES = GlobalModel::TEXTURE_DIMENSION * GlobalModel::TEXTURE_DIMENSION;
//const int GlobalModel::NODE_TEXTURE_DIMENSION = 16384;
//const int GlobalModel::MAX_NODES = GlobalModel::NODE_TEXTURE_DIMENSION / 16; //16 floats per node

GlobalModel::GlobalModel()
 : bufferSize(MAX_VERTICES * Config::vertexSize()),
   count(0),
   offset(0),
   dataCount(0),
   unstableCount(0),
   initProgram(loadProgramFromFile("init_unstable.vert")),
   modelProgram(loadProgramFromFile("map.vert", "map.frag")),
   drawPointProgram(loadProgramFromFile("draw_feedback.vert", "draw_feedback.frag")),
   drawSurfelProgram(loadProgramFromFile("draw_global_surface.vert", "draw_global_surface.geom", "draw_global_surface.frag")),
   dataProgram(loadProgramGeomFromFile("data.vert", "data.geom")),
   updateProgram(loadProgramFromFile("update.vert", "update.frag")),
   backMappingProgram(loadProgramGeomFromFile("back_map.vert", "back_map.geom")),
   unstableProgram(loadProgramGeomFromFile("unstable.vert", "unstable.geom")),
   renderBuffer(TEXTURE_DIMENSION, TEXTURE_DIMENSION),
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

    glGenBuffers(1, &unstableVbo);
    glBindBuffer(GL_ARRAY_BUFFER, unstableVbo);
    glBufferData(GL_ARRAY_BUFFER, Config::numPixels() * Config::vertexSize() / 4, nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    std::vector<Eigen::Vector2f> uv;
    uv.reserve(Config::numPixels());
    for(int i = 0; i < Config::W(); ++i)
    {
        for(int j = 0; j < Config::H(); ++j)
        {
            uv.push_back(Eigen::Vector2f(((float)i / (float)Config::W()) + 1.0 / (2 * (float)Config::W()),
                                         ((float)j / (float)Config::H()) + 1.0 / (2 * (float)Config::H())));
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

    frameBuffer.AttachColour(*modelMapVertsConfs.texture);
    frameBuffer.AttachColour(*modelMapColorsTime.texture);
    frameBuffer.AttachColour(*modelMapNormsRadii.texture);
    frameBuffer.AttachDepth(renderBuffer);

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

void GlobalModel::initialize(const FeedbackBuffer & rawFeedback)
{
    initProgram->Bind();

    glBindBuffer(GL_ARRAY_BUFFER, rawFeedback.vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), 0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f)));

    //glBindBuffer(GL_ARRAY_BUFFER, filteredFeedback.vbo);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Config::vertexSize(), reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));

    glEnable(GL_RASTERIZER_DISCARD);

//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, modelFid);

    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, modelVbo);

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
//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    initProgram->Unbind();

    glFinish();


    CheckGlDieOnError();
}

void GlobalModel::fuse(const Eigen::Matrix4f & pose,
                       const int & time,
                       GPUTexture * rgb,
                       GPUTexture * depthRaw,
                       GPUTexture * indexMap,
                       GPUTexture * vertConfMap,
                       GPUTexture * colorTimeMap,
                       GPUTexture * normRadMap,
                       const float depthCutoff)
{
    TICK("Fuse::Data");
    //This first part does data association and computes the vertex to merge with, storing
    //in an array that sets which vertices to update by index
    dataProgram->Bind();

    dataProgram->setUniform(Uniform("cSampler", 0));
    dataProgram->setUniform(Uniform("drSampler", 1));
    dataProgram->setUniform(Uniform("indexSampler", 2));
    dataProgram->setUniform(Uniform("vertConfSampler", 3));
    dataProgram->setUniform(Uniform("colorTimeSampler", 4));
    dataProgram->setUniform(Uniform("normRadSampler", 5));
    dataProgram->setUniform(Uniform("time", (float)time));

    dataProgram->setUniform(Uniform("cam", Eigen::Vector4f(Config::cx(),
                                                           Config::cy(),
                                                           1.0 / Config::fx(),
                                                           1.0 / Config::fy())));
    dataProgram->setUniform(Uniform("cols", (float)Config::W()));
    dataProgram->setUniform(Uniform("rows", (float)Config::H()));
    dataProgram->setUniform(Uniform("scale", (float)IndexMap::FACTOR));
    dataProgram->setUniform(Uniform("texDim", (float)TEXTURE_DIMENSION));
    dataProgram->setUniform(Uniform("pose", pose));
    dataProgram->setUniform(Uniform("maxDepth", depthCutoff));

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, uvo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glEnable(GL_RASTERIZER_DISCARD);

    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, dataFid);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, dataVbo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, rgb->texture->tid);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthRaw->texture->tid);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, indexMap->texture->tid);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, vertConfMap->texture->tid);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, colorTimeMap->texture->tid);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, normRadMap->texture->tid);

    glBeginTransformFeedback(GL_POINTS);

    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);

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
    TOCK("Fuse::Data");

    CheckGlDieOnError();

    //------------------------------------------------------------------

    TICK("Fuse::Update");
    //Next we update the vertices at the indexes stored in the modelMap* texturefs
    frameBuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, renderBuffer.width, renderBuffer.height);

    glDisable(GL_DEPTH_TEST);

    updateProgram->Bind();
    updateProgram->setUniform(Uniform("texDim", TEXTURE_DIMENSION));

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

    updateProgram->Unbind();

    glEnable(GL_DEPTH_TEST);

    glPopAttrib();

    frameBuffer.Unbind();

    glFinish();
    TOCK("Fuse::Update");

    CheckGlDieOnError();
}

void GlobalModel::backMapping()
{
    TICK("BackMapping");

    // First we copy modelMap* texturefs to modelVbo
    backMappingProgram->Bind();
    backMappingProgram->setUniform(Uniform("vertConfSampler", 0));
    backMappingProgram->setUniform(Uniform("colorTimeSampler", 1));
    backMappingProgram->setUniform(Uniform("normRadSampler", 2));

    glEnable(GL_RASTERIZER_DISCARD);

    // set receiver
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, modelVbo);
//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, modelFid);

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

//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDisableVertexAttribArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE0);

    backMappingProgram->Unbind();

    glFinish();

    TOCK("BackMapping");


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

    //glDrawTransformFeedback(GL_POINTS, dataFid);
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
    frameBuffer.Bind();

    glPushAttrib(GL_VIEWPORT_BIT);

    glViewport(0, 0, renderBuffer.width, renderBuffer.height);

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

    frameBuffer.Unbind();

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
                              const float threshold,
                              const bool drawUnstable,
                              const bool drawNormals,
                              const bool drawColors,
                              const bool drawPoints,
                              const bool drawWindow,
                              const bool drawTimes,
                              const int time,
                              const int timeDelta)
{
    std::shared_ptr<Shader> program = drawPoints ? drawPointProgram : drawSurfelProgram;

    program->Bind();

    program->setUniform(Uniform("MVP", mvp));

    program->setUniform(Uniform("threshold", threshold));

    program->setUniform(Uniform("colorType", (drawNormals ? 1 : drawColors ? 2 : drawTimes ? 3 : 0)));

    program->setUniform(Uniform("unstable", drawUnstable));

    program->setUniform(Uniform("drawWindow", drawWindow));

    program->setUniform(Uniform("time", time));

    program->setUniform(Uniform("timeDelta", timeDelta));

    Eigen::Matrix4f pose = Eigen::Matrix4f::Identity();
    program->setUniform(Uniform("pose", pose));

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

void GlobalModel::clean(const Eigen::Matrix4f & pose,
                        const int & time,
                        GPUTexture * indexMap,
                        GPUTexture * vertConfMap,
                        GPUTexture * colorTimeMap,
                        GPUTexture * normRadMap,
                        GPUTexture * depthMap,
                        const float confThreshold,
                        std::vector<float> & graph,
                        const int timeDelta,
                        const float maxDepth,
                        const bool isFern)
{
//    assert(graph.size() / 16 < MAX_NODES);
//
//    if(graph.size() > 0)
//    {
//        //Can be optimised by only uploading new nodes with offset
//        glBindTexture(GL_TEXTURE_2D, deformationNodes.texture->tid);
//        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, graph.size(), 1, GL_LUMINANCE, GL_FLOAT, graph.data());
//    }
//
//    TICK("Fuse::Copy");
//    //Next we copy the new unstable vertices from the newUnstableFid transform feedback into the global map
//    unstableProgram->Bind();
//    unstableProgram->setUniform(Uniform("time", time));
//    unstableProgram->setUniform(Uniform("confThreshold", confThreshold));
//    unstableProgram->setUniform(Uniform("scale", (float)IndexMap::FACTOR));
//    unstableProgram->setUniform(Uniform("indexSampler", 0));
//    unstableProgram->setUniform(Uniform("vertConfSampler", 1));
//    unstableProgram->setUniform(Uniform("colorTimeSampler", 2));
//    unstableProgram->setUniform(Uniform("normRadSampler", 3));
//    unstableProgram->setUniform(Uniform("nodeSampler", 4));
//    unstableProgram->setUniform(Uniform("depthSampler", 5));
//    unstableProgram->setUniform(Uniform("nodes", (float)(graph.size() / 16)));
//    unstableProgram->setUniform(Uniform("nodeCols", (float)NODE_TEXTURE_DIMENSION));
//    unstableProgram->setUniform(Uniform("timeDelta", timeDelta));
//    unstableProgram->setUniform(Uniform("maxDepth", maxDepth));
//    unstableProgram->setUniform(Uniform("isFern", (int)isFern));
//
//    Eigen::Matrix4f t_inv = pose.inverse();
//    unstableProgram->setUniform(Uniform("t_inv", t_inv));
//
//    unstableProgram->setUniform(Uniform("cam", Eigen::Vector4f(Intrinsics::getInstance().cx(),
//                                                         Intrinsics::getInstance().cy(),
//                                                         Intrinsics::getInstance().fx(),
//                                                         Intrinsics::getInstance().fy())));
//    unstableProgram->setUniform(Uniform("cols", (float)Resolution::getInstance().cols()));
//    unstableProgram->setUniform(Uniform("rows", (float)Resolution::getInstance().rows()));
//
//    glBindBuffer(GL_ARRAY_BUFFER, vbos[target].first);
//
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Vertex::SIZE, 0);
//
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Vertex::SIZE, reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f)));
//
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Vertex::SIZE, reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));
//
//    glEnable(GL_RASTERIZER_DISCARD);
//
//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, vbos[renderSource].second);
//
//    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbos[renderSource].first);
//
//    glBeginTransformFeedback(GL_POINTS);
//
//    glActiveTexture(GL_TEXTURE0);
//    glBindTexture(GL_TEXTURE_2D, indexMap->texture->tid);
//
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, vertConfMap->texture->tid);
//
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_2D, colorTimeMap->texture->tid);
//
//    glActiveTexture(GL_TEXTURE3);
//    glBindTexture(GL_TEXTURE_2D, normRadMap->texture->tid);
//
//    glActiveTexture(GL_TEXTURE4);
//    glBindTexture(GL_TEXTURE_2D, deformationNodes.texture->tid);
//
//    glActiveTexture(GL_TEXTURE5);
//    glBindTexture(GL_TEXTURE_2D, depthMap->texture->tid);
//
//    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, countQuery);
//
//    glDrawTransformFeedback(GL_POINTS, vbos[target].second);
//
//    glBindBuffer(GL_ARRAY_BUFFER, newUnstableVbo);
//
//    glEnableVertexAttribArray(0);
//    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, Vertex::SIZE, 0);
//
//    glEnableVertexAttribArray(1);
//    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, Vertex::SIZE, reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f)));
//
//    glEnableVertexAttribArray(2);
//    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, Vertex::SIZE, reinterpret_cast<GLvoid*>(sizeof(Eigen::Vector4f) * 2));
//
//    glDrawTransformFeedback(GL_POINTS, newUnstableFid);
//
//    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
//
//    glGetQueryObjectuiv(countQuery, GL_QUERY_RESULT, &count);
//
//    glEndTransformFeedback();
//
//    glDisable(GL_RASTERIZER_DISCARD);
//
//    glBindTexture(GL_TEXTURE_2D, 0);
//    glActiveTexture(GL_TEXTURE0);
//
//    glDisableVertexAttribArray(0);
//    glDisableVertexAttribArray(1);
//    glDisableVertexAttribArray(2);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
//
//    unstableProgram->Unbind();
//
//    std::swap(target, renderSource);
//
//    glFinish();
//    TOCK("Fuse::Copy");
}

unsigned int GlobalModel::getCount()
{
    return count;
}

unsigned int GlobalModel::getOffset()
{
    return offset;
}

unsigned int GlobalModel::getDataCount()
{
    return dataCount;
}

unsigned int GlobalModel::getUnstableCount()
{
    return unstableCount;
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

std::pair<GLuint, GLuint> GlobalModel::getUnstable()
{
    return {unstableVbo, unstableCount};
}

Eigen::Vector4f * GlobalModel::downloadMap()
{
//    glFinish();
//
//    Eigen::Vector4f * vertices = new Eigen::Vector4f[count * 3];
//
//    memset(&vertices[0], 0, count * Vertex::SIZE);
//
//    GLuint downloadVbo;
//
//    glGenBuffers(1, &downloadVbo);
//    glBindBuffer(GL_ARRAY_BUFFER, downloadVbo);
//    glBufferData(GL_ARRAY_BUFFER, bufferSize, 0, GL_STREAM_DRAW);
//    glBindBuffer(GL_ARRAY_BUFFER, 0);
//
//    glBindBuffer(GL_COPY_READ_BUFFER, vbos[renderSource].first);
//    glBindBuffer(GL_COPY_WRITE_BUFFER, downloadVbo);
//
//    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, count * Vertex::SIZE);
//    glGetBufferSubData(GL_COPY_WRITE_BUFFER, 0, count * Vertex::SIZE, vertices);
//
//    glBindBuffer(GL_COPY_READ_BUFFER, 0);
//    glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
//    glDeleteBuffers(1, &downloadVbo);
//
//    glFinish();
//
//    return vertices;
}
