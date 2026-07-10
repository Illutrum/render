#!/usr/bin/env node
/**
 * Generate MSDF font atlas (.png + .fnt) for FastGFX examples.
 * Requires: npm install msdf-bmfont-xml (run from game-dev/fgfx/tools)
 *
 * Usage: node generate-msdf-font.mjs <ttf-path> <output-basename>
 * Example: node generate-msdf-font.mjs /usr/share/fonts/TTF/DejaVuSans.ttf ../examples/assets/ui
 */

import { writeFile } from 'node:fs/promises'
import { dirname, resolve } from 'node:path'
import { fileURLToPath } from 'node:url'
import generateBMFont from 'msdf-bmfont-xml'

const __dirname = dirname(fileURLToPath(import.meta.url))

const ttfPath = process.argv[2] ?? '/usr/share/fonts/TTF/DejaVuSans.ttf'
const outputBase = resolve(__dirname, process.argv[3] ?? '../examples/assets/ui')
const charset = ' !"#$%&\'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~'

const { textures, font } = await new Promise((resolvePromise, reject) => {
  generateBMFont(ttfPath, {
    outputType: 'txt',
    filename: outputBase,
    charset,
    fontSize: 42,
    textureSize: [512, 512],
    distanceRange: 4,
    fieldType: 'msdf'
  }, (error, textures, font) => {
    if (error) reject(error)
    else resolvePromise({ textures, font })
  })
})

const pngPath = `${outputBase}.png`
const fntPath = `${outputBase}.fnt`

await writeFile(pngPath, textures[0].texture)
await writeFile(fntPath, font.data)

console.log(`Wrote ${pngPath}`)
console.log(`Wrote ${fntPath}`)
