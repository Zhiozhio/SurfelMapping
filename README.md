# SurfelMapping
Real time traffic scene mapping using dense surfel representation.

This repo is part of the code of our LADS: Lightweight Autonomous Driving Simulator Using Surfel Mapping and GAN Enhancement. The SurfelMapping rebuilds the 3D traffic scene from images collected by stereo camera of a driving vihecle. The 3D model is represented by surfels to make the rendered images more realistic.

The code is inspired by https://www.imperial.ac.uk/dyson-robotics-lab/downloads/elastic-fusion/

![image](https://github.com/Zhiozhio/SurfelMapping/blob/master/loadmap.gif)

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
The program requires RGB images and corresponding DEPTH and SEMANTIC maps as input. We use some third part learning methods prediction to provide dense depth map and semantic labels. You can download demo data from [here](https://drive.google.com/file/d/1VpIspJlR6erI3WYbwHAe1Gqn8hCgZgjE/view?usp=sharing) (using [PSMNet](https://github.com/JiaRenChang/PSMNet) for depth and [PointRend](https://github.com/facebookresearch/detectron2/tree/main/projects/PointRend) for semantic). You can use your own data including RGB, depth and semantic. The RGB, depth and semantic subdirectories should be in a same super directory and set the subdir name in the KittiReader. See [KittiReader.cpp](https://github.com/Zhiozhio/SurfelMapping/blob/master/gui/KittiReader.cpp) for details of input path. If you use the demo data, you do not need to change it.

To build the surfel map, run
```
cd build
./build_map [path to the data super dir]
```
In the GUI window, uncheck "pause" button and run the mapping. click "save" for saving the built map.

To load saved map and generate new data, run
```
cd build
./load_map [path to the data super dir] [saved map path]
```
Click "path mode" and generate novel views as shown and instructed in _loadmap.gif_. Then click "Acquire Novel Images" to get new images of those views.

## 4. SPADE
The work in _SPADE_ dir is forked from https://github.com/NVlabs/SPADE. We modified the input and some other parts. a _postprocess.py_ is also added for synthesizing final images from the GAN generated image and rendered image. Please go to original [SPADE](https://github.com/NVlabs/SPADE) to see the training and testing of the code.
