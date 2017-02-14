//#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable 

const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

bool checkRimCornerBool(__global uchar* D, int idx, int idy, int sz_x, int r_rim);
void checkRimCorner(__global uchar* D, int idx, int idy, int sz_x, int r_rim, uchar *pcol1, uchar *pcol2, uchar *pcol3);
void shrinkBox(__global uchar* D, size_t *px, size_t *py, int w, int h, int sz_x);
void jointDetect(__global uchar* D, size_t *px, size_t *py, int w, int h, int sz_x);
void binarySearch(int x1, int x2, int y1, int y2, int *x_ret, int *y_ret, int w, int h, __global uchar *D);
uchar getColVal_colMajor(int x, int y, int w, int h, __global uchar *D);
void shrinkLine(float x1, float x2, float y1, float y2, float d,__read_only image2d_t input_image, float *px, float *py);

__kernel void plotCorners(__write_only image2d_t out_image, __global float * pts, float r, float g, float b){
    size_t p0 = get_global_id(0);
    int2 pos;
    float4 pixelf;
    
    int x = (int)pts[p0*2];
    int y = (int)pts[p0*2+1];
    
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
                 write_imagef(out_image, pos, pixelf);
            }
        }
    }    
}
 
__kernel void writeCorners(__write_only image2d_t out_image, __global float * pts, int nPts){
    size_t p0 = get_global_id(0);
    int2 pos;
    float4 pixelf;
    
    int x = (int)pts[p0*2];
    int y = (int)pts[p0*2+1];
    
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
                write_imagef(out_image, pos, pixelf);
            }
        }
    }
}

__kernel void getLineCrossing(__read_only image2d_t input_image, __global float * loc, __global int *endPtIds, __global float *finalLoc){
    size_t id = get_global_id(0);
    int endId1 = endPtIds[id*2];
    int endId2 = endPtIds[id*2+1];
    
    float x1 = loc[2*endId1];
    float y1 = loc[2*endId1+1];
    
    float x2 = loc[2*endId2];
    float y2 = loc[2*endId2+1];
    
    float d = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
    
    float x=(x1+x2)/2;
    float y=(y1+y2)/2;
    shrinkLine(x1,x2,y1,y2,d,input_image, &x, &y);
    
    finalLoc[2*id]   = x;
    finalLoc[2*id+1] = y;
}

__kernel void getColPixels(__read_only image2d_t input_image, __global float * loc, __global float *colPixel){
    size_t id = get_global_id(0);

    float x = loc[2*id];
    float y = loc[2*id+1];
               
    int2 pos;
    float4 pfBase;
    
    pos.y=floor(x+0.5f);
    pos.x=floor(y+0.5f);

    pfBase = read_imagef(input_image, sampler, pos);
    
    colPixel[3*id]   = pfBase.x;
    colPixel[3*id+1] = pfBase.y;
    colPixel[3*id+2] = pfBase.z;

}

// runs as 
__kernel void getTransValidity(__read_only image2d_t input_image, __global uchar * validity, __global float *data, float x_mid, float y_mid, float angle, float d_w, float d_h){

    size_t id1 = get_global_id(0);
    size_t id2 = get_global_id(1);
    
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

    
    int dxt = floor(id/(float)((2*ny_check+1)*(2*nt_check+1)))-nx_check;
    int id_temp = id%((2*ny_check+1)*(2*nt_check+1));
    int dyt = floor(id_temp/(float)(2*nt_check+1))-ny_check;
    float dt = (id_temp%(2*nt_check+1))-nt_check;
      
    float dx = dxt*d_w*cos(angle)+dyt*d_h*sin(angle);
    float dy = -dxt*d_w*sin(angle)+dyt*d_h*cos(angle);
    dt = dt*3.142f/12.0f;
    
    float trans[2][2] = {{cos(dt),-sin(dt)},{sin(dt),cos(dt)}};

    int2 pos;
    float4 pf;
    
    //uchar color[16] = {1,2,1,2,2,3,2,3,1,2,1,2,3,1,3,1};
    uchar color[16] = {1,3,1,2,2,1,2,3,3,2,3,1,1,3,1,2}; // new pattern
    
    uchar flag=1;
    for(int i=0;i<16;i++){
        float x = data[2*i]-x_mid;
        float y = data[2*i+1]-y_mid;

        pos.y = floor(x*trans[0][0]+y*trans[0][1]+dx+x_mid+0.5f);
        pos.x = floor(x*trans[1][0]+y*trans[1][1]+dy+y_mid+0.5f);
        
        if(pos.x<0 || pos.x>=1920 || pos.y<0 || pos.y>=1080){flag=0;break;}
        
        pf = read_imagef(input_image, sampler, pos);
        if(color[i]==1 && (pf.x<pf.y || pf.x<pf.z)){flag=0;break;}
        if(color[i]==2 && (pf.y<pf.x || pf.y<pf.z)){flag=0;break;}
        if(color[i]==3 && (pf.z<pf.y || pf.z<pf.x)){flag=0;break;}
    }
    
    // check neighborhood
    if(flag==1){
        float d=d_h/4.0f;
        if(d>d_w/4.0f)d=d_w/4.0f;
        d = floor(d);
        if(d>2)d=2;
        int iVec[4]={0, 3, 12, 5};
        for(int t=0;t<4;t++){
            int i=iVec[t];
            float x = data[2*i]-x_mid;
            float y = data[2*i+1]-y_mid;
            
            float xt[4] = {-d,-d, d, d};
            float yt[4] = {-d, d,-d, d};
            
            for(int k=0;k<4;k++){
                pos.y = floor((x+xt[k])*trans[0][0]+(y+yt[k])*trans[0][1]+dx+x_mid+0.5f);
                pos.x = floor((x+xt[k])*trans[1][0]+(y+yt[k])*trans[1][1]+dy+y_mid+0.5f);
                
                if(pos.x<0 || pos.x>=1920 || pos.y<0 || pos.y>=1080){flag=0;break;}
                
                pf = read_imagef(input_image, sampler, pos);
                if(color[i]==1 && (pf.x<pf.y || pf.x<pf.z)){flag=0;break;}
                if(color[i]==2 && (pf.y<pf.x || pf.y<pf.z)){flag=0;break;}
                if(color[i]==3 && (pf.z<pf.y || pf.z<pf.x)){flag=0;break;}
            }
            if(flag==0)break;
        }
    }
    
    validity[id]=flag;
}

