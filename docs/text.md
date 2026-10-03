# MSDF / bitmap text layout

`measureString` returns **advance width × lineHeight**. That is for wrapping and caret motion, not for placing a glyph in a box.

`measureInk` returns the AABB of the actual glyph quads in the same font space as `renderString` (pen origin, +X right, +Y up). Use that for centering, fitting, and aligning ink to a cell.

```cpp
fgfx::TextInk ink = font->measureInk("♒");
float scale = fgfx::fitInkScale(ink, boxW, boxH, 0.7f);
glm::mat4 mat = fgfx::textYDownMat(worldCenter, scale, ink, fgfx::TextAlignH::Center);
font->renderString(batch, mat, color, "♒");
```

`textYDownMat` is for the usual Y-down ortho (`glm::ortho(..., +h/2, -h/2)`). Do not add extra `lineHeight * 0.25` fudge offsets.

Do **not** center with `measureString().x / 2` or assume the visual center sits on the baseline. Astrology symbols, `°`, and accented letters all have bearings; the fudge will miss.

Page type size (`MSDFFont` names, degrees, date) comes from the Kiwi `scale` variable (`w == scale * ink-at-1`), with a preferred row height of 7.5mm on the A4 sheet. `fitInkScale` is only for glyphs that should fill an already-solved cell (planet/sign marks in a legend box or identity circle).
