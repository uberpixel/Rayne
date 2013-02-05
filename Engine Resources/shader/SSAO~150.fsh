
#version 150
precision highp float;

uniform sampler2D mTexture0; // Random normals
uniform sampler2D targetmap0; // Scene

in vec2 texcoord;

float strength = 0.07;
float offset = 18.0;
float falloff = 0.000002;
float rad = 0.006;

#define SSAO_SAMPLES 10
const float invSamples = -1.38/10.0;

out vec4 fragColor0;

void main()
{
	/*vec3 pSphere[10] = vec3[](vec3(-0.010735935, 0.01647018, 0.0062425877),vec3(-0.06533369, 0.3647007, -0.13746321),vec3(-0.6539235, -0.016726388, -0.53000957),vec3(0.40958285, 0.0052428036, -0.5591124),vec3(-0.1465366, 0.09899267, 0.15571679),vec3(-0.44122112, -0.5458797, 0.04912532),vec3(0.03755566, -0.10961345, -0.33040273),vec3(0.019100213, 0.29652783, 0.066237666),vec3(0.8765323, 0.011236004, 0.28265962),vec3(0.29264435, -0.40794238, 0.15964167));
	vec3 fres = normalize((texture(mTexture0, texcoord * offset).xyz * 2.0) - vec3(1.0));

	vec4 currentPixelSample = texture(targetmap0, texcoord);
	float currentPixelDepth = currentPixelSample.a;

	vec3 ep = vec3(texcoord.xy, currentPixelDepth);
	vec3 norm = currentPixelSample.xyz * 2.0 - 1.0;

	float bl = 0.0;
	float radD = rad / currentPixelDepth;

	float occluderDepth, depthDifference;
	vec4 occluderFragment;
	vec3 ray;

	for(int i=0; i<SSAO_SAMPLES; i++)
	{
		ray = radD * reflect(pSphere[i], fres);

		occluderFragment = texture(targetmap0, ep.xy + sign(dot(ray, norm)) * ray.xy);
		depthDifference = currentPixelDepth - occluderFragment.a;

		bl += step(falloff,depthDifference)*(1.0-dot(occluderFragment.xyz*2.0-1.0,norm))*(1.0-smoothstep(falloff,strength,depthDifference));
	}

	float ao = 1.0 + bl * invSamples;
	fragColor0.rgb = vec3(ao, ao, ao);
	fragColor0.a = 1.0;*/

	fragColor0 = texture(mTexture0, texcoord);
}