void shrinkLine(float x1, float x2, float y1, float y2, float d,__read_only image2d_t input_image, float *px, float *py){
    int2 pos1,pos2;
    float4 pf1Base,pf2Base,pf1, pf2, pfm;
    
    float dirx = (x2-x1)/d;
    float diry = (y2-y1)/d;
    
    pos1.y=floor(x1+0.5f);
    pos1.x=floor(y1+0.5f);
    pos2.y=floor(x2+0.5f);
    pos2.x=floor(y2+0.5f);

    pf1Base = read_imagef(input_image, sampler, pos1);
    pf2Base = read_imagef(input_image, sampler, pos2);
    
    pf1=pf1Base.xyzw;
    
    int step1=0;
    while((fabs(pf1.x-pf1Base.x)+fabs(pf1.y-pf1Base.y)+fabs(pf1.z-pf1Base.z)) < (fabs(pf1.x-pf2Base.x)+fabs(pf1.y-pf2Base.y)+fabs(pf1.z-pf2Base.z))){
        step1++;
        d-=1;
        pos1.y = floor(x1+dirx*step1+0.5);
        pos1.x = floor(y1+diry*step1+0.5);
        pf1 = read_imagef(input_image, sampler, pos1);        
    }
    int stepl=step1-1;
    int stepr=step1;
    
    int2 posl,posr;
    posl.x=pos1.x;
    posl.y=pos1.y;
    posr.x=pos1.x;
    posr.y=pos1.y;
    
    int dir_shift=1;
    int count_shift=0;
    for(int i=0;i<5;i++){
        step1++;
        pos1.y = floor(x1+dirx*step1+0.5);
        pos1.x = floor(y1+diry*step1+0.5);
        if(pos1.x>=0 && pos1.x<1920 && pos1.y>=0 && pos1.y<1080){
            pf1 = read_imagef(input_image, sampler, pos1);   
            if(dir_shift==1){    
                if((fabs(pf1.x-pf1Base.x)+fabs(pf1.y-pf1Base.y)+fabs(pf1.z-pf1Base.z)) < (fabs(pf1.x-pf2Base.x)+fabs(pf1.y-pf2Base.y)+fabs(pf1.z-pf2Base.z))){
                    dir_shift=-1;     
                    posr.x=pos1.x;
                    posr.y=pos1.y;
                    count_shift++;
                }
                }else{
                 if((fabs(pf1.x-pf1Base.x)+fabs(pf1.y-pf1Base.y)+fabs(pf1.z-pf1Base.z)) > (fabs(pf1.x-pf2Base.x)+fabs(pf1.y-pf2Base.y)+fabs(pf1.z-pf2Base.z))){
                    dir_shift=1;
                    posr.x=pos1.x;
                    posr.y=pos1.y;
                    count_shift++;
                }
            }
        }
    } 
    
    if(count_shift>0){
            *py = floor(((float)posl.x+(float)posr.x)/2);
            *px = floor(((float)posl.y+(float)posr.y)/2);
        }else{
            posl.y = floor(x1+dirx*stepl+0.5);
            posl.x = floor(y1+diry*stepl+0.5);
            pf1 = read_imagef(input_image, sampler, posl);     
            float val_left = fabs((fabs(pf1.x-pf1Base.x)+fabs(pf1.y-pf1Base.y)+fabs(pf1.z-pf1Base.z)) - (fabs(pf1.x-pf2Base.x)+fabs(pf1.y-pf2Base.y)+fabs(pf1.z-pf2Base.z)));
            
            posr.y = floor(x1+dirx*stepr+0.5);
            posr.x = floor(y1+diry*stepr+0.5);
            pf1 = read_imagef(input_image, sampler, posr);     
            float val_right = fabs((fabs(pf1.x-pf1Base.x)+fabs(pf1.y-pf1Base.y)+fabs(pf1.z-pf1Base.z)) - (fabs(pf1.x-pf2Base.x)+fabs(pf1.y-pf2Base.y)+fabs(pf1.z-pf2Base.z)));
           
            if(val_left>0 && val_right>0){
                *py = (val_right*posl.x+val_left*posr.x)/(val_left+val_right);
                *px = (val_right*posl.y+val_left*posr.y)/(val_left+val_right);
                }else{
                if(val_left==0){
                    *py=posl.x;
                    *px=posl.y;
                }
                if(val_right==0){
                    *py=posr.x;
                    *px=posr.y;
                }
            }
    }
}

