//
// Created by zhijun on 2021/5/22.
//

#include "Config.h"

Config::Config(float fx, float fy, float cx, float cy, int rows, int cols)
: fx_(fx),
  fy_(fy),
  cx_(cx),
  cy_(cy),
  rows_(rows),
  cols_(cols),
  num_pixels(rows * cols)
{
    //============ Shader =====================//
    /* Single surfel model:
     *--------------------
     * vec3 position
     * float confidence
     *
     * float color + class (24-bit rgb + 8-bit class)
     * float <standby>  for feasible usage
     * float initTime
     * float timestamp
     *
     * vec3 normal
     * float radius
     *--------------------
     * Which is three vec4s
     */
    vertex_size = sizeof(Eigen::Vector4f) * 3;
    near_clip = 1.0f;                                         /// the min depth processed
    far_clip = 20.0f;                                         /// the max depth processed
    surfel_fuse_distance_threshold = 0.0;

    max_sqrt_vertices = 5000;
}

Config & Config::getInstance(float fx, float fy, float cx, float cy, int rows, int cols)
{
    static Config instance(fx, fy, cx, cy, rows, cols);
    return instance;
}
