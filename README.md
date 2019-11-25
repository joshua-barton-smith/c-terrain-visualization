# CS 3GC3 Assignment 4 - Terrain Generation

## Additional Features
1. Improved Camera - the camera can be moved and rotated freely in the style of an FPS camera, using W/S/A/D for movement and mouse for rotation.

2. 2D Terrain Overview - we chose to implement this as a minimap displayed as part of a 2D HUD rather than as a secondary window. The minimap is colored according to the coloring of the terrain (green for low points, fading to red at high points) and additionally shows a blue cross indicating the camera location when it is over the top of the terrain.
-- Side note, the terrain overview doesn't seem to work on (maybe all Linux machines?), possibly because of differences in the way GL_POINT is being rendered.

3. Terrain Generation Animation - when it's initially generated or re-generated, the terrain will start fully flat and green and will animate its points until they reach their actual height/color.

## Instructions

Move the camera with W/S/A/D and mouse.
Swap rendering mode (filled polys, wires, or both) with the F key.
Enable and disable Gouraud shading with the H key.
Enable and disable lighting with the L key.
Generate new terrain with the R key.
Swap between terrain textures with the T key.
Swap between a quad or a triangle mesh with the M key.