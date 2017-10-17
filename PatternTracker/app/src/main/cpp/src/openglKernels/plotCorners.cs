#version 310 es
#define LOCAL_SIZE 1

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE) in;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
layout(std430, binding = 2) buffer pts_ssbo {float pts[];};
layout(std430, binding = 3) buffer colVals_ssbo {float colVals[];};
void main()
{
	float r = colVals[0];
	float g = colVals[1];
	float b = colVals[2];
	
    int p0 = int(gl_GlobalInvocationID.x);
    ivec2 pos;
    vec4 pixelf;
    
    int x = int(pts[p0*2]);
    int y = int(pts[p0*2+1]);
    
    int s=5;
    int flag=0;
    for(int i=-s;i<s;i++){
        for(int j=-s;j<s;j++){
             pos.x=y+j;
             pos.y=x+i;
             if(pos.x>=0 && pos.x<1920 && pos.y>=0 && pos.y<1080){
                 pixelf.x = r;
                 pixelf.y = g;
                 pixelf.z = b;
                 pixelf.w = 1.0;
				 imageStore(output_image, pos, pixelf);
                 //write_imagef(out_image, pos, pixelf);
            }
        }
    }  
}