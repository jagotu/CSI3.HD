# CSI3 HD

Use only 4:3 resolutions, otherwise stuff gets cut off (technically you can use taller but it will get letterboxed)

## Installing

1. Download Ultimate ASI Loader d3d8.dll (https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/Win32-latest/d3d8-Win32.zip) and extract the d3d8.dll next to CSI3.exe
2. Download CSI3.HD latest release (https://github.com/jagotu/CSI3.HD/releases/latest) and extract the scripts folder next to CSI3.EXE
3. Modify scripts/CSI3.HD.ini with your desired resolution
4. Go catch some murderers

## Limit FPS

On modern Windows, the game effectively has no frame limit/vsync, supposedly because of a DirectX 8 implementation issue. This means that it will use your GPU to 100%, which is probably a waste.
Luckily, this can be easily fixed by using d3d8to9, which is built-in the ASI Loader. Follow https://github.com/ThirteenAG/Ultimate-ASI-Loader?tab=readme-ov-file#d3d8to9 to enable it.

## Building
Build it in https://github.com/ThirteenAG/WidescreenFixesPack/ environment

## 4k screenshots
![4k screenshot 1](screenshots/4k_1.png)
![4k screenshot 2](screenshots/4k_2.png)
