import pluginBabel from 'rollup-plugin-babel'
import pluginCommonJS from 'rollup-plugin-commonjs'
import { terser as pluginTerser } from 'rollup-plugin-terser'

export default {
  input: 'src/index.js',

  plugins: [
    pluginCommonJS(),

    pluginBabel({
      exclude: /node_modules/,

      presets: [
        [
          '@babel/preset-env',
          {
            exclude: ['transform-async-to-generator', 'transform-regenerator'],
          },
        ],
      ],

      plugins: [],
    }),

    pluginTerser(),
  ],

  output: {
    sourcemap: false,
    file: 'dist/cache.min.js',
    name: '__cachePolyfill',
    format: 'umd',
    footer: `
function _expose(target, name, interfaceObject) {
  Object.defineProperty(target, name, {
    __proto__: null,
    writable: true,
    enumerable: false,
    configurable: true,
    value: interfaceObject
  });
}

_expose(self, "Cache", self.__cachePolyfill.Cache);
_expose(self, "CacheStorage", self.__cachePolyfill.CacheStorage);
_expose(self, "caches", self.__cachePolyfill.caches);

self.__cachePolyfill = undefined;
_expose = undefined;
`
  },
}
