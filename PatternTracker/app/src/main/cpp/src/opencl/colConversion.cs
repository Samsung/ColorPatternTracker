#version 310 es
#define LOCAL_SIZE 1024

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = LOCAL_SIZE) in;
layout(binding=0, rgba32f) uniform mediump readonly imageBuffer velocity_buffer;
layout(binding=1, rgba32f) uniform mediump writeonly imageBuffer position_buffer;

void main()
{
	vec4 vel = imageLoad(velocity_buffer, int(gl_GlobalInvocationID.x));
	vel += vec4(0.0f, 0.0f, 25.0f, 12.5f);
	vec4 result = vec4(gl_LocalInvocationID.x, gl_WorkGroupID.x, gl_LocalInvocationID.y, gl_WorkGroupID.y);
	imageStore(position_buffer, int(gl_GlobalInvocationID.x), result);
}