__kernel void getPatternIndicator(__read_only image2d_t input_image, __global float * pts){
    int2 pos;
    float4 pixelf;
    
    pixelf.x = 0.0;
    pixelf.y = 0.0;
    pixelf.z = 0.0;
    pixelf.w = 1.0;
    
    size_t id = get_global_id(0);
    
    int x,y;
    pos.y=floor(pts[2*id]+0.5);
    pos.x=floor(pts[2*id+1]+0.5); 

    // -1 is white
    // -2 is black
    // -3 is undefined
    float brightest_col=-1;
    float brightest_col2=-1;
    if(pos.y>=0 && pos.y<1080 && pos.x>=0 && pos.x<1920){
        pixelf = read_imagef(input_image, sampler, pos);
        float gr = (pixelf.x + pixelf.y + pixelf.z)/3;
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
                gr = gr+1;
            }
            if(pixelf.y>=pixelf.z && pixelf.z>=pixelf.x){
                brightest_col=pixelf.y;
                brightest_col2=pixelf.z;
                gr = gr+1;
            }
            if(pixelf.z>=pixelf.x && pixelf.x>=pixelf.y){
                brightest_col=pixelf.z;
                brightest_col2=pixelf.x;
                gr = gr+2;
            }
            if(pixelf.z>=pixelf.y && pixelf.y>=pixelf.x){
                brightest_col=pixelf.z;
                brightest_col2=pixelf.y;
                gr = gr+2;
            }   
            pts[2*id]=gr;         
            pts[2*id+1]=(brightest_col-brightest_col2)/gr;   
        }else{
            pts[2*id]=-1;
            pts[2*id+1]=-1;
    }
}

__kernel void markDetectedCorners(__write_only image2d_t out_image, __global float * pts){
    int2 pos;
    float4 pixelf;
    
    pixelf.x = 0.0;
    pixelf.y = 0.0;
    pixelf.z = 0.0;
    pixelf.w = 1.0;
    
    size_t id = get_global_id(0);
    
    int x,y;
    x=floor(pts[2*id]+0.5);
    y=floor(pts[2*id+1]+0.5);
        
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
                write_imagef(out_image, pos, pixelf);
            }
        }
    }
}

