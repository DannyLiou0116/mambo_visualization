# Mambo_Visualization

---
## How to use

#### Dependencies

##### System dependencies
First you need to install the nvidia driver and CUDA.

- CUDA Installation guide: [Link](https://docs.nvidia.com/cuda/cuda-installation-guide-linux/index.html)

- Then you can do the other dependencies:

  ```sh
  $ sudo apt-get update 
  $ sudo apt-get install -yqq  build-essential python3-dev python3-pip apt-utils git cmake libboost-all-dev libyaml-cpp-dev libopencv-dev
  ```
  
##### Python dependencies

- Then install the Python packages needed:

  ```sh
  $ sudo apt install python-empy
  $ sudo pip install catkin_tools trollius numpy
  ```
  
##### TensorRT

In order to infer with TensorRT during inference with the C++ libraries:

- Install TensorRT: [Link](https://developer.nvidia.com/tensorrt).
- Our code and the pretrained model now only works with **TensorRT version 5** (Note that you need at least version 5.1.0).
- To make the code also works for higher versions of TensorRT, one could have a look at [here](https://github.com/PRBonn/rangenet_lib/issues/9).

#### Build the library
We use the catkin tool to build the library.

  ```sh
  $ source /opt/ros/kinetic/setup.bash
  $ mkdir -p ~/catkin_ws/src
  $ cd ~/catkin_ws/src
  $ git clone https://github.com/ros/catkin.git 
  $ git clone https://github.com/PRBonn/rangenet_lib.git
  $ git clone https://github.com/DoggyLiu0116/mambo_glow glow
  $ git clone https://github.com/DoggyLiu0116/mambo_visualization semantic_suma
  $ cd .. && catkin init
  $ catkin deps fetch
  $ catkin build --save-config -i --cmake-args -DCMAKE_BUILD_TYPE=Release -DOPENGL_VERSION=430 -DENABLE_NVIDIA_EXT=YES

  ```
  
#### Run the demo

1. 下載 darknet [model](http://www.ipb.uni-bonn.de/html/projects/semantic_suma/darknet53.tar.gz), 解壓縮後放入 rangenet_lib 的資料夾內  
(最一開始是用這個大眾 model 做過實驗, 之後再改成我們研究出較佳的 model 去產生檔案, 雖然在 mambo_visualization 內已經更正我們的程式, 但由於指向 darknet 的路徑還沒清乾淨, 所以還務必再下載 darknet 的 model 並且解壓縮後放入 rangenet_lib 的資料夾內)  
2. 輸入`cd  ~/catkin_ws/src/semantic_suma/bin`  
3. 輸入`./visualizer`  
4. 選取我們從 MamboNet 產生的 .bin 們  
