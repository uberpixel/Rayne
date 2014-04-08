#line 2 // -*- c++ -*-
/**
 \file rn_SAO_reconstruct.fsh
 \author Morgan McGuire, NVIDIA Research

 Routines for reconstructing linear Z, camera-space position, and camera-space face normals from a standard
 or infinite OpenGL projection matrix from G3D.
 */

// Note that positions (which may affect z) are snapped during rasterization, but 
// attributes are not.

/* 
 Clipping plane constants for use by reconstructZ

 clipInfo = (z_f == -inf()) ? Vector3(z_n, -1.0f, 1.0f) : Vector3(z_n * z_f,  z_n - z_f,  z_f);
*/
uniform vec2      clipPlanes;

float reconstructCSZ(float d) {
    return clipPlanes.x*clipPlanes.y / ((clipPlanes.x-clipPlanes.y) * d + clipPlanes.y);
}

/**  vec4(-2.0f / (width*P[0][0]), 
          -2.0f / (height*P[1][1]),
          ( 1.0f - P[0][2]) / P[0][0], 
          ( 1.0f + P[1][2]) / P[1][1])
    
    where P is the projection matrix that maps camera space points 
    to [-1, 1] x [-1, 1].  That is, GCamera::getProjectUnit(). */
//uniform vec4 projInfo;

uniform mat4 matProj;

/** Reconstruct camera-space P.xyz from screen-space S = (x, y) in
    pixels and camera-space z < 0.  Assumes that the upper-left pixel center
    is at (0.5, 0.5) [but that need not be the location at which the sample tap 
    was placed!]

    Costs 3 MADD.  Error is on the order of 10^3 at the far plane, partly due to z precision.
  */
vec3 reconstructCSPosition(vec2 S, float z) {
	vec4 projInfo = vec4(-2.0f / (1920.0*matProj[0][0]),
						 -2.0f / (1080.0*matProj[1][1]),
						 ( 1.0f - matProj[0][2]) / matProj[0][0],
						 ( 1.0f + matProj[1][2]) / matProj[1][1]);
    return vec3((S.xy * projInfo.xy + projInfo.zw) * z, z);
}

/** Reconstructs screen-space unit normal from screen-space position */
vec3 reconstructCSFaceNormal(vec3 C) {
    return normalize(cross(dFdy(C), dFdx(C)));
}

vec3 reconstructNonUnitCSFaceNormal(vec3 C) {
    return cross(dFdy(C), dFdx(C));
}
