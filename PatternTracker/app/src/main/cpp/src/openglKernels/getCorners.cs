#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 128, local_size_y = 8) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
layout(std430, binding = 2) buffer X_ssbo {float X[];};
layout(std430, binding = 3) buffer Y_ssbo {float Y[];};
layout(std430, binding = 4) buffer C_ssbo {int C[];};
layout(std430, binding = 5) buffer nPts_ssbo {int nPts[];};
void main()
{	
	int sz_x = int((gl_NumWorkGroups.x*gl_WorkGroupSize.x));
    int sz_y = int((gl_NumWorkGroups.y*gl_WorkGroupSize.y));
	
    int idx = int(gl_GlobalInvocationID.x);
    int idy = int(gl_GlobalInvocationID.y);

    int id = idy*sz_x + idx;
    
	int numPts_old=0;
    if(C[id]==1){
        numPts_old = atomicAdd(nPts[0],1);
         X[numPts_old]=float(idx);
         Y[numPts_old]=float(idy);
    }
}