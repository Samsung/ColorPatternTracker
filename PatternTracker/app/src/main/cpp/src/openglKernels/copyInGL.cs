#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 2) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
layout(binding=1, rgba32f) uniform mediump writeonly image2D output_image;
void main()
{
	ivec2 pos;
    
    pos.x = int(gl_GlobalInvocationID.x);
    pos.y = int(gl_GlobalInvocationID.y);
	
	vec4 pixelf = imageLoad(input_image, pos);
	imageStore(output_image, pos, pixelf);
}