__kernel void getLinePtAssignment(__global int * ptsVote, __global uchar * D, __global float * lineEqns, __global float *x, __global float *y, int w_img, int h_img){
    int debug=1;
    size_t nPts = get_global_size(0);
    size_t p0 = get_global_id(0);
    size_t p1 = get_global_id(1);
    int deg_eq=27;
    
    if(debug==1){
        for(int i=0;i<deg_eq;i++){
            lineEqns[(p0*nPts+p1)*deg_eq+i]=0;
        }
    }

    if(p1 <= p0)return;
    
    // getting the line equation
    float x1 = x[p0];
    float y1 = y[p0];
    float x2 = x[p1];
    float y2 = y[p1];
    
    float a = (y2 - y1);
    float b = (x1 - x2);
    float c = (x2*y1 - x1*y2);    
    float d = sqrt(a*a+b*b);

    // if points are too close, they may represent the same point
    if(d<=5)return;
    
    a /= d;
    b /= d;
    c /= d;
    
    float d_pts = d;//(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
    
    // direction of line is (-b, a) from p0 to p1
    // get points on line at 4 equidistant places spread across the two points
    float xm[4];
    float ym[4]; 
    int xmt[4], ymt[4], xmb[4], ymb[4];
    xm[0] = x1 - d_pts/4.0*(-b);
    ym[0] = y1 - d_pts/4.0*(a);
    xm[1] = x1 + d_pts/4.0*(-b);
    ym[1] = y1 + d_pts/4.0*(a);
    xm[2] = x1 + 3*d_pts/4.0*(-b);
    ym[2] = y1 + 3*d_pts/4.0*(a);
    xm[3] = x1 + 5*d_pts/4.0*(-b);
    ym[3] = y1 + 5*d_pts/4.0*(a);

    // get points and colors just top and bottom of the line
    for(int i=0;i<4;i++){
        xmt[i] = floor(xm[i] + 5*a);
        ymt[i] = floor(ym[i] + 5*b);
        
        xmb[i] = floor(xm[i] - 5*a);
        ymb[i] = floor(ym[i] - 5*b);
    }

    uchar ct[4],cb[4];
    for(int i=0;i<4;i++){
        ct[i] = getColVal_colMajor(xmt[i], ymt[i], w_img, h_img, D);
        cb[i] = getColVal_colMajor(xmb[i], ymb[i], w_img, h_img, D);
    }
    
    for(int i=0;i<4;i++)
    {
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+0 ] = xmt[i];
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+1 ] = ymt[i];
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+2 ] = xmb[i];
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+3 ] = ymb[i];
        lineEqns[ (p0*nPts+p1)*deg_eq+16+2*i ] = ct[i];
        lineEqns[ (p0*nPts+p1)*deg_eq+16+2*i+1 ] = cb[i];
        }
    }
    
    if(ct[1]==0 || cb[1]==0 || ct[2]==0 || cb[2]==0){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 0;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1;
        }
        return;
    }
    if(ct[0]!=0 && cb[0]!=0 && ct[0]==cb[0]){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 1;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1;
        }
        return;
    }
    if(ct[3]!=0 && cb[3]!=0 && ct[3]==cb[3]){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 2;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1;
        }
        return;
    }
    if(ct[1]==cb[1] || ct[2]==cb[2] || ct[1]==ct[2] || cb[1]==cb[2]){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 3;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1;
        }
        return;
    }
     
    // find the line ID(s)
    // colors of 4x4 grid pattern
    uchar colset[4][4];
    /*
    colset[0][0] = 1;
    colset[0][1] = 2;
    colset[0][2] = 1;
    colset[0][3] = 3;

    colset[1][0] = 2;
    colset[1][1] = 3;
    colset[1][2] = 2;
    colset[1][3] = 1;

    colset[2][0] = 1;
    colset[2][1] = 2;
    colset[2][2] = 1;
    colset[2][3] = 3;

    colset[3][0] = 2;
    colset[3][1] = 3;
    colset[3][2] = 2;
    colset[3][3] = 1;
*/
    colset[0][0] = 1;
    colset[0][1] = 2;
    colset[0][2] = 3;
    colset[0][3] = 1;

    colset[1][0] = 3;
    colset[1][1] = 1;
    colset[1][2] = 2;
    colset[1][3] = 3;

    colset[2][0] = 1;
    colset[2][1] = 2;
    colset[2][2] = 3;
    colset[2][3] = 1;

    colset[3][0] = 2;
    colset[3][1] = 3;
    colset[3][2] = 1;
    colset[3][3] = 2;
    
    uchar linet[6][4];
    uchar lineb[6][4];
    
    // colors for top and bottom of each of the horizontal and vertical lines
    for(int i=0;i<3;i++){
        for(int j=0;j<4;j++){
            // horizontal lines
            linet[i][j]=colset[i][j];
            lineb[i][j]=colset[i+1][j];
            
            // vertical lines
            linet[3+i][j]=colset[j][i+1];
            lineb[3+i][j]=colset[j][i];
        }
    }
   
    int linePts[6][3];

    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            linePts[i][j] = j*3+i;
            linePts[3+i][j] = j+3*i;
        }
    }
    
    // check lines
    for(int i=0;i<6;i++)
    {
        int flag=1;
        for(int j=0;j<4;j++){
            if(ct[j]!=0 && cb[j]!=0 && (ct[j]!=linet[i][j] || cb[j]!=lineb[i][j]) ){
                flag=0;
                break;
            }
        }
        if(flag==0){ // reverse lin
            flag=2;
            for(int j=0;j<4;j++){
                if(ct[j]!=0 && cb[j]!=0 && (ct[j]!=lineb[i][3-j] || cb[j]!=linet[i][3-j]) ){
                    flag=0;
                    break;
                }
            }
        }
        
        // found matching line
        if(flag>0){            
            // find middle point
            // binary search for crossing
            int x1_mid,x2_mid,y1_mid,y2_mid;
            int *px1_mid,*px2_mid,*py1_mid,*py2_mid;
            px1_mid = &x1_mid;
            px2_mid = &x2_mid;
            py1_mid = &y1_mid;
            py2_mid = &y2_mid;

            binarySearch(xmt[1], xmt[2], ymt[1], ymt[2], px1_mid, py1_mid, w_img, h_img, D);
            binarySearch(xmb[1], xmb[2], ymb[1], ymb[2], px2_mid, py2_mid, w_img, h_img, D);

            x1_mid = *px1_mid;
            x2_mid = *px2_mid;
            y1_mid = *py1_mid;
            y2_mid = *py2_mid;
            
            // find intersection point - alpha is described in definition of x_mid and y_mid - put these values in eq of the line and find the value of alpha
            float alpha = (a*x2_mid+b*y2_mid+c)/(a*x2_mid+b*y2_mid-a*x1_mid-b*y1_mid);
            int x_mid = floor(x1_mid+alpha*(x2_mid-x1_mid));
            int y_mid = floor(y1_mid+alpha*(y2_mid-y1_mid));
            
            if(debug==1){
            lineEqns[ (p0*nPts+p1)*deg_eq+16 ] = p0;
            lineEqns[ (p0*nPts+p1)*deg_eq+17 ] = p1;
            lineEqns[ (p0*nPts+p1)*deg_eq+18 ] = x1;
            lineEqns[ (p0*nPts+p1)*deg_eq+19 ] = y1;
            lineEqns[ (p0*nPts+p1)*deg_eq+20 ] = x2;
            lineEqns[ (p0*nPts+p1)*deg_eq+21 ] = y2;
            lineEqns[ (p0*nPts+p1)*deg_eq+22 ] = x_mid;
            lineEqns[ (p0*nPts+p1)*deg_eq+23 ] = y_mid;            
            lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = i;
            lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = flag;
            lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 2;
            }
            
            // find all points near mid
            for(int t=0;t<nPts;t++){
                if(fabs(x[t]-x_mid)<5 && fabs(y[t]-y_mid)<5){
                    atomic_inc(&ptsVote[t*9+linePts[i][1]]);
                }
                if(fabs(x[t]-x1)<5 && fabs(y[t]-y1)<5){
                    if(flag==1){
                        atomic_inc(&ptsVote[t*9+linePts[i][0]]);
                        }else{
                        atomic_inc(&ptsVote[t*9+linePts[i][2]]);
                    }
                }
                if(fabs(x[t]-x2)<5 && fabs(y[t]-y2)<5){
                    if(flag==1){
                        atomic_inc(&ptsVote[t*9+linePts[i][2]]);
                        }else{
                        atomic_inc(&ptsVote[t*9+linePts[i][0]]);
                    }
                }
            }
        }
    }
}

