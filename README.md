# Pelican

tiny game engine

## dependency

- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
- [vcpkg](https://github.com/microsoft/vcpkg)

## how to build

command:

```
cmake . -B build -DCMAKE_TOOLCHAIN_FILE=(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
cmake --build ./build/
```

Replace `(VCPKG_ROOT)` properly.

## directories

- `src`: source files
- `include`: include headers
- `example`: example apps
