import { babel } from '@rollup/plugin-babel';

const input = process.env.I18NEXT_INPUT;
const output = process.env.I18NEXT_OUTPUT;

if (!input || !output) {
  throw new Error('I18NEXT_INPUT and I18NEXT_OUTPUT must be set.');
}

export default {
  input,
  output: {
    file: output,
    format: 'es',
  },
  plugins: [
    babel({
      babelHelpers: 'bundled',
      babelrc: false,
      configFile: false,
      comments: false,
      presets: [
        [
          '@babel/preset-env',
          {
            targets: {
              chrome: '49',
            },
            modules: false,
          },
        ],
      ],
    }),
  ],
};
