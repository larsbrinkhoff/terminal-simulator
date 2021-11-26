#version 120

uniform vec2 resolution;
uniform float curvature;
uniform sampler2D tex;

float gauss (float x, float sigma)
{
  x /= sigma;
  return exp (-0.5 * x * x) / sigma / 6.2832;
}

void main (void)
{
  int size2 = 2;
  float kernel2[5];

  vec2 pos = gl_FragCoord.xy / resolution;
  pos.y = 1.0 - pos.y;

  pos -= vec2 (0.5);
  float curve = curvature * dot (pos, pos);
  pos += curve * pos;
  pos += vec2 (0.5);

  for (int i = 0; i <= size2; i++)
    kernel2[size2 + i] = kernel2[size2 - i] = gauss (float (i), 0.6);

  float scanline = 0.0;
  for (int i = -size2; i <= size2; i++) {
    for (int j = -size2; j <= size2; j++)
      scanline += kernel2[size2 + i] * kernel2[size2 + j] *
        texture2D (tex, pos + vec2 (i, j) / resolution).r;
  }
  scanline *= 1.5*12.0;

  vec3 col = vec3 (0.0);
  col += scanline * vec3 (0.8, 0.9, 1.0);
  col += vec3 (0.10, 0.15, 0.10);
  gl_FragColor = vec4 (col, 1.0);
}
