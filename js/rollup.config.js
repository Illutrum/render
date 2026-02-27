import { nodeResolve } from '@rollup/plugin-node-resolve'
import commonjs from '@rollup/plugin-commonjs'
import babel from '@rollup/plugin-babel'

export default {
  input: 'runtime.js',
  output: {
    file: '../runtime-js-compiled.js',
    format: 'iife',
    sourcemap: true
  },
  plugins: [
    nodeResolve(),
    commonjs(),
    babel({
      babelHelpers: 'bundled',
      exclude: 'node_modules/**',
      presets: [['@babel/env', { targets: { browsers: 'defaults' } }]]
    })
  ]
}
