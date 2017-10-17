#version 310 es
#define LOCAL_SIZE 1

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
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
    pos.y=int(floor(pts[2*id]+0.5));
    pos.x=int(floor(pts[2*id+1]+0.5)); 

    // -1 is white
    // -2 is black
    // -3 is undefined
    float brightest_col=-1.0;
    float brightest_col2=-1.0;
    if(pos.y>=0 && pos.y<1080 && pos.x>=0 && pos.x<1920){
		pixelf = imageLoad(input_image, pos);
        //pixelf = read_imagef(input_image, sampler, pos);
        float gr = (pixelf.x + pixelf.y + pixelf.z)/3.0;
        if(gr>0.99)gr=0.99;
            if(pixelf.x>=pixelf.y && pixelf.y>=pixelf.z){
                brightest_col=pixelf.x;
                brightest_col2=pixelf.y;
            }
            if(pixelf.x>=pixelf.z && pixelf.z>=pixelf.y){
                brightest_col=pixelf.x;
                brightest_col2=pixelf.z;
            }
            if(pixelf.y>=pixelf.x && pixelf.x>=pixelf.z){
                brightest_col=pixelf.y;
                brightest_col2=pixelf.x;
                gr = gr+1.0;
            }
            if(pixelf.y>=pixelf.z && pixelf.z>=pixelf.x){
                brightest_col=pixelf.y;
                brightest_col2=pixelf.z;
                gr = gr+1.0;
            }
            if(pixelf.z>=pixelf.x && pixelf.x>=pixelf.y){
                brightest_col=pixelf.z;
                brightest_col2=pixelf.x;
                gr = gr+2.0;
            }
            if(pixelf.z>=pixelf.y && pixelf.y>=pixelf.x){
                brightest_col=pixelf.z;
                brightest_col2=pixelf.y;
                gr = gr+2.0;
            }   
            pts[2*id]=gr;         
            pts[2*id+1]=(brightest_col-brightest_col2)/gr;   
        }else{
            pts[2*id]=-1.0;
            pts[2*id+1]=-1.0;
    }
}