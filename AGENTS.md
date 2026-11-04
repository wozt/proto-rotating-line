# Project rules

- C11 with SDL3 for the window; keep all curve generation in portable C math.
- Logical framebuffer is 240x160; display must preserve proportions and use nearest-pixel scaling.
- No artwork, textures, proprietary ROMs or arbitrary random geometry.
- Each curve section must be independently configurable and remain continuous with adjacent sections.
- Add tests for geometry and configuration before adjusting the rendering and avoid changing the Metroidvania repository.
- Build and run CTest, then commit and push the repository after a successful cycle.
