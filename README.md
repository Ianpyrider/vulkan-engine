# VulkaN64: A Modern Vulkan 1.4 Rendering Engine 

Inspired by the low-poly, uncanny-valley fantasy worlds of the N64 era, I wanted to explore what an engine with modern graphics programming and GPU capabilities would be capable of in this style. I hope to use this engine as a sandbox to explore my love of physical simulation and raytracing in the highest performance environment possible, all while capturing the essence of what is, in my opinion, the most immersive and magical era of 3D graphical worlds!          

https://github.com/user-attachments/assets/210e7ea4-8985-48f2-a35f-1aedb52068a5

(You can see the dithering pattern more easily in fullscreen! For more info: https://en.wikipedia.org/wiki/Dither)

## Engine Features

- Languages: C++, Slang
- Build System: CMake
- Vulkan 1.4 using modern API features: RAII, Dynamic Rendering, Sync2
- Vulkan Memory Allocator (VMA) for more efficient/safe memory allocation
- Granular GPU Profiling via timestamp queries for identifying performance bottlenecks 
- Robust architecture emphasizing separation of concerns in idiomatic Vulkan, supporting all manner of future development and developers!

## Performance Metrics

- Average total frame time (Razer Blade w/ Nvidia RTX 2070 using mailbox presentation): ~0.05ms = 20k FPS
	- 3D Compute: ~0.005ms
	- Mesh Graphics Pass: ~0.003ms
	- Snow Graphics Pass: ~0.002ms
	- 2D Compute: ~0.03ms
- Summing the individual parts and comparing to the total gives us a ceiling of ~0.01ms for sync elements like barriers, meaning commands are waiting on one another negligibly! 
- The 2D compute N64-style pass is currently the bottleneck
	- It seems like this could be for a couple of reasons: Using eGeneral for my swapchain image, making it a suboptimal write target, my shader math being slow (which seems unlikely, since the shader math is only a few steps), or maybe suboptimal worker pooling (but 16x16 seems to be a solid standard)
	- I had the shader directly output its input values, skipping the math, and this shader remained the bottleneck at ~0.025ms, supporting my layout theory
	- Potential fixes: Combine into the fragment shader and bypass eGeneral entirely, or do some staging buffer-like trickery
	- This bottleneck doesn't scale with anything except for screen size, so I feel like 0.03ms is perfectly acceptable for now (and likely forever). I also like that our post-processing can be run in parallel with the fragment shader for the next frame.

## Compute Shaders

### 3D Particle Shader

- Updates the 3D positions of snow particles in parallel to achieve a realtime snowfall effect
- Custom compute shader simulates wind and gravity while introducing noise, tracking the position and velocity of individual particles, and looping particles within a bounding box
- Currently renders 2048 particles in under a hundreth of a ms for compute and draw
	- I chose this number for aesthetic purposes, but I've tried up to a million particles (which turns the screen almost white), and it remains extremely performant. The bottleneck becomes the graphics pass at that point, but I could probably get better performance through culling.

### N64 Post-Processing Shader

- Applies N64-like post processing operations to an offscreen image drawn to by the graphics pipelines via another custom shader, outputting to the swapchain
- Color quantization: Crushes existing colors into their 15-bit counterparts
- Bayer Dithering: Applies a tiled 8x8 Bayer matrix to the image, keeping with the retro style and concealing color banding as suggested by official N64 programming documentation (https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro15/index15.5.html)
	- The Bayer matrix definitely captured the look I wanted best, but the manual also mentions a couple more approaches I tested or want to try out, sources/info which I'll list here:
		- Magic Square: https://www.cg.tuwien.ac.at/courses/CG2/SS2002/RasterGraphics.pdf (turns out you can probably just use any magic square, which is funny)
		- Bayer: https://en.wikipedia.org/wiki/Bayer_filter
		- CD_NOISE: "Adds pesudo-random noise with a very long period into the LSBs of each pixel"
			- I guess this would make the bayer pattern less predictable?
	    - There's more (look back at the docs!) but this seems decent for now. Fog seems important

## Challenges and Learnings (so far)

- Learning Vulkan and building an engine in ~1.5 months
	- Easily the most challenging and rewarding part of this project was maintaining a strong mental model of the Vulkan specification's various moving parts and how they relate to each other. Often my ability to progress felt directly proportional to how deeply I understood the API, since breaking away from the tutorial and building my own features required me to implement updates to the pipeline end-to-end. 
- Integrating my initial compute pipeline with an offscreen image
	- This combined a pretty wide variety of elements I hadn't used yet and had to combine in a novel way based on documentation instead of a tutorial. I had to learn descriptor set layouts, pools, and sets, uniform images allocated via VMA, a new type of shader, offscreen rendering, and image layout transitions/barriers, all on top of setting up a compute pipeline. 
	- I figured since I had dealt with uniform buffers in OpenGL and used many of the other elements, it would work out fine, but I probably could've done it a lot faster if I had taken each step one at a time instead of learning them all at once and fighting validation layers. That said, I walked away from this particular task feeling like a grisled veteran.
- App Architecture
	- Vulkan docs doesn't provide much guidance on structuring projects like this: The initial tutorial is all on one doc and the only guide they provide is a modern game engine architecture tutorial that felt pretty far beyond the scope of what I need right now. Luckily I have no shortage of experience with organizing codebases and system design, but even then I had to spend a lot of time making sure my mental models were solid in order to actually make a decent structure. 
	- I think my structure is solid for the current scope of the project, but I still spend a good amount of time refactoring, so if I build something more substantial with this as a baseline I'll probably use that modern engine tutorial.  

## Ideas for Engine Optimization

- Performance
	- Currently we use 24-bit colors and crush them to their 15-bit counterparts via a compute shader, but I wonder if I can configure Vulkan to pass 15-bit colors to the fragment shader, bypassing the need for this step and reducing the volume of color data we have to pass across the pipeline
	- Depth-based culling of snow particles
- Architecture
	- Renderer is too long/complex right now, I need a class that allows me to write to the command buffer in pipeline-based chunks instead of all in one call
	- Too much falls under the responsibility of VulkanContext, especially when integrating new features, so I'll have to break down engine-wide initialization into logical chunks
	- EngineConfig currently handles more than engine config, eventually flags and constants will need to be addressed in a more systematic way
	- Maybe port over my quadric error simplification/loop subdivision project so developers can generate low-poly meshes more easily

## Feature Goals

- Realtime ray tracing! https://docs.vulkan.org/tutorial/latest/courses/18_Ray_tracing/00_Overview.html 
	- PBR would also be cool to implement, especially if raytracing is too intense/doesn't capture the style well (not that PBR necessarily will but it's coool)
- Keep learning about graphics performance optimizations, and how they vary across different use cases or are specific to certain ones
- I really want to do something physics based, I'd love to look into fluid simulation the most but I also could do FEM-based mesh warping/collision or interactive particle simulation
- Make a basic game demo using the engine!
	- Use a more scalable archtecture: https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/introduction.html 
	- Support 2d as well/hotswapping between the two?
	- I have this idea for an overhead section of a game where giant hands are messing with you from either side while you run across a bridge, then you somehow go into 3d and there are like eldritch horror creatures towering over either side of the bridge that the hands belonged to
- Shaders with cool visual effects: https://www.reddit.com/r/GraphicsProgramming/comments/1s9bf3y/does_anyone_else_think_signed_distance_functions/
	- Imagine a MacGuffin in an N64 game that was high-poly and looked like this lmaoo