void binarySearch(int x1, int x2, int y1, int y2, int *x_ret, int *y_ret, int w, int h, __global uchar *D){
    int x_mid = (x1+x2)/2;
    int y_mid = (y1+y2)/2;
        
    int id1 = x1*h + y1;
    int id2 = x2*h + y2;
    
    uchar c1 = D[id1];
    uchar c2 = D[id2];
        
    int id_mid = x_mid*h + y_mid;
    uchar c_mid = D[id_mid];
    
    while( abs(x1-x2) + abs(y1-y2) >2 ){
        if(c_mid==c1){
            x1=x_mid;
            y1=y_mid;
            id1 = x1*h + y1;
        }
        if(c_mid==c2){
            x2=x_mid;
            y2=y_mid;
            id2 = x2*h + y2;
        }
        if(c_mid==0) {*x_ret=-1; *y_ret=-1;return;}
        if(c_mid!=c1 && c_mid!=c2 && c_mid!=0){*x_ret=x_mid; *y_ret=y_mid; return;}
        x_mid = (x1+x2)/2;
        y_mid = (y1+y2)/2;
        id_mid = x_mid*h + y_mid;
        c_mid = D[id_mid];
    }
    *x_ret=x_mid; *y_ret=y_mid;
}

uchar getColVal_colMajor(int x, int y, int w, int h, __global uchar *D)
{
    uchar c=4;
    if(x>=0 && x<w && y>=0 && y<h)
    {
        int id = x*h + y;
        c = D[id];
    }
    return c;
}

__kernel void integralZero(__global int *Iimg)
{
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t idx = get_global_id(0);
    size_t idy = get_global_id(1);
    
    int id = idy*sz_x + idx;
    Iimg[id]=0;
}

__kernel void cornersZero(__global uchar *C)
{
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t idx = get_global_id(0);
    size_t idy = get_global_id(1);

    int id = idy*sz_x + idx;
    C[id]=0;
}

bool checkRimCornerBool(__global uchar* D, int idx, int idy, int sz_x, int r_rim)
{
    uchar col1, col2, col3;
    col1=0;
    col2=0;
    col3=0;
    
    checkRimCorner(D, idx, idy, sz_x, r_rim, &col1, &col2, &col3);
    
    if((col1>0 && col2>0) && col3>0){return true; }
    
    return false;
}

void checkRimCorner(__global uchar* D, int idx, int idy, int sz_x, int r_rim, uchar *pcol1, uchar *pcol2, uchar *pcol3)
{
    size_t id, idbx, idby;
    uchar c;
    for(int i=0;i<2*r_rim;i++){
        idbx = idx+i;
        idby = idy;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)*pcol1=1;
        if(c==2)*pcol2=1;
        if(c==3)*pcol3=1;

        idbx = idx+i;
        idby = idy+2*r_rim-1;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)*pcol1=1;
        if(c==2)*pcol2=1;
        if(c==3)*pcol3=1;
    }

    for(int i=1;i<2*r_rim-1;i++){
        idbx = idx;
        idby = idy+i;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)*pcol1=1;
        if(c==2)*pcol2=1;
        if(c==3)*pcol3=1;

        idbx = idx+2*r_rim-1;
        idby = idy+i;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)*pcol1=1;
        if(c==2)*pcol2=1;
        if(c==3)*pcol3=1;
    }
}

__kernel void refineCorners(__global uchar* C, __global uchar* CNew){
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t x = get_global_id(0);
    size_t y = get_global_id(1);
    
    int sz_win = 5;
    
    if(x<sz_win || x>=sz_x-sz_win || y<sz_win || y>=sz_y-sz_win)return;
    
    size_t id = y*sz_x+x;
    
    if(C[id]==0)return;    
    
    uchar c;
    
    float mx=0;
    float my=0;
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
    
    x = floor(mx);
    y = floor(my);
    
    id = y*sz_x+x;
    CNew[id]=1;
}

