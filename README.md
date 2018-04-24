# Playstation 1 lighting example

Simple example showing a flat-shaded rotating cube with one static directional light source, running on the Sony Playstation 1.

The code is written in C and requires the Psy-Q SDK, which was the official SDK bundled with the Sony Playstation 1 Development kit.

The thing to take away from this example is the lighting, which is explained in a pretty complicated way in the SDK, but in reality is pretty simple.

The basic lighting system on the PSX has 3 parallel lights in 3 user defined colors. On modern engines/systems these are usually called directional lights. A parallel light is named this way because it has an infinite amount of light rays in parallel to eachother throughout the entire scene. So unlike a spot-light, they are basicially everywhere. That means that to represent a parallel light, we only need 2 things:

* A vector for the direction
* RGB values for the color

On the PSX, these things are stored in two constant matrices called the __local light matrix__ and the __local color matrix__. These can be set using the __SetLightMatrix__ and __SetColorMatrix__ functions. 

The format of these matrices are as follows:

__Local light matrix__ stores direction for all 3 lights:

| L1x | L1y | L1z |
|-----|-----|-----|
| L2x | L2y | L2z |
| L3x | L3y | L3z |

__Local color matrix:__ stores color of all 3 lights:

| L1r | L2r | L3r |
|-----|-----|-----|
| L1g | L2g | L3g |
| L1b | L2b | L3b |

Ln x,y,z = directional vectors
Ln r,g,b = RGB color values

Note that the directional vectors fill horizontally in the matrix and the colors vertically.

Everything is explained in more mathematical details in the official Psy-Q documentation, but seeing an example and a quick "easy" explanation often helps.
