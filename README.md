![Sample](https://files.slindev.com/images/rayne_github.jpg)

# Rayne
## About
Rayne is a game engine for Windows, macOS, Linux and Android (and iOS shouldn't be missing much) written in C++. It currently has rendering implementations for D3D12, Vulkan and Metal.

Most functionality (like physics, path finding and UI) is added with optional modules. Not all included modules are fully working, some may not build at all. Modules can be found in the *Modules* directory.
Some of the working modules are:
* D3D12 (Not maintained anymore, will eventually be removed. Use Vulkan instead.)
* Vulkan
* Metal
* UI
* OpenAL
* Ogg
* PhysX
* OpenXR
* Oculus (Requires the Oculus SDK to be downloaded and copied into the module folder!) - Only works with D3D12! - Outdated, use OpenXR instead.
* Oculus Mobile (Requires the Oculus Mobile SDK to be downloaded and copied into the module folder!) - Only works with Vulkan! - Outdated, use OpenXR instead.
* OpenVR - Outdated, use OpenXR instead.
* ENet
* Recast
* Assimp

There are a couple of useful python scripts in the *Tools* directory.


## Getting Started

### Preparations
#### Windows
1. Install Visual Studio (2017 or newer)
2. Install [CMake](https://cmake.org) and have it added to the PATH environment variable
3. Install [Python 3.x](https://www.python.org) and have them added to the PATH environment variable
4. Run ```git submodule update --init``` from within the *Rayne* directory
5. Go to *Rayne/Tools/ShaderProcessor/Vendor/ShaderConductor* and run ```py -3 BuildAll.py```
6. Contiune with *Creating a new Project*

There are currently some issues with the Windows setup requiring the cmake path to SetupAPI (part of the windows sdk) to be set manually.

#### macOS
1. Install a recent Version of Xcode (11 and newer should be fine, older hasn't been tested in a while).
2. Install Xcode command line tools: ```xcode-select --install```
3. Install Homebrew: ```/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"```
4. Install Python 3.x: ```brew install python```
5. Install Ninja: ```brew install ninja```
5.5 (Install patchelf (only needed for android builds with OpenXR!): ```brew install patchelf```)
6. Run ```git submodule update --init``` from within the *Rayne* directory
7. Go to *Rayne/Tools/ShaderProcessor/Vendor/ShaderConductor* and run ```python3 BuildAll.py```
8. Contiune with *Creating a new Project*

#### Linux
Depending on your linux distribution you may or may not want to use a different packet manager and the packets may be called differently, other packets could be missing or require an additional repository to install them from. The following has only been tested on Ubuntu 20.04 LTS.
1. Install cmake: ```sudo apt install cmake```
2. Install ninja: ```sudo apt install ninja-build```
4. Install python 3.x: ```sudo apt install python3```
5. Install python 3 distutils, will already have been included in some python 3 releases: ```sudo apt install python3-distutils```
6. Install X11 SDK: ```sudo apt install xorg-dev```
7. Install libxfixes, which extends and improves the X11 SDK: ```sudo apt install libxfixes-dev```
8. Install Vulkan SDK: ```sudo apt install libvulkan-dev```
9. Install GCC: ```sudo apt install gcc```
10. Install G++: ```sudo apt install g++```
11. Run ```git submodule update --init``` from within the *Rayne* directory
12. Go to *Rayne/Tools/ShaderProcessor/Vendor/ShaderConductor* and run ```python3 BuildAll.py```
13. Contiune with *Creating a new Project*

### Creating a new Project
Create a folder structure like the following (*Rayne* should be the directory containing this readme and everything else from this repo)
- root
  - Rayne
  - Games
    - GameName

1. Open a terminal and change the current directory to GameName.
2. Run ```python3 ../../Rayne/Tools/ProjectGenerator/CreateProject.py```
*Template Name* has to be *base-vr* at the moment (it also works without VR, but handles some additional setup needed for VR), everything else can be chosen however you want.
3. Run ```python3 ../../Rayne/Tools/BuildHelper/CreateBuildProject.py build-config.json platform independent``` replace *platform* with either *windows* (only works on windows), *macos* (only works on macOS), *linux* (only works on linux) or *android* (should work on any host system, only really tested on macOS though).
This will create a new *Builds* directory with another directory for the platform inside. For windows this will contain a Visual Studio solution, for macOS it will be an XCode workspace, on linux it will be ninja makefiles and for android it will be a gradle project that can be imported into Android Studio.
5. Then build and run and everything should work.


## Screenshots
![Project Z](https://files.slindev.com/images/rayne_github_projectz.jpg)
![Concealed](https://files.slindev.com/images/rayne_github_concealed.jpg)
![Ski Jump](https://files.slindev.com/images/rayne_github_skijump.jpg)
![Blobby Tennis](https://files.slindev.com/images/rayne_github_blobbytennis_2.jpg)
![Vehicle Construction Kit](https://files.slindev.com/images/rayne_github_vck_2.jpg)
