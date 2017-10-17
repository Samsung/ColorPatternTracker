#version 310 es
#define LOCAL_SIZE 1

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE) in;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
layout(std430, binding = 2) buffer pts_ssbo {float pts[];};
void main()
{
    ivec2 pos;
    vec4 pixelf;
    
    pixelf.x = 0.0;
    pixelf.y = 0.0;
    pixelf.z = 0.0;
    pixelf.w = 1.0;
    
    int id = int(gl_GlobalInvocationID.x);
    
    int x,y;
    x=int(floor(pts[2*id]+0.5));
    y=int(floor(pts[2*id+1]+0.5));
        
    int s=5;
    int flag=0;
    for(int i=-s;i<s;i++){
        for(int j=-s;j<s;j++){
            if(x+i>=0 && x+i<1080 && y+j>=0 && y+j<1920){
                pos.x=y+j;
                pos.y=x+i;
                pixelf.x = 1.0;
                pixelf.y = 1.0;
                pixelf.z = 1.0;
                pixelf.w = 1.0;
				imageStore(output_image, pos, pixelf);
                //write_imagef(out_image, pos, pixelf);
            }
        }
    }  
}