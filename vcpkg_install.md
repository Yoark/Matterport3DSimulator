# Problem of current installing method
If you use docker image provided in original repo, it will work, and you will be able to modify/rebuild the image as needed based on a previous image (locally)
But it is likely you can only rebuild/build the image locally. Because docker need root and additional dependencies you want may require root. users generally don't have root access on a shared cluster. If we look at an docker-alternative: singularity, which don't requre root-access for use, but it still requires root access to download neccessary dependencies if you want to build/rebuild image(.sif) on cluster from scratch. Both of them requires you to build image locally, and upload to the cluster to use. In summary, to build/rebuild image in order to change the simulator, or just have more control of your environment, docker-like installation is not ideal for a shared cluster. 

You only option left is to install locally then. But the root access will stills a problem. Here in this repo, we try to use vcpkg to install dependencies, but it still requires root access to install some dependencies. But these dependencies should be general enough that they are likely to be already installed on a shared cluster.

# Installation notes
```sudo apt install bison # a requirement for some vcpkg install``` 

## install these
libdbus-1-dev libxi-dev libxtst-dev
libx11-dev libxft-dev libxext-dev libxrandr-dev
> This are likely already in your cluster.

## recursively load vcpkg of this repo
./vcpkg/bootstrap-vcpkg.sh
./vcpkg integrate install
 CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/home/zijiao/research/Matterport3DSimulator/vcpkg/scripts/buildsystems/vcpkg.cmake"
> **Note**: recommend to use diff between CMakeLists.txt of this repo and the original repo to see what changes are made and how to use vcpkg cmake toolchain file and 
> as well as the vcpkg-installed packages
./vcpkg install glm opencv jsoncpp

> **Note** we use opencv4 here*, modify the code base with `opencv2to4.sh`

## create conda env
conda create -n mp3d python=3.9
conda install -c conda-forge pybind11
## install python packages
```
conda install pytorch torchvision torchaudio pytorch-cuda=11.8 -c pytorch -c nvidia
pip install opencv-python numpy pandas networkx
```
# HPC info (for OSU HPC)
Hpc does not have (we don't need 2,3,4 in this build)
1. libglm (installed use vcpkg)
2. libosmesa6
3. libosmesa6-dev
4. libglew
# RENDERING
This build does not support cpu off-screen rendering `-DOSMESA_RENDERING=ON`, but support GPU rendering (default) and off-screen rendering with GPU
`-DEGL_RENDERING=ON`
# Problem notes
1. `SIGSTKSZ` in `src/Catch.hpp` can lead to compatibility issues (os), use diff tool on `Catch.hpp` to see what I have changed
2. use `JsonCpp::JsonCpp` target directly in cmake (vcpkg install)