__kernel void getBlockCorners10(__global uchar* D, __global uchar* C, __global uchar *purity, int sz_step){
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t idx  = get_global_id(0);
    size_t idy  = get_global_id(1);
    
    if(idx>=sz_x-5 || idy>=sz_y-5) return;
    
    size_t id, idbx, idby;
    int colID;
    
    id = idy*sz_x+idx;
    if(purity[id]==0) return;
    
    idx *= sz_step;
    idy *= sz_step;
    sz_x *= sz_step;
    sz_y *= sz_step;
    
    int r_rim=7;

    if(checkRimCornerBool(D, idx, idy, sz_x, r_rim)){
        // find corner
        shrinkBox(D, &idx, &idy, 2*r_rim, 2*r_rim, sz_x);
    
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        idx-=r_rim;
        idy-=r_rim;
        shrinkBox(D, &idx, &idy, 2*r_rim, 2*r_rim, sz_x);
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        
        jointDetect(D, &idx, &idy, 2*r_rim, 2*r_rim, sz_x);
        
        id = idy*sz_x+idx;
        C[id]=1;
    }

    // check diagonally offset blocks
    idx = get_global_id(0);
    idy = get_global_id(1);
    
    idx = idx*sz_step + r_rim;
    idy = idy*sz_step + r_rim;
    
    if(checkRimCornerBool(D, idx, idy, sz_x, r_rim)){
        // find corner
        shrinkBox(D, &idx, &idy, 2*r_rim, 2*r_rim, sz_x);
        
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        
        idx-=r_rim;
        idy-=r_rim;
        shrinkBox(D, &idx, &idy, 2*r_rim, 2*r_rim, sz_x);
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        
        jointDetect(D, &idx, &idy, 2*r_rim, 2*r_rim, sz_x);
        
        id = idy*sz_x+idx;
        C[id]=1;
    }
}

void jointDetect(__global uchar* D, size_t *px, size_t *py, int w, int h, int sz_x){
    int x,y;
    x = *px;
    y = *py; 
    
    uchar c;
    int sz_win = 5; // it has to be <= r_rim
    int id1, id2;
    
    uchar grad[11][11];

    // vertical gradient
    for(int i=-sz_win;i<sz_win;i++)
    {
        for(int j=-sz_win;j<sz_win-1;j++){
            id1 = (y+j)*sz_x+(x+i);
            id2 = (y+j+1)*sz_x+(x+i);
            if(D[id1]!=D[id2]){
                grad[sz_win+j][sz_win+i]=1;
                grad[sz_win+j+1][sz_win+i]=1;
            }
        }
    }
    
    for(int i=-sz_win;i<sz_win-1;i++)
    {
        for(int j=-sz_win;j<sz_win;j++){
            if(grad[sz_win+j][sz_win+i]==1){
                id1 = (y+j)*sz_x+(x+i);
                id2 = (y+j)*sz_x+(x+i+1);
                if(D[id1]!=D[id2]){
                    grad[sz_win+j][sz_win+i]=2;
                    if(grad[sz_win+j][sz_win+i+1]==1)grad[sz_win+j][sz_win+i+1]=2;
                }
            }
        }
    }
    
    uchar col[4];
    col[1]=0;col[2]=0;col[3]=0;
    int i0,j0,flag;
    flag=0;
    for(int i=-sz_win;i<sz_win;i++)
    {
        for(int j=-sz_win;j<sz_win;j++){
            if(grad[sz_win+j][sz_win+i]==2){
                col[1]=0;col[2]=0;col[3]=0;
                col[D[(y+j-1)*sz_x+(x+i)]]++;
                col[D[(y+j+1)*sz_x+(x+i)]]++;
                col[D[(y+j)*sz_x+(x+i-1)]]++;
                col[D[(y+j)*sz_x+(x+i+1)]]++;
                col[D[(y+j)*sz_x+(x+i)]]++;
                if(col[1]>0 && col[2]>0 && col[3]>0){flag=1;i0=i;j0=j;break;}
            }
        }
        if(flag==1)break;
    }
    if(flag==1){
        *px=x+i0;
        *py=y+j0;
    }
}

