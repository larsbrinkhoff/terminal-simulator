#version 120

uniform vec2 resolution;
uniform sampler2D tex;

void main (void)
{
  vec2 pos = gl_FragCoord.xy / resolution;
  pos.y = 1.0 - pos.y;
  vec3 col = texture2D (tex, pos).rgb;
  gl_FragColor = vec4 (col, 1.0);
}
