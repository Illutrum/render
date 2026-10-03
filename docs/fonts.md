# Fonts and text assets

FastGFX supports two font workflows: **MSDF** (signed-distance-field, scalable) and **bitmap sprite fonts** (BMFont + PNG atlas).

Both use ASCII `.fnt` descriptors (BMFont text format). XML `.fnt` is accepted as a fallback parser path.

## MSDF fonts (MSDFLayer)

Used by `msdf_example` and asset-manager loading screens (`assets/ui`).

### Generate with msdf-bmfont-xml

Tool: [tools/generate-msdf-font.mjs](../tools/generate-msdf-font.mjs)

```bash
cd game-dev/fgfx/tools
npm install msdf-bmfont-xml
node generate-msdf-font.mjs /usr/share/fonts/TTF/DejaVuSans.ttf ../examples/assets/ui
```

This writes:

- `examples/assets/ui.png` — MSDF atlas
- `examples/assets/ui.fnt` — ASCII BMFont descriptor

Important settings in the script:

- `outputType: 'txt'` — ASCII format (preferred; XML is fallback only)
- `fieldType: 'msdf'`
- `distanceRange: 4` — must match shader / `MSDFFont::setPxRange`
- `charset` — printable ASCII by default

### Loading in code

```cpp
auto font = fgfx::engine->getMSDFFont("assets/ui");
auto layer = fgfx::engine->createMSDFLayer();
layer->drawText(font, "Hello", glm::vec2(0, 0), 1.0f, glm::vec4(1));
```

On web, font PNG and `.fnt` are fetched via `fgfxLoader.loadMSDFFontImage` / `loadMSDFFontData` in the JS runtime.

## Bitmap sprite fonts (SpriteFont)

Used by `logo_blur` example (`assets/cat`).

### Generate with Hiero (libGDX)

Reference settings: [examples/assets/cat.hiero](../examples/assets/cat.hiero)

1. Open Hiero (`com.badlogic.gdx.tools.hiero.Hiero`)
2. Load a TTF, set size (32), padding, glyph set
3. Apply effects (outline + color in the reference file)
4. Export PNG atlas + `.fnt` (text format)

Output:

- `cat.png`
- `cat.fnt`

### Loading in code

```cpp
auto font = fgfx::engine->getSpriteFont("assets/cat");
```

## General assets (PNG)

Native: `fgfx::loadPng(path)` reads via `fgfx::io::loadFile` and decodes with libpng.

Web: images load through `fgfxLoader.loadSpriteImage` / `loadTextureImage` in the JS TextureManager.

Paths are relative to the process working directory (examples use `assets/` next to the binary or wasm output).
