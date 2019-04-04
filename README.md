# AsyncVideoToolkit

## AsyncVideoPlayer

A Hardware-Accelerated Video Player for Streaming Frames from Disk for Pixel-Accurate Rendering to High Definition/Retina Display Outputs with Maximum Hardware Performance and Minimal Throughput Latency

### Features

  - Native Win32 Window and WGL OpenGL Context Creation
  - Manual OpenGL function loading 
  - Dedicated Platform Window Event and OpenGL Rendering Threads
  - Memory Mapped Raw (Uncompressed) BGRA Texture File Streaming From Disk
  - Dedicated Threads separate from the rendering thread for asynchronous streaming and texture upload using Pixel Buffer Objects (PBOs) and OpenGL 4.2 Persistent-Mapped Buffers
  - A Win32 Circular Buffer Implementation for texture handle synchronization between threads
  - OpenGL ES 2.0/GLSL 2.1 Compatible Shaders for Quad VBO Texture Rendering
  
### Usage

  - Play/Pause [Spacebar]
  - Enable/Disable Vsync [V]
  - Toggle Window Border [B]
  - Toggle Fullscreen Mode [F]
  - Open File Dialog to load .bgra file containing Uncompressed Video frames in BGRA format for playback
  - Quit [Esc]
