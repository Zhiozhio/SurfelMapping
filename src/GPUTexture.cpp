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
 
#include "GPUTexture.h"

const std::string GPUTexture::RGB = "RGB";
const std::string GPUTexture::DEPTH_RAW = "DEPTH";
const std::string GPUTexture::DEPTH_FILTERED = "DEPTH_FILTERED";
const std::string GPUTexture::DEPTH_METRIC = "DEPTH_METRIC";
const std::string GPUTexture::SEMANTIC = "SEMANTIC";

GPUTexture::GPUTexture(const int width,
                       const int height,
                       const GLenum internalFormat,
                       const GLenum format,
                       const GLenum dataType,
                       const bool sampleLinear,
                       const bool cuda)
 : texture(new pangolin::GlTexture(width, height, internalFormat, sampleLinear, 0, format, dataType)),
   sample_linear(sampleLinear),
   width_(width),
   height_(height),
   internal_format(internalFormat),
   format_(format),
   data_type(dataType)
{
//    if(cuda)
//    {
//        cudaGraphicsGLRegisterImage(&cudaRes, texture->tid, GL_TEXTURE_2D, cudaGraphicsRegisterFlagsReadOnly);
//    }
//    else
//    {
//        cudaRes = 0;
//    }
}

GPUTexture::~GPUTexture()
{
    if(texture)
    {
        delete texture;
    }

//    if(cudaRes)
//    {
//        cudaGraphicsUnregisterResource(cudaRes);
//    }
}
