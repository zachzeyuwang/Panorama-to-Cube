# Panorama-to-Cube

Part of code for the paper

**Zeyu Wang**, Xiaohan Jin, Fei Xue, Xin He, Renju Li, Hongbin Zha

[Panorama to Cube: A Content-Aware Representation Method](https://www.researchgate.net/publication/281589647_Panorama_to_Cube_A_Content-Aware_Representation_Method)

ACM SIGGRAPH Asia Technical Briefs, 2015

## Documentation

- `panorama_processing.cpp`: OpenCV-based code for panorama processing, including shifting, orientation rectification, panorama to cube, and panorama to cuboid.
- `Source.cpp`, `glad.c`, `camera.h`, `shader.h`, `shader.vs`, `shader.fs`: OpenGL-based code for a viewer of the cube or cuboid converted from a panorama.
- `panorama.jpg`: Original 2:1 panoramic image.
- `0-5.jpg`, `6.jpg`: Top, front, right, back, left, bottom faces of the converted cube, and the cutlines overlayed on the original panorama.
- `a0-a5.jpg`, `a6.jpg`: Top, front, right, back, left, bottom faces of the converted cuboid, and the cutlines overlayed on the original panorama.
