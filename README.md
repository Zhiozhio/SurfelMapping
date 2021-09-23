# SurfelMapping
Real time traffic scene mapping using dense surfel representation.

This repo is part of the code of our LADS: Lightweight Autonomous Driving Simulator Using Surfel Mapping and GAN Enhancement. The SurfelMapping rebuilds the 3D traffic scene from images collected by stereo camera of a driving vihecle. The 3D model is represented by surfels to make the rendered images more realistic.

The code is inspired by https://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/

## 1. Dependencies
1. Ubuntu 18.04. We only tested on 18.04 but should be compatible to other adjacent distributions.
2. CMake
3. OpenGL
4. Eigen
5. [pangolin](https://github.com/stevenlovegrove/Pangolin). Build from source.
6. not too small GPU memory

## 2. Build
run commands
```
mkdir build
cd build
cmake ..
make
```

## 3. Usage
