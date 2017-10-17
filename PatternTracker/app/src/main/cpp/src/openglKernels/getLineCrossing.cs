#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 1) in;
layout(binding=0, rgba32f) uniform mediump readonly image2D input_image;
layout(std430, binding = 2) buffer loc_ssbo {float loc[];};
layout(std430, binding = 3) buffer endPtIds_ssbo {int endPtIds[];};
layout(std430, binding = 4) buffer finalLoc_ssbo {float finalLoc[];};
void main()
{
    int id_org = int(gl_GlobalInvocationID.x);
    int endId1 = endPtIds[id_org*2];
    int endId2 = endPtIds[id_org*2+1];
    
    float x1 = loc[2*endId1];
    float y1 = loc[2*endId1+1];
    
    float x2 = loc[2*endId2];
    float y2 = loc[2*endId2+1];
    
    float d = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
    
    float x=(x1+x2)/2.0;
    float y=(y1+y2)/2.0;
	float px = x;
	float py = y;
	
    //shrinkLine(x1,x2,y1,y2,d,input_image, &x, &y);
	ivec2 pos1,pos2;
    vec4 pf1Base,pf2Base,pf1, pf2, pfm;
    
    float dirx = (x2-x1)/d;
    float diry = (y2-y1)/d;
    
    pos1.y=int(floor(x1+0.5f));
    pos1.x=int(floor(y1+0.5f));
    pos2.y=int(floor(x2+0.5f));
    pos2.x=int(floor(y2+0.5f));

	pf1Base = imageLoad(input_image, pos1);
	pf2Base = imageLoad(input_image, pos2);
    //pf1Base = read_imagef(input_image, sampler, pos1);
    //pf2Base = read_imagef(input_image, sampler, pos2);
    
    pf1=pf1Base.xyzw;
    
    float step1=0.0;
    while((abs(pf1.x-pf1Base.x)+abs(pf1.y-pf1Base.y)+abs(pf1.z-pf1Base.z)) < (abs(pf1.x-pf2Base.x)+abs(pf1.y-pf2Base.y)+abs(pf1.z-pf2Base.z))){
        step1+=1.0;
        d-=1.0;
        pos1.y = int(floor(x1+dirx*step1+0.5));
        pos1.x = int(floor(y1+diry*step1+0.5));
		pf1 = imageLoad(input_image, pos1);
        //pf1 = read_imagef(input_image, sampler, pos1);        
    }
    float stepl=step1-1.0;
    float stepr=step1;
    
    ivec2 posl,posr;
    posl.x=pos1.x;
    posl.y=pos1.y;
    posr.x=pos1.x;
    posr.y=pos1.y;
    
    int dir_shift=1;
    int count_shift=0;
    for(int i=0;i<5;i++){
        step1+=1.0;
        pos1.y = int(floor(x1+dirx*step1+0.5));
        pos1.x = int(floor(y1+diry*step1+0.5));
        if(pos1.x>=0 && pos1.x<1920 && pos1.y>=0 && pos1.y<1080){
			pf1 = imageLoad(input_image, pos1);
            //pf1 = read_imagef(input_image, sampler, pos1);   
            if(dir_shift==1){    
                if((abs(pf1.x-pf1Base.x)+abs(pf1.y-pf1Base.y)+abs(pf1.z-pf1Base.z)) < (abs(pf1.x-pf2Base.x)+abs(pf1.y-pf2Base.y)+abs(pf1.z-pf2Base.z))){
                    dir_shift=-1;     
                    posr.x=pos1.x;
                    posr.y=pos1.y;
                    count_shift++;
                }
                }else{
                 if((abs(pf1.x-pf1Base.x)+abs(pf1.y-pf1Base.y)+abs(pf1.z-pf1Base.z)) > (abs(pf1.x-pf2Base.x)+abs(pf1.y-pf2Base.y)+abs(pf1.z-pf2Base.z))){
                    dir_shift=1;
                    posr.x=pos1.x;
                    posr.y=pos1.y;
                    count_shift++;
                }
            }
        }
    } 
    
    if(count_shift>0){
            py = floor((float(posl.x)+float(posr.x))/2.0);
            px = floor((float(posl.y)+float(posr.y))/2.0);
        }else{
            posl.y = int(floor(x1+dirx*stepl+0.5));
            posl.x = int(floor(y1+diry*stepl+0.5));
			pf1 = imageLoad(input_image, posl);
            //pf1 = read_imagef(input_image, sampler, posl);     
            float val_left = abs((abs(pf1.x-pf1Base.x)+abs(pf1.y-pf1Base.y)+abs(pf1.z-pf1Base.z)) - (abs(pf1.x-pf2Base.x)+abs(pf1.y-pf2Base.y)+abs(pf1.z-pf2Base.z)));
            
            posr.y = int(floor(x1+dirx*stepr+0.5));
            posr.x = int(floor(y1+diry*stepr+0.5));
			pf1 = imageLoad(input_image, posr);
            //pf1 = read_imagef(input_image, sampler, posr);     
            float val_right = abs((abs(pf1.x-pf1Base.x)+abs(pf1.y-pf1Base.y)+abs(pf1.z-pf1Base.z)) - (abs(pf1.x-pf2Base.x)+abs(pf1.y-pf2Base.y)+abs(pf1.z-pf2Base.z)));
           
            if(val_left>0.0 && val_right>0.0){
                py = (val_right*float(posl.x)+val_left*float(posr.x))/(val_left+val_right);
                px = (val_right*float(posl.y)+val_left*float(posr.y))/(val_left+val_right);
                }else{
                if(val_left==-0.0 || val_left==0.0){
                    py=float(posl.x);
                    px=float(posl.y);
                }
                if(val_right==-0.0 || val_right==0.0){
                    py=float(posr.x);
                    px=float(posr.y);
                }
            }
    }
    x=px;
	y=py;
    finalLoc[2*id_org]   = x;
    finalLoc[2*id_org+1] = y;
}
