#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 1, local_size_y = 1) in;
layout(std430, binding = 2) buffer ptsVote_ssbo {int ptsVote[];};
layout(std430, binding = 3) buffer D_ssbo {int D[];};
layout(std430, binding = 4) buffer lineEqns_ssbo {float lineEqns[];};
layout(std430, binding = 5) buffer x_ssbo {float x[];};
layout(std430, binding = 6) buffer y_ssbo {float y[];};
layout(std430, binding = 7) buffer inVals_ssbo {int inVals[];};

int getColVal_colMajor(int x, int y, int w, int h);
void binarySearch(int x1, int x2, int y1, int y2, inout int x_ret, inout int y_ret, int w, int h);
void main()
{	
	int w_img, h_img;
	w_img = inVals[0];
	h_img = inVals[1];
	
    int debug=1;
	
	int nPts = int(gl_NumWorkGroups.x);
	
    int p0 = int(gl_GlobalInvocationID.x);
    int p1 = int(gl_GlobalInvocationID.y);
	
    int deg_eq=27;
    
    if(debug==1){
        for(int i=0;i<deg_eq;i++){
            lineEqns[(p0*nPts+p1)*deg_eq+i]=0.0;
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
    if(d<=5.0)return;
	
	float th_dist = d/10.0;
	if (th_dist < 5.0)th_dist = 5.0;
    
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
    xm[2] = x1 + 3.0*d_pts/4.0*(-b);
    ym[2] = y1 + 3.0*d_pts/4.0*(a);
    xm[3] = x1 + 5.0*d_pts/4.0*(-b);
    ym[3] = y1 + 5.0*d_pts/4.0*(a);

    // get points and colors just top and bottom of the line
    for(int i=0;i<4;i++){
        xmt[i] = int(floor(xm[i] + 5.0*a));
        ymt[i] = int(floor(ym[i] + 5.0*b));
        
        xmb[i] = int(floor(xm[i] - 5.0*a));
        ymb[i] = int(floor(ym[i] - 5.0*b));
    }

    int ct[4],cb[4];
    for(int i=0;i<4;i++){
        ct[i] = getColVal_colMajor(xmt[i], ymt[i], w_img, h_img);
        cb[i] = getColVal_colMajor(xmb[i], ymb[i], w_img, h_img);
    }
    
    for(int i=0;i<4;i++)
    {
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+0 ] = float(xmt[i]);
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+1 ] = float(ymt[i]);
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+2 ] = float(xmb[i]);
        lineEqns[ (p0*nPts+p1)*deg_eq+i*4+3 ] = float(ymb[i]);
        lineEqns[ (p0*nPts+p1)*deg_eq+16+2*i ] = float(ct[i]);
        lineEqns[ (p0*nPts+p1)*deg_eq+16+2*i+1 ] = float(cb[i]);
        }
    }
    
    if(ct[1]==0 || cb[1]==0 || ct[2]==0 || cb[2]==0){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 0.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;
        }
        return;
    }
    if(ct[0]!=0 && cb[0]!=0 && ct[0]==cb[0]){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 1.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;
        }
        return;
    }
    if(ct[3]!=0 && cb[3]!=0 && ct[3]==cb[3]){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 2.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;
        }
        return;
    }
    if(ct[1]==cb[1] || ct[2]==cb[2] || ct[1]==ct[2] || cb[1]==cb[2]){
        if(debug==1){
        lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 3.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;
        lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;
        }
        return;
    }
     
    // find the line ID(s)
    // colors of 4x4 grid pattern
    int colset[4][4];
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
    
    int linet[6][4];
    int lineb[6][4];
    
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
            int px1_mid,px2_mid,py1_mid,py2_mid;
            px1_mid = x1_mid;
            px2_mid = x2_mid;
            py1_mid = y1_mid;
            py2_mid = y2_mid;

            binarySearch(xmt[1], xmt[2], ymt[1], ymt[2], px1_mid, py1_mid, w_img, h_img);
            binarySearch(xmb[1], xmb[2], ymb[1], ymb[2], px2_mid, py2_mid, w_img, h_img);

            x1_mid = px1_mid;
            x2_mid = px2_mid;
            y1_mid = py1_mid;
            y2_mid = py2_mid;
            
            // find intersection point - alpha is described in definition of x_mid and y_mid - put these values in eq of the line and find the value of alpha
            float alpha = (a*float(x2_mid)+b*float(y2_mid)+c)/(a*float(x2_mid)+b*float(y2_mid)-a*float(x1_mid)-b*float(y1_mid));
            int x_mid = int(floor(float(x1_mid)+alpha*float(x2_mid-x1_mid)));
            int y_mid = int(floor(float(y1_mid)+alpha*float(y2_mid-y1_mid)));
            
            if(debug==1){
            lineEqns[ (p0*nPts+p1)*deg_eq+16 ] = float(p0);
            lineEqns[ (p0*nPts+p1)*deg_eq+17 ] = float(p1);
            lineEqns[ (p0*nPts+p1)*deg_eq+18 ] = x1;
            lineEqns[ (p0*nPts+p1)*deg_eq+19 ] = y1;
            lineEqns[ (p0*nPts+p1)*deg_eq+20 ] = x2;
            lineEqns[ (p0*nPts+p1)*deg_eq+21 ] = y2;
            lineEqns[ (p0*nPts+p1)*deg_eq+22 ] = float(x_mid);
            lineEqns[ (p0*nPts+p1)*deg_eq+23 ] = float(y_mid); 
            lineEqns[ (p0*nPts+p1)*deg_eq+24 ] = float(i);
            lineEqns[ (p0*nPts+p1)*deg_eq+25 ] = float(flag);
            lineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 2.0;
            }
            
            // find all points near mid
			int id_pt0 = -1;
			int id_pt1 = -1;
			int id_pt2 = -1;
            for(int t=0;t<nPts;t++){
                if(abs(x[t]-float(x_mid))<th_dist && abs(y[t]-float(y_mid))<th_dist){
                    atomicAdd(ptsVote[t*9+linePts[i][1]],1);
					id_pt1 = t;
                }
                if(abs(x[t]-float(x1))<th_dist && abs(y[t]-float(y1))<th_dist){
                    if(flag==1){
                        atomicAdd(ptsVote[t*9+linePts[i][0]],1);
						id_pt0 = t;
                        }else{
                        atomicAdd(ptsVote[t*9+linePts[i][2]],1);
						id_pt2 = t;
                    }
                }
                if(abs(x[t]-float(x2))<th_dist && abs(y[t]-float(y2))<th_dist){
                    if(flag==1){
                        atomicAdd(ptsVote[t*9+linePts[i][2]],1);
						id_pt2 = t;
                        }else{
                        atomicAdd(ptsVote[t*9+linePts[i][0]],1);
						id_pt0 = t;
                    }
                }
            }
			
			if(id_pt1 != -1){
				for(int t=0;t<nPts;t++){
					if(abs(x[t]-float(x_mid))<th_dist && abs(y[t]-float(y_mid))<th_dist){
						atomicAdd(ptsVote[t*9+linePts[i][1]],1);
					}
					if(abs(x[t]-float(x1))<th_dist && abs(y[t]-float(y1))<th_dist){
						if(flag==1){
							atomicAdd(ptsVote[t*9+linePts[i][0]],1);
							}else{
							atomicAdd(ptsVote[t*9+linePts[i][2]],1);
						}
					}
					if(abs(x[t]-float(x2))<th_dist && abs(y[t]-float(y2))<th_dist){
						if(flag==1){
							atomicAdd(ptsVote[t*9+linePts[i][2]],1);
							}else{
							atomicAdd(ptsVote[t*9+linePts[i][0]],1);
						}
					}
				}	
			}
        }
    }
}

void binarySearch(int x1, int x2, int y1, int y2, inout int x_ret, inout int y_ret, int w, int h){
    int x_mid = (x1+x2)/2;
    int y_mid = (y1+y2)/2;
        
    int id1 = x1*h + y1;
    int id2 = x2*h + y2;
    
    int c1 = D[id1];
    int c2 = D[id2];
        
    int id_mid = x_mid*h + y_mid;
    int c_mid = D[id_mid];
    
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
        if(c_mid==0) {x_ret=-1; y_ret=-1;return;}
        if(c_mid!=c1 && c_mid!=c2 && c_mid!=0){x_ret=x_mid; y_ret=y_mid; return;}
        x_mid = (x1+x2)/2;
        y_mid = (y1+y2)/2;
        id_mid = x_mid*h + y_mid;
        c_mid = D[id_mid];
    }
    x_ret=x_mid; y_ret=y_mid;
}

int getColVal_colMajor(int x_this, int y_this, int w, int h)
{
    int c=4;
    if(x_this>=0 && x_this<w && y_this>=0 && y_this<h)
    {
        int id = x_this*h + y_this;
        c = D[id];
    }
    return c;
}