void shrinkBox(__global uchar* D, size_t *px, size_t *py, int w, int h, int sz_x){
    uchar newLineCol[4];
    uchar rimColCount[4];
    uchar lineColCount[4][4];
    int x,y;
    x = *px;
    y = *py;
    
    int id, xt, yt;
    uchar c;
    uchar c_edge1, c_edge2, c_edge1_old, c_edge2_old;
    
    for(int i=0;i<4;i++)
    {
        newLineCol[i]=0;
        rimColCount[i]=0;
        for(int j=0;j<4;j++)
        {
            lineColCount[i][j]=0;
        }
    }
    
    // getting color counts for all sides
    // top & bot
    for(int i=0;i<w;i++)
    {
        int j=0;
        id = (y+j)*sz_x+(x+i);
        c=D[id];
        if(c>0){
            rimColCount[c]++;
            lineColCount[0][c]++;
        }
        j=h-1;
        id = (y+j)*sz_x+(x+i);
        c=D[id];
        if(c>0){
            rimColCount[c]++;
            lineColCount[1][c]++;
        }
    }
    // left & right
    for(int j=0;j<h;j++)
    {
        int i=0;
        id = (y+j)*sz_x+(x+i);
        c=D[id];
        if(c>0){
            if(j>0 && j<h-1)rimColCount[c]++;
            lineColCount[2][c]++;
        }
        i=w-1;
        id = (y+j)*sz_x+(x+i);
        c=D[id];
        if(c>0){
            if(j>0 && j<h-1)rimColCount[c]++;
            lineColCount[3][c]++;
        }
    }
    
    // shrinking - top bot left right
    int flag_shrink=1;
    while(flag_shrink==1){
        flag_shrink=0;
        
        if(h>1){ // top line
            // fill new line color
            for(int t=0;t<4;t++)newLineCol[t]=0;
            for(int i=0;i<w;i++)
            {
                int j=1;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                if(c>0)newLineCol[c]++;
                if(i==0)c_edge1=c;
                if(i==(w-1))c_edge2=c;
            }
            {
                int i=0;
                int j=0;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge1_old = c;
                i=w-1;
                j=0;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge2_old = c;
            }
        
            // check if removing line is ok
            int flag=1;
            for(int t=1;t<4;t++){
                if(newLineCol[t]==0 && lineColCount[0][t]>0 && rimColCount[t] == lineColCount[0][t]){
                    flag=0; // not ok to remove line
                    break;
                }
            }
            // remove line
            if(flag==1){ 
                for(int t=1;t<4;t++){
                    rimColCount[t] = rimColCount[t] - lineColCount[0][t] + newLineCol[t];
                    lineColCount[0][t]=newLineCol[t];
                }
                // removing boundary two points
                rimColCount[c_edge1]--;
                rimColCount[c_edge2]--;
                
                lineColCount[2][c_edge1_old]--;
                lineColCount[3][c_edge2_old]--;
                
                y++;
                h--;
                flag_shrink=1;
            }
        }// top line

        if(h>1){ // bot line
            // fill new line color
            for(int t=0;t<4;t++)newLineCol[t]=0;
            for(int i=0;i<w;i++)
            {
                int j=h-2;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                if(c>0)newLineCol[c]++;
                if(i==0)c_edge1=c;
                if(i==w-1)c_edge2=c;
            }
            {
                int i=0;
                int j=h-1;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge1_old = c;
                i=w-1;
                j=h-1;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge2_old = c;
            }
            
            // check if removing line is ok
            int flag=1;
            for(int t=1;t<4;t++){
                if(newLineCol[t]==0 && lineColCount[1][t]>0 && rimColCount[t] == lineColCount[1][t]){
                    flag=0;
                    break;
                }
            }
            // remove line
            if(flag==1){ 
                for(int t=1;t<4;t++){
                    rimColCount[t] = rimColCount[t] - lineColCount[1][t] + newLineCol[t];
                    lineColCount[1][t]=newLineCol[t];
                }
                // removing boundary two points
                rimColCount[c_edge1]--;
                rimColCount[c_edge2]--;
                
                lineColCount[2][c_edge1_old]--;
                lineColCount[3][c_edge2_old]--;
                
                h--;
                flag_shrink=1;
            }
        }// bot line
        
        if(w>1){ // left line
            // fill new line color
            for(int t=0;t<4;t++)newLineCol[t]=0;
            for(int j=0;j<h;j++)
            {
                int i=1;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                if(c>0)newLineCol[c]++;
                if(j==0)c_edge1=c;
                if(j==h-1)c_edge2=c;
            }
            {
                int i=0;
                int j=0;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge1_old = c;
                i=0;
                j=h-1;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge2_old = c;
            }
            
            // check if removing line is ok
            int flag=1;
            for(int t=1;t<4;t++){
                if(newLineCol[t]==0 && lineColCount[2][t]>0 && rimColCount[t] == lineColCount[2][t]){
                    flag=0;
                    break;
                }
            }
            // remove line
            if(flag==1){ 
                for(int t=1;t<4;t++){
                    rimColCount[t] = rimColCount[t] - lineColCount[2][t] + newLineCol[t];
                    lineColCount[2][t]=newLineCol[t];
                }
                // removing boundary two points
                rimColCount[c_edge1]--;
                rimColCount[c_edge2]--;

                lineColCount[0][c_edge1_old]--;
                lineColCount[1][c_edge2_old]--;
                
                x++;
                w--;
                flag_shrink=1;
            }
        }// left line
        
        if(w>1){ // right line
            // fill new line color
            for(int t=0;t<4;t++)newLineCol[t]=0;
            for(int j=0;j<h;j++)
            {
                int i=w-2;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                if(c>0)newLineCol[c]++;
                if(j==0)c_edge1=c;
                if(j==h-1)c_edge2=c;
            }
            {
                int i=w-1;
                int j=0;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge1_old = c;
                i=w-1;
                j=h-1;
                id = (y+j)*sz_x+(x+i);
                c=D[id];
                c_edge2_old = c;
            }
            
            // check if removing line is ok
            int flag=1;
            for(int t=1;t<4;t++){
                if(newLineCol[t]==0 && lineColCount[3][t]>0 && rimColCount[t] == lineColCount[3][t]){
                    flag=0;
                    break;
                }
            }
            // remove line
            if(flag==1){ 
                for(int t=1;t<4;t++){
                    rimColCount[t] = rimColCount[t] - lineColCount[3][t] + newLineCol[t];
                    lineColCount[3][t]=newLineCol[t];
                }
                // removing boundary two points
                rimColCount[c_edge1]--;
                rimColCount[c_edge2]--;

                lineColCount[0][c_edge1_old]--;
                lineColCount[1][c_edge2_old]--;
                
                w--;
                flag_shrink=1;
            }
        } // right line
        
    } // while
    
    // get center pixel
    x += floor(w/2.0);
    y += floor(h/2.0);
    
    *px = x;
    *py = y;
}

