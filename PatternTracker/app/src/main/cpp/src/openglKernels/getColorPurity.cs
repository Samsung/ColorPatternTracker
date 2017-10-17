#version 310 es
#define LOCAL_SIZE 1

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 2) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
layout(std430, binding = 2) buffer P_ssbo {int P[];};
layout(std430, binding = 3) buffer params_int_ssbo {int params_int[];};
void main()
{
	int sz_blk = params_int[0];
	int skip = params_int[1];
	
	ivec2 pos;
    vec4 pixelf;
    int idx, idy, id, id_blk;
    
	int sz_x = int(gl_NumWorkGroups.x*gl_WorkGroupSize.x);
    int sz_y = int(gl_NumWorkGroups.y*gl_WorkGroupSize.y);
	
    idx = int(gl_GlobalInvocationID.x);
    idy = int(gl_GlobalInvocationID.y);
    
    id_blk = idy*sz_x+idx;

    idx *= sz_blk;
    idy *= sz_blk;
    
    sz_x *= sz_blk;
    sz_y *= sz_blk;
    
    float v_avg=0.0;
    int count=0;
    for(int i=0;i<sz_blk;i+=skip){
        for(int j=0;j<sz_blk;j+=skip){
            pos.x = idx+i;
            pos.y = idy+j;
			pixelf = imageLoad(input_image, pos);
			//vec4 pixelfo = vec4(1.0f,1.0f,1.0f,1.0f);
			//imageStore(output_image, pos, pixelfo);
			
            //pixelf = read_imagef(input_image, sampler, pos);
            
            float gr = (pixelf.x + pixelf.y + pixelf.z)/3.0;
            
            if(gr<0.2)continue;
            
            if(pixelf.x >= pixelf.y && pixelf.y >= pixelf.z){
                v_avg += (pixelf.x - pixelf.y)/gr;
            }
            if(pixelf.x >= pixelf.z && pixelf.z >= pixelf.y){
                v_avg += (pixelf.x - pixelf.z)/gr;
            }
            if(pixelf.y >= pixelf.z && pixelf.z >= pixelf.x){
                v_avg += (pixelf.y - pixelf.z)/gr;
            }
            if(pixelf.y >= pixelf.x && pixelf.x >= pixelf.z){
                v_avg += (pixelf.y - pixelf.x)/gr;
            }
            if(pixelf.z >= pixelf.x && pixelf.x >= pixelf.y){
                v_avg += (pixelf.z - pixelf.x)/gr;
            }
            if(pixelf.z >= pixelf.y && pixelf.y >= pixelf.x){
                v_avg += (pixelf.z - pixelf.y)/gr;
            }
            count++;
        }
    }


    if(count<2){
        P[id_blk]=0;
        return;
    }

    v_avg /= float(count);
    
    if(v_avg>0.3){
        P[id_blk]=1;
        }else{
        P[id_blk]=0;
    }
	
	// if(P[id_blk]==1){
		// for(int i=0;i<sz_blk;i+=skip){
			// for(int j=0;j<sz_blk;j+=skip){
				// pos.x = idx+i;
				// pos.y = idy+j;
				// vec4 pixelfo = vec4(1.0f,1.0f,1.0f,1.0f);
				// imageStore(output_image, pos, pixelfo);
			// }
		// }
	// }
}