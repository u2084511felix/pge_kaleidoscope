Software Kaleidoscope using Pixel Game Engine.

Notes: 
1. This requires a webcam to work.
2. It will also therefore not work in WSL since last I checked WSL can't access device peripherals.

References:
ProcGen / Randomizer utils: from: https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_ProcGen_Universe.cpp
Dithered and quantization techniques from: https://github.com/OneLoneCoder/Javidx9/blob/master/PixelGameEngine/SmallerProjects/OneLoneCoder_PGE_Dithering.cpp 
Hi-res Space telescope images from https://science.nasa.gov/mission/webb/

Thanks to MaGetzUb (from the onelonecoder discord community) for the SaveSprite screenshot function.

Linux Build command:
NOTE: This will not work in WSL since last I checked WSL can't access device peripherals (e.g. webcam).

```bash
g++ -o kaleidoscope pgekaleidoscope.cpp -lX11 -lGL -lpthread -lpng -lstdc++fs -lopencv_core -lopencv_videoio -lopencv_imgproc -std=c++20
```

Example images:
<img width="1019" height="962" alt="Screenshot from 2025-09-03 20-39-59" src="https://github.com/user-attachments/assets/81ccf7c3-e1c3-4d90-ba1a-7ddaf3996186" />
<img width="1019" height="962" alt="pge_kaleidoscope_Fri_Sep_12_18_21_23_2025" src="https://github.com/u2084511felix/pge_kaleidoscope/blob/main/Screenshots/pge_kaleidoscope_Fri_Sep_12_18_21_23_2025_.png" />
<img width="1019" height="962" alt="pge_kaleidoscope_Fri_Sep_12_18_23_14_2025_" src="https://github.com/u2084511felix/pge_kaleidoscope/blob/main/Screenshots/pge_kaleidoscope_Fri_Sep_12_18_23_14_2025_.png" />**
