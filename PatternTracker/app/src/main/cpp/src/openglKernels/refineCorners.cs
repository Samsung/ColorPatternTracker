#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 2) in;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
layout(std430, binding = 2) buffer C_ssbo {int C[];};
layout(std430, binding = 3) buffer CNew_ssbo {int CNew[];};
void main()
{	
	int sz_x = int((gl_NumWorkGroups.x*gl_WorkGroupSize.x));
    int sz_y = int((gl_NumWorkGroups.y*gl_WorkGroupSize.y));
	
    int x = int(gl_GlobalInvocationID.x);
    int y = int(gl_GlobalInvocationID.y);
	    
    int sz_win = 5;
    
    if(x<sz_win || x>=sz_x-sz_win || y<sz_win || y>=sz_y-sz_win)return;
    
    int id = y*sz_x+x;
    
    if(C[id]==0)return;    
    
    int c;
    
    int mx=0;
    int my=0;
    int count=0;
    for(int i=-sz_win;i<sz_win;i++)
    {
        for(int j=-sz_win;j<sz_win-1;j++){
            id = (y+j)*sz_x+(x+i);
            if(C[id]==1){
                mx += (x+i);
                my += (y+j);
                count++;
            }
        }
    }
    
    mx /= count;
    my /= count;
    
    x = mx;
    y = my;
    
    id = y*sz_x+x;
    CNew[id]=1;
	
	// if(CNew[id]==1){
		// ivec2 pos;
		// for(int i=-sz_win;i<sz_win;i++)
		// {
			// for(int j=-sz_win;j<sz_win-1;j++){
				// pos.x = x+i;
				// pos.y = y+j;
				// vec4 pixelfo = vec4(1.0f,1.0f,1.0f,1.0f);
				// imageStore(output_image, pos, pixelfo);					
			// }
		// }
	// }
}