#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 64, local_size_y = 8) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
layout(std430, binding = 2) buffer D_ssbo {int D[];};

void checkPass();
void main()
{	
	ivec2 pos;
    
    int R = 1, G=2, B=3;

    int sz_x = int((gl_NumWorkGroups.x*gl_WorkGroupSize.x));
    int sz_y = int((gl_NumWorkGroups.y*gl_WorkGroupSize.y));
    pos.x = int((gl_GlobalInvocationID.x));
    pos.y = int((gl_GlobalInvocationID.y));
	
    int id = pos.y*sz_x + pos.x;
    
	vec4 pixelf = imageLoad(input_image, pos);
    
    float cr=pixelf.x;
    float cg=pixelf.y;
    float cb=pixelf.z;

    //if (id >= N) return;

    if ( (cr + cg + cb) < 0.05){ // 0.78
        D[id] = 0; 
		vec4 pixelfo = vec4(0.0f,0.0f,0.0f,1.0f);
		imageStore(output_image, pos, pixelfo);
    }
    else{
        if (cr > cg){
            if(cr > cb){ 
                D[id] = R;
				//vec4 pixelfo = vec4(1.0f,0.0f,0.0f,1.0f);
				//imageStore(output_image, pos, pixelfo);
            }else{ 
                D[id] = B;
				//vec4 pixelfo = vec4(0.0f,0.0f,1.0f,1.0f);
				//imageStore(output_image, pos, pixelfo);
            }
        }
        else{
            if(cg > cb){ 
                D[id] = G;
				//vec4 pixelfo = vec4(0.0f,1.0f,0.0f,1.0f);
				//imageStore(output_image, pos, pixelfo);
            }else{ 
                D[id] = B;
				//vec4 pixelfo = vec4(0.0f,0.0f,1.0f,1.0f);
				//imageStore(output_image, pos, pixelfo);
            }
        }
    }
}
