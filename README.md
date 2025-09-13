## Software Kaleidoscope using Pixel Game Engine.

There are 3 types of software kaleidoscope in this repo:
- Camera Kaleidoscope: [camerakaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/camerakaleidoscope.cpp)  
- Image Kaleidoscope [imagekaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/imagekaleidoscope.cpp)  
- Shapes Kaleidoscope [shapeskaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/shapeskaleidoscope.cpp)

Each can be built individually into standalone binaries, or you can build the [multikaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/multikaleidoscope.cpp) which contains each kaleidoscope type with an interactive switch to change between kaleidoscopes during runtime.

At some point I would like to create an emscripten build for some or all of these kaleidoscopes.

### Notes 
1. The [camerakaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/camerakaleidoscope.cpp) and [multikaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/multikaleidoscope.cpp) require a working webcam or usb cam to work. In addition they currently only work on linux and require the opencv webcam api to build and run (see dependencies for install command).
2. The PGE and PGEX header files are included under /usr/include/pge/ on my system. They are required as buld dependencies and you may need to clone them from github (see dependencies for link) and adjust the include paths in the kaleidoscope source files relative to where you clone them to.
3. The [shapeskaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/shapeskaleidoscope.cpp) and [multikaleidoscope.cpp](https://github.com/u2084511felix/pge_kaleidoscope/blob/main/multikaleidoscope.cpp) use my own forked version of the olcPixelGameEngine.h which include changes from this [Pull Request](https://github.com/OneLoneCoder/olcPixelGameEngine/pull/414/commits/441f901f5ded549612de7e1aac9efa9959e1e18d)

## References

- ProcGen / Randomizer utils: [OneLoneCoder_PGE_ProcGen_Universe.cpp](https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_ProcGen_Universe.cpp)

- Dithered and quantization techniques: [OneLoneCoder_PGE_Dithering.cpp](https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_Dithering.cpp)

- Hi-res Space telescope images: [science.nasa.gov](https://science.nasa.gov/mission/webb)

## Kaleidoscope Research Material 
---
https://discoverycentermd.org/news/kaleidoscope/
https://optica.machorro.net/Optica/SciAm/Kaleidoscope/1985-12-body.html
https://en.etudes.ru/etudes/kaleidoscope/
https://en.m.wikipedia.org/wiki/File:Hyperbolic_kaleidoscopes.png
https://en.wikipedia.org/wiki/Triangle_group
https://en.wikipedia.org/wiki/Triangular_tiling
https://en.wikipedia.org/wiki/Uniform_tilings_in_hyperbolic_plane#(3_2_2_2)
http://mathengaged.org/resources/activities/art-projects/tessellations/
https://brewstersociety.com/kaleidoscope-university/types-of-scopes/
https://www.worthpoint.com/worthopedia/wildewood-creative-products-unique-1819630333


---

Thanks to MaGetzUb (from the onelonecoder discord community) for the SaveSprite screenshot function.
Also thanks to Talon (also from the onelonecoder discord community) for a lot of help with the kaleidoscope texture mapping, PGE decal draw procedures, and some missing C++ keywords!

## Build commands

```bash
##Multi Kaleidoscope
g++ -o multikaleidoscope multikaleidoscope.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -lopencv_core -lopencv_videoio -lopencv_imgproc -std=c++20
```

```bash
##Camera Kaleidoscope
g++ -o camkaleidoscope camerakaleidoscope.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -lopencv_core -lopencv_videoio -lopencv_imgproc -std=c++20
```

```bash
##Shapes Kaleidoscope
g++ -o shapeskaleidoscope shapeskaleidoscope.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++20
```

```bash
##Image Kaleidoscope
g++ -o imagekaleidoscope imagekaleidoscope.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -std=c++20
```

## Dependencies:

- [Official olcPixelGameEngine.h](https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/olcPixelGameEngine.h): Works for all but the shapes and mutli kaleidoscope builds.
- [Custom fork of olcPixelGameEngine.h with new draw routines](https://github.com/u2084511felix/olcPixelGameEngine/blob/drawquadrect/olcPixelGameEngine.h): Required for the shapes and multikaleidoscope builds.

- [Official olcPGEX_TransformedView.h](https://github.com/OneLoneCoder/olcPixelGameEngine/blob/master/extensions/olcPGEX_TransformedView.h) Required for the image and multi kaleidoscope builds.


```bash  
## Required for the camera and multi kaleidoscope builds:
sudo apt-get install libopencv-dev
```

## Example images
[See more here](https://github.com/u2084511felix/pge_kaleidoscope/tree/main/Screenshots)

Coloured Shapes Kaleidoscope:
<img width="1019" height="962" alt="Screenshot from 2025-09-03 20-39-59" src="https://github.com/user-attachments/assets/81ccf7c3-e1c3-4d90-ba1a-7ddaf3996186" />
Space telescope images kaleidoscope:
<img width="1019" height="962" alt="pge_kaleidoscope_Fri_Sep_12_18_21_23_2025" src="https://github.com/u2084511felix/pge_kaleidoscope/blob/main/Screenshots/pge_kaleidoscope_Fri_Sep_12_18_21_23_2025_.png" />
Webcam image kaleidoscope:
<img width="1019" height="962" alt="pge_kaleidoscope_Fri_Sep_12_18_23_14_2025_" src="https://github.com/u2084511felix/pge_kaleidoscope/blob/main/Screenshots/pge_kaleidoscope_Fri_Sep_12_18_23_14_2025_.png" />
