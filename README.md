# Lucurious
[![License](https://img.shields.io/badge/license-MIT-brightgreen.svg)](#license)

Inspired by the t.v series expanse and other syfy t.v shows/movies.

Lucurious (L) Lowkey (U) unsure and curious if this is even possible, but why stop now!!! It\'s a library for building and styling 3D Wayland Compositors. Will be using [Vulkan](https://vulkan.lunarg.com) and the Wayland protocol to turn the nonexistent UI into reality.

## Dependencies
* wayland
* wayland-protocol
* vulkan
* libinput

**Current Usage**

Current pulls up red cube mesa 3D (OpenGL implementation used) example. Will be porting over to vulkan soon!!
```bash
mkdir -v build
meson build
ninja -C build
./build/lucur
```

## Development

**How to use wayland-scanner**
```bash
wayland-scanner private-code < xdg-shell.xml > xdg-shell-protocol.c
wayland-scanner server-header < xdg-shell.xml > xdg-shell-server-protocol.h
wayland-scanner client-header < xdg-shell.xml > xdg-shell-client-protocol.h
```

## References
* [Wayland freedesktop](https://wayland.freedesktop.org/)
* [An introduction to Wayland](https://drewdevault.com/2017/06/10/Introduction-to-Wayland.html)
* [Weston](https://github.com/wayland-project/weston)
* [Programming Wayland Clients](https://jan.newmarch.name/Wayland/index.html)
* [Swaywm](https://github.com/swaywm)
* [Emersion Hello Wayland](https://github.com/emersion/hello-wayland)
* [Direct Rendering Manager](https://dri.freedesktop.org/wiki/DRM/)
* [vulkan](https://vulkan.lunarg.com)
