#version 150
out vec4 oFragColor;
#define COLOR vec4(0.0,0.737,0.6,1.0);
void main(void)
{
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
	// aspect-ratio correction
   	uv.x*=iResolution.x/iResolution.y;
   	uv.x -= iRenderXY.x;
   	uv.y -= iRenderXY.y;

    vec4 noise = texture(iChannel1,uv + iGlobalTime / 10.0);
    vec4 col = texture(iChannel0,uv + noise.xy * 0.01);
   	oFragColor = col * 1.5 * COLOR;
}
