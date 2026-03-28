Current goal is for the engine to render objects in an N64 style to enhance low poly models and get a creepy/atmospheric low-res feel

Ideas:
- 15-bit color quantization (done)
- Dithering to reduce color banding
	- Found documentation describing dithering algorithms used on N64: https://ultra64.ca/files/documentation/online-manuals/man/pro-man/pro15/index15.5.html (this site is kinda a gold mine in general)
		- Magic Square: https://www.cg.tuwien.ac.at/courses/CG2/SS2002/RasterGraphics.pdf (ok I'm pretty sure this is literally just any magic square which is funny)
		- Bayer: https://en.wikipedia.org/wiki/Bayer_filter (done)
		- CD_NOISE: "Adds pesudo-random noise with a very long period into the LSBs of each pixel"
			- Apparently this makes dithering vary from frame to frame?
	    - There's more but this seems decent for now. It keeps talking about fog
