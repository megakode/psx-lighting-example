# Playstation 1 lighting example

Simple example showing a flat-shaded rotating cube with one static directional light source, running on the Sony Playstation 1.

The code is written in C and requires the Psy-Q SDK, which was the official SDK bundled with the Sony Playstation 1 Development kit.

The thing to take away from this example is the lighting, which is explained in a pretty complicated way in the SDK, but in reality is pretty simple.

## Basic directional light

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

These matrices are then used by any of the Color functions. Either the Rot......Col or the dedicated NormalCol ones.

## Rotation

The problem with all the light functions is, that you give a surface normal vector to it, for calculating an output color. These are usually precalculated and included in your 3d model or surface data. This is all fine for static non-rotated surfaces, but as soon as you begin calling the SetRotationMatrix or by other means rotating your surfaces, you giving the same precalculated surface normal to the light functions will result in the exact same light output as before the rotation, and will therefore look as if the light is following the rotation of the object. Which we don't want. We want it to stay static, as a lamp or the sun, and shine on the different sides of the object as it rotates.

We therefore have two options:
- recalculate all the newly rotated surface normals on every frame, before passing them to the lighting functions
- counter rotate the light itself and set this in the local light matrix

We of course take the second option. So if you rotate the surface 10 degrees around the X axis, you will have to rotate the light -10 degrees around the X axis. Be aware of the sequence order of rotations! Rotating around X, then Y, then Z, will mean you will have to counter rotate in the opposite sequence order: Z, Y then X, otherwise you will end up in a very different end rotation.

Everything is explained in more mathematical details in the official Psy-Q documentation, but seeing an example and a quick "easy" explanation often helps.
