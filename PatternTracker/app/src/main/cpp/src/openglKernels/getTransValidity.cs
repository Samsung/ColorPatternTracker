#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 2) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
layout(std430, binding = 2) buffer transValidity_ssbo {int validity[];};
layout(std430, binding = 3) buffer data_ssbo {float data[];};
layout(std430, binding = 4) buffer inVals_ssbo {float inVals[];};
void main()
{
	float x_mid, y_mid, angle, d_w, d_h;
	x_mid = inVals[0];
	y_mid = inVals[1];
	angle = inVals[2];
	d_w = inVals[3];
	d_h = inVals[4];
	
    int id1 = int(float(gl_GlobalInvocationID.x));
    int id2 = int(float(gl_GlobalInvocationID.y));
		    
    //int id = id1*18+id2;
    //if(id>=405)return;
    int id = id1*8+id2;
    if(id>=125)return;
    
    //int nx_check = 4;
    //int ny_check = 4;
    //int nt_check = 2;
    
    int nx_check = 2;
    int ny_check = 2;
    int nt_check = 2;
    
    float dxt = (floor(float(id)/float((2*ny_check+1)*(2*nt_check+1)))-float(nx_check));
    int id_temp = id%((2*ny_check+1)*(2*nt_check+1));
    float dyt = (floor(float(id_temp)/float(2*nt_check+1))-float(ny_check));
    float dt = float((id_temp%(2*nt_check+1))-nt_check);
      
    float dx = dxt*d_w*cos(angle)+dyt*d_h*sin(angle);
    float dy = -dxt*d_w*sin(angle)+dyt*d_h*cos(angle);
    dt = dt*3.142f/12.0f;
    
    float trans[2][2];
	trans[0][0] = cos(dt);
	trans[0][1] = -sin(dt);
	trans[1][0] = sin(dt);
	trans[1][1] = cos(dt);
	
    ivec2 pos;
    vec4 pf;
    
    //uchar color[16] = {1,2,1,2,2,3,2,3,1,2,1,2,3,1,3,1};
    int color[16] = int[16](1,3,1,2,2,1,2,3,3,2,3,1,1,3,1,2); // new pattern
    
    int flag=1;
    for(int i=0;i<16;i++){
        float x = data[2*i]-x_mid;
        float y = data[2*i+1]-y_mid;

        pos.y = int(floor(x*trans[0][0]+y*trans[0][1]+dx+x_mid+0.5f));
        pos.x = int(floor(x*trans[1][0]+y*trans[1][1]+dy+y_mid+0.5f));
        
        if(pos.x<0 || pos.x>=1920 || pos.y<0 || pos.y>=1080){flag=0;break;}
        
		pf = imageLoad(input_image, pos);
        //pf = read_imagef(input_image, sampler, pos);
        if(color[i]==1 && (pf.x<pf.y || pf.x<pf.z)){flag=0;break;}
        if(color[i]==2 && (pf.y<pf.x || pf.y<pf.z)){flag=0;break;}
        if(color[i]==3 && (pf.z<pf.y || pf.z<pf.x)){flag=0;break;}
    }
    
    // check neighborhood
    if(flag==1){
        float d=d_h/4.0f;
        if(d>d_w/4.0f)d=d_w/4.0f;
        d = floor(d);
        if(d>2.0)d=2.0;
        int iVec[4]=int[4](0, 3, 12, 5);
        for(int t=0;t<4;t++){
            int i=iVec[t];
            float x = data[2*i]-x_mid;
            float y = data[2*i+1]-y_mid;
            
            float xt[4] = float[4](-d,-d, d, d);
            float yt[4] = float[4](-d, d,-d, d);
            
            for(int k=0;k<4;k++){
                pos.y = int(floor((x+xt[k])*trans[0][0]+(y+yt[k])*trans[0][1]+dx+x_mid+0.5f));
                pos.x = int(floor((x+xt[k])*trans[1][0]+(y+yt[k])*trans[1][1]+dy+y_mid+0.5f));
                
                if(pos.x<0 || pos.x>=1920 || pos.y<0 || pos.y>=1080){flag=0;break;}
                
				pf = imageLoad(input_image, pos);
                //pf = read_imagef(input_image, sampler, pos);
                if(color[i]==1 && (pf.x<pf.y || pf.x<pf.z)){flag=0;break;}
                if(color[i]==2 && (pf.y<pf.x || pf.y<pf.z)){flag=0;break;}
                if(color[i]==3 && (pf.z<pf.y || pf.z<pf.x)){flag=0;break;}
            }
            if(flag==0)break;
        }
    }
    
    validity[id]=flag;
}