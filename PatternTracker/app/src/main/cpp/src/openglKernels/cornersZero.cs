#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 2) in;
layout(std430, binding = 2) buffer C_ssbo {int C[];};
void main()
{
	int sz_x = int((gl_NumWorkGroups.x*gl_WorkGroupSize.x));
    int sz_y = int((gl_NumWorkGroups.y*gl_WorkGroupSize.y));
	
    int idx = int(gl_GlobalInvocationID.x);
    int idy = int(gl_GlobalInvocationID.y);
	
    int id = idy*sz_x + idx;
    C[id]=0;
}