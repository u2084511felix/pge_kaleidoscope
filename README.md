Software Kaleidoscope using Pixel Game Engine.

Note: This requires a webcam to work.

References:

ProcGen / Randomizer utils: from: https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_ProcGen_Universe.cpp
Dithered and quantization techniques from: https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_Dithering.cpp 
Hi-res Space telescope images from https://science.nasa.gov/mission/webb/

Linux Build command:
NOTE: This will not work in WSL since last I checked WSL can't access device peripherals (e.g. webcam).

```bash
g++ -o kaleidoscope pgekaleidoscope.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -lopencv_core -lopencv_videoio -lopencv_imgproc -std=c++20
```

Example image:
<img width="1019" height="962" alt="Screenshot from 2025-09-03 20-39-59" src="https://github.com/user-attachments/assets/81ccf7c3-e1c3-4d90-ba1a-7ddaf3996186" />
