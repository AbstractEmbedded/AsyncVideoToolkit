# AsyncVideoToolkit

## AsyncVideoPlayer

A Video Player Application for Streaming Frames from Disk for Pixel-Accurate Rendering to High Resolution Display Ouputs with Maximum Hardware Performance and Minimal Throughput Latency

### Features

  - Native Win32 Window and WGL OpenGL Context Creation
  - Manual OpenGL function loading 
  - Dedicated Platform Window Event and OpenGL Rendering Threads
  - Memory Mapped Raw (Uncompressed) BGRA Texture File Streaming From Disk
  - Dedicated Threads separate from the rendering thread for asynchronous streaming and texture upload using Pixel Buffer Objects (PBOs) and OpenGL 4.2 Persistent-Mapped Buffers
  - A Win32 Circular Buffer Implementation for texture handle synchronization between threads
  - Quad VBO Texture Rendering
  
### Usage

  - Play/Pause [Spacebar]
  - Enable/Disable Vsync [V]
  - Toggle Window Border [B]
  - Toggle Fullscreen Mode [F]
  - Open File Dialog to load uncompressed .bgra file containing Uncompressed Video frames in BGRA format for playback
  - Quit [Esc]
