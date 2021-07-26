//
// Created by zhijun on 2021/5/22.
//

#ifndef SURFELMAPPING_CONFIG_H
#define SURFELMAPPING_CONFIG_H

#include <Eigen/Core>
#include <string>


#define STR(x) #x
#define define_value(x) STR(x)

class Config
{
public:
    static Config & getInstance(float fx = 0, float fy = 0, float cx = 0, float cy = 0, int rows = 0, int cols = 0);

    static float &fx() { return getInstance().fx_; }
    static float &fy() { return getInstance().fy_; }
    static float &cx() { return getInstance().cx_; }
    static float &cy() { return getInstance().cy_; }
    static int &H() { return getInstance().rows_; }
    static int &W() { return getInstance().cols_; }
    static int &numPixels() { return getInstance().num_pixels; }
    static int &vertexSize() { return getInstance().vertex_size; }
    static float &nearClip() { return getInstance().near_clip; }
    static float &farClip() { return getInstance().far_clip; }
    static float &surfelFuseDistanceThresh() { return getInstance().surfel_fuse_distance_threshold; }

    static int &maxSqrtVertices() { return getInstance().max_sqrt_vertices; }

    static std::string shaderDir()
    {
#ifdef SHADER_DIR
        return define_value(SHADER_DIR);
#else
        return "";
#endif
    }

private:
    // list of configurations
    float fx_, fy_, cx_, cy_;                        /// camera intrinsics
    int rows_, cols_;                                /// image dimensions
    int num_pixels;
    int vertex_size;
    float near_clip;
    float far_clip;
    float surfel_fuse_distance_threshold;

    int max_sqrt_vertices;

    // --------------
    Config(float fx, float fy, float cx, float cy, int rows, int cols);

};

#endif //SURFELMAPPING_CONFIG_H