__kernel void readImage(__read_only image2d_t input_image, __global uchar* D){
    int2 pos;
    float4 pixelf;
    
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    pos.x = get_global_id(0);
    pos.y = get_global_id(1);

    int id = pos.y*sz_x + pos.x;
    
    pixelf = read_imagef(input_image, sampler, pos);
    
    uchar cr = floor(pixelf.x*255);
    uchar cg = floor(pixelf.y*255);
    uchar cb = floor(pixelf.z*255);
    
    D[id*3] = cr;
    D[id*3+1] = cg;
    D[id*3+2] = cb;
    
}

__kernel void getColorPurity(__read_only image2d_t input_image, __global uchar* P, int sz_blk, int skip)
{
    int2 pos;
    float4 pixelf;
    size_t idx, idy, id, id_blk;
    
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    idx = get_global_id(0);
    idy = get_global_id(1);
    
    id_blk = idy*sz_x+idx;

    idx *= sz_blk;
    idy *= sz_blk;
    
    sz_x *= sz_blk;
    sz_y *= sz_blk;
    
    float v_avg=0;
    int count=0;
    for(int i=0;i<sz_blk;i+=skip){
        for(int j=0;j<sz_blk;j+=skip){
            pos.x = idx+i;
            pos.y = idy+j;
            pixelf = read_imagef(input_image, sampler, pos);
            
            float gr = (pixelf.x + pixelf.y + pixelf.z)/3;
            
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

    v_avg /= count;
    
    if(v_avg>0.3){
        P[id_blk]=1;
        }else{
        P[id_blk]=0;
    }
}

__kernel void colConversionGL(__read_only image2d_t input_image, __global uchar* D, int N)
{
    int2 pos;
    float4 pixelf;
    
    int R = 1, G=2, B=3;

    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    pos.x = get_global_id(0);
    pos.y = get_global_id(1);

    int id = pos.y*sz_x + pos.x;
    
    pixelf = read_imagef(input_image, sampler, pos);
    
    float cr=pixelf.x;
    float cg=pixelf.y;
    float cb=pixelf.z;

    if (id >= N) return;

    if ( (cr + cg + cb) < 0.50){ // 0.78
        D[id] = 0; 
    }
    else{
        if (cr > cg){
            if(cr > cb){ 
                D[id] = R;
            }else{ 
                D[id] = B;
            }
        }
        else{
            if(cg > cb){ 
                D[id] = G;
            }else{ 
                D[id] = B;
            }
        }
    }
}

__kernel void copyInGL(__read_only image2d_t input_image, __write_only image2d_t output_image){
    int2 pos;
    float4 pixelf;
    
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    pos.x = get_global_id(0);
    pos.y = get_global_id(1);

    int id = pos.y*sz_x + pos.x;
    pixelf = read_imagef(input_image, sampler, pos);
    write_imagef(output_image, pos, pixelf);
}

__kernel void resetNCorners(__global int *nPts)
{
    *nPts=0;
}

__kernel void getCorners(__global float *X, __global float *Y, __global uchar *C, __global int *nPts){
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t idx = get_global_id(0);
    size_t idy = get_global_id(1); 
    
    int id = idy*sz_x + idx;
    
    int numPts_old=0;
    if(C[id]==1){
        numPts_old = atomic_inc(nPts);
         X[numPts_old]=idx;
         Y[numPts_old]=idy;
    }
}

 
__kernel void getNCorners(__global uchar *C, __global int *nPts){
    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t idx = get_global_id(0);
    size_t idy = get_global_id(1); 
    
    int id = idy*sz_x + idx;
    
    if(C[id]==1){
         atomic_inc(nPts);
    }
}

__kernel void init1dArray(__global int*arr, int val){ 
    size_t id = get_global_id(0);
    
    arr[id] = val;
}




__kernel void colConversion(__global int* D, int N)
{
    int R = 1, G=2, B=3;

    size_t sz_x = get_global_size(0);
    size_t sz_y = get_global_size(1);
    size_t idx = get_global_id(0);
    size_t idy = get_global_id(1);

    int id = idy*sz_x + idx;

    if (id >= N) return;

    int c = D[id];
    uchar cr = ((c >> 16) & 0xff);
    uchar cg = ((c >> 8) & 0xff);
    uchar cb = (c & 0xff);

    if ( ((int)cr + (int)cg + (int)cb) < 200){ 
        D[id] = 0; 
    }
    else{
        if (cr > cg){
            if(cr > cb){ 
                D[id] = R;
            }else{ 
                D[id] = B;
            }
        }
        else{
            if(cg > cb){ 
                D[id] = G;
            }else{ 
                D[id] = B;
            }
        }
    }
}

