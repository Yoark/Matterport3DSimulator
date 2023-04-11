# Problem of current installing method
1. If you use docker image, it will work, and you will be able to modify/rebuild the image as needed based on a previous image.
But unfortunately, docker requires root access, and user normally don't have root access on a shared cluster, an docker-alternative is 
available in this case: Singularity, but it does not support modify/build image based on another image


# Installation notes
```sudo apt install bison # a requirement for some vcpkg install``` 

**Looks like vcpkg path is not applicable as there are too many dependencies that are likely require a sudo install in os rather than within vcpkg**
## install these
libdbus-1-dev libxi-dev libxtst-dev
libx11-dev libxft-dev libxext-dev libxrandr-dev
./vcpkg integrate install
 CMake projects should use: "-DCMAKE_TOOLCHAIN_FILE=/home/zijiao/research/Matterport3DSimulator/vcpkg/scripts/buildsystems/vcpkg.cmake"
./vcpkg install glm opencv jsoncpp

## create conda env
conda create -n mp3d python=3.9
conda install -c conda-forge pybind11
## install python packages
```
conda install pytorch torchvision torchaudio pytorch-cuda=11.8 -c pytorch -c nvidia
pip install opencv-python numpy pandas networkx
```
## HPC info
hpc does not have
1. libglm
2. libosmesa6
3. libosmesa6-dev
4. libglew
# RENDERING
this build does not support cpu off-screen rendering `-DOSMESA_RENDERING=ON`, but support GPU rendering (default) and off-screen rendering with GPU
`-DEGL_RENDERING=ON`