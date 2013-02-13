
uniform samplerBuffer lightListColor;
uniform samplerBuffer lightListPosition;
uniform isamplerBuffer lightList;
uniform isamplerBuffer lightListOffset;
uniform vec4 lightTileSize;

vec3 rn_calculateLight(vec3 inNormal, vec3 inPosition)
{
	vec3 normal = normalize(inNormal);
	vec3 posdiff = vec3(0.0);
	float attenuation = 0.0;
	vec3 light = vec3(0.1);
	vec4 lightpos;
	vec3 lightcolor;
	int lightindex = 0;

	int tileindex = int(int(gl_FragCoord.y/lightTileSize.y)*lightTileSize.z+int(gl_FragCoord.x/lightTileSize.x));
	ivec2 listoffset = texelFetch(lightListOffset, tileindex).xy;

	for(int i = 0; i < listoffset.y; i++)
	{
		lightindex = texelFetch(lightList, listoffset.x+i).r;
		lightpos = texelFetch(lightListPosition, lightindex);
		lightcolor = texelFetch(lightListColor, lightindex).xyz;

		posdiff = lightpos.xyz-inPosition;
		attenuation = max((lightpos.w-length(posdiff))/lightpos.w, 0.0);

		light += lightcolor*max(dot(normal, normalize(posdiff)), 0.0)*attenuation*attenuation;
	}

	return light;
}
