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

#ifndef COMPUTEPACK_H_
#define COMPUTEPACK_H_

#include "Shaders.h"
#include "Uniform.h"
#include <pangolin/gl/gl.h>

class ComputePack
{
    public:
        ComputePack(std::shared_ptr<Shader> program);

        virtual ~ComputePack();

        // type of compute packs
        static const std::string FILTER, METRIC, SMOOTH;

        void compute(pangolin::GlTexture * target, pangolin::GlTexture * input, const std::vector<Uniform> * uniforms = 0);

        void compute(pangolin::GlTexture * target, const std::vector<pangolin::GlTexture *> & inputs, const std::vector<Uniform> * uniforms = 0);

    private:
        std::shared_ptr<Shader> program;
};

#endif /* COMPUTEPACK_H_ */
