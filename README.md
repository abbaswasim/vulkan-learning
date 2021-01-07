# Vulkan Education

Vulkan Education is my personal vulkan rendering learning tests.

## Platforms

* GLFW
  * MacOS
  * Linux
  * Windows (Eventually)
* Android
* iOS

## Build

This project can be built using `CMake` minimum version 3.6.

```bash
git clone --recurse-submodules https://github.com/abbaswasim/vulkaned.git && cd vulkaned && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j8
```
## Project conventions

This project uses colum major matrices this means the following Matrix4 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15] has 0, 1, 2, 3 as the first colum of the matrix and 3, 4, 5, 6 as the second. Origin of the matrix is at 12, 13, 14 which is also translation.

To transform a vector using this matrix you have to post-multiply it

```c++
Matrix4 mvp;
Vector3 pos;

auto new_pos = mvp * pos;
```
If you pre-multiply position you will need to transpose the matrix to get the same result

```c++
auto same_as_new_pos = pos * mvpT;
```
### Degrees vs Radians

There is no concept of degrees in this project. You can only use radians. If any method is called with degrees the result will be wrong.

### Order of euler angles

This project uses NASA standard aeroplane conventions as described on page: [Euler angles](https://www.euclideanspace.com/maths/geometry/rotations/euler/index.htm)

* Coordinate System: right hand
* Positive angle: right hand
* Order of euler angles: heading first, then attitude, then bank (YZX)
  * heading=Y
  * attitude=Z
  * bank=X

## Third party

This project uses the following third party software as submodules.

* [GLFW](https://github.com/glfw/glfw)
* [Roar](https://github.com/abbaswasim/roar)

## License

The code is licensed under Apache 2.0.
