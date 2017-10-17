#version 310 es

#extension GL_ANDROID_extension_pack_es31a : require
layout(local_size_x = 4, local_size_y = 2) in;
layout(std430, binding = 2) buffer D_ssbo {int D[];};
layout(std430, binding = 3) buffer C_ssbo {int C[];};
layout(std430, binding = 4) buffer purity_ssbo {int purity[];};
layout(std430, binding = 5) buffer pSz_step_ssbo {int pSz_step[];};

void jointDetect(inout int px, inout int py, int w, int h, int sz_x);
bool checkRimCornerBool(int idx, int idy, int sz_x, int r_rim);
void checkRimCorner(int idx, int idy, int sz_x, int r_rim, inout int pcol1, inout int pcol2, inout int pcol3);
void shrinkBox(inout int px, inout int py, int w, int h, int sz_x);
void jointDetect(inout int px, inout int py, int w, int h, int sz_x);

void main()
{	
	int sz_x = int((gl_NumWorkGroups.x*gl_WorkGroupSize.x));
    int sz_y = int((gl_NumWorkGroups.y*gl_WorkGroupSize.y));
	
    int idx = int(gl_GlobalInvocationID.x);
    int idy = int(gl_GlobalInvocationID.y);
	
	int sz_step = pSz_step[0];

	if(idx>=sz_x-5 || idy>=sz_y-5) return;
    
    int id, idbx, idby;
    int colID;
    
    id = idy*sz_x+idx;
    if(purity[id]==0) return;
    
    idx *= sz_step;
    idy *= sz_step;
    sz_x *= sz_step;
    sz_y *= sz_step;
    
    int r_rim=7;
	

    if(checkRimCornerBool(idx, idy, sz_x, r_rim)){
        // find corner
        shrinkBox( idx, idy, 2*r_rim, 2*r_rim, sz_x);
    
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        idx-=r_rim;
        idy-=r_rim;
        shrinkBox(idx, idy, 2*r_rim, 2*r_rim, sz_x);
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        
        jointDetect(idx, idy, 2*r_rim, 2*r_rim, sz_x);
        
        id = idy*sz_x+idx;
        C[id]=1;
    }

    // check diagonally offset blocks
    idx = int(gl_GlobalInvocationID.x);
    idy = int(gl_GlobalInvocationID.y);
    
    idx = idx*sz_step + r_rim;
    idy = idy*sz_step + r_rim;
    
    if(checkRimCornerBool(idx, idy, sz_x, r_rim)){
        // find corner
        shrinkBox(idx, idy, 2*r_rim, 2*r_rim, sz_x);
        
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        
        idx-=r_rim;
        idy-=r_rim;
        shrinkBox(idx, idy, 2*r_rim, 2*r_rim, sz_x);
        if(idx<r_rim || idx>=sz_x-r_rim || idy<r_rim || idy>=sz_y-r_rim)return;
        
        jointDetect(idx, idy, 2*r_rim, 2*r_rim, sz_x);
        
        id = idy*sz_x+idx;
        C[id]=1;
    }
}

bool checkRimCornerBool(int idx, int idy, int sz_x, int r_rim)
{
    int col1, col2, col3;
    col1=0;
    col2=0;
    col3=0;
    
    checkRimCorner(idx, idy, sz_x, r_rim, col1, col2, col3);
    
    if((col1>0 && col2>0) && col3>0){return true; }
    
    return false;
}

void checkRimCorner(int idx, int idy, int sz_x, int r_rim, inout int pcol1, inout int pcol2, inout int pcol3)
{
    int id, idbx, idby;
    int c;
    for(int i=0;i<2*r_rim;i++){
        idbx = idx+i;
        idby = idy;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)pcol1=1;
        if(c==2)pcol2=1;
        if(c==3)pcol3=1;

        idbx = idx+i;
        idby = idy+2*r_rim-1;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)pcol1=1;
        if(c==2)pcol2=1;
        if(c==3)pcol3=1;
    }

    for(int i=1;i<2*r_rim-1;i++){
        idbx = idx;
        idby = idy+i;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)pcol1=1;
        if(c==2)pcol2=1;
        if(c==3)pcol3=1;

        idbx = idx+2*r_rim-1;
        idby = idy+i;
        
        id = idby*sz_x+idbx;
        c=D[id];
        if(c==1)pcol1=1;
        if(c==2)pcol2=1;
        if(c==3)pcol3=1;
    }
}

void shrinkBox(inout int px, inout int py, int w, int h, int sz_x){
    int newLineCol[4];
    int rimColCount[4];
    int lineColCount[4][4];
    int x,y;
    x = px;
    y = py;
    
    int id, xt, yt;
    int c;
    int c_edge1, c_edge2, c_edge1_old, c_edge2_old;
    
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
    x += w/2;
    y += h/2;
    
    px = x;
    py = y;
}

void jointDetect(inout int px, inout int py, int w, int h, int sz_x){
    int x,y;
    x = px;
    y = py; 
    
    int c;
    int sz_win = 5; // it has to be <= r_rim
    int id1, id2;
    
    int grad[11][11];

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
    
    int col[4];
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
        px=x+i0;
        py=y+j0;
    }
}
