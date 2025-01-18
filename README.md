# Pelican

tiny game engine

## dependency

- [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)

## how to build

command(bash):

```
vcpkg install
cmake . -B build -DCMAKE_TOOLCHAIN_FILE=(VCPKG_ROOT)/scripts/buildsystems/vcpkg.cmake
cmake --build ./build/
```

Replace `(VCPKG_ROOT)` properly.

## directories

- `src`: source files
- `include`: include headers
- `example`: example apps
