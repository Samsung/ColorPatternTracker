static const char *getLinePtAssignment_kernel =
"#version 310 es\n"
"\n"
"#extension GL_ANDROID_extension_pack_es31a : require\n"
"layout(local_size_x = 1, local_size_y = 1) in;\n"
"layout(std430, binding = 2) buffer ptsVote_ssbo {int ptsVote[];};\n"
"layout(std430, binding = 3) buffer D_ssbo {int D[];};\n"
"layout(std430, binding = 4) buffer lineEqns_ssbo {float lineEqns[];};\n"
"layout(std430, binding = 5) buffer x_ssbo {float x[];};\n"
"layout(std430, binding = 6) buffer y_ssbo {float y[];};\n"
"layout(std430, binding = 7) buffer inVals_ssbo {int inVals[];};\n"
"\n"
"int getColVal_colMajor(int x, int y, int w, int h);\n"
"void binarySearch(int x1, int x2, int y1, int y2, inout int x_ret, inout int y_ret, int w, int h);\n"
"void main()\n"
"{	\n"
"	int w_img, h_img;\n"
"	w_img = inVals[0];\n"
"	h_img = inVals[1];\n"
"	\n"
"\t\tint debug=1;\n"
"	\n"
"	int nPts = int(gl_NumWorkGroups.x);\n"
"	\n"
"\t\tint p0 = int(gl_GlobalInvocationID.x);\n"
"\t\tint p1 = int(gl_GlobalInvocationID.y);\n"
"	\n"
"\t\tint deg_eq=27;\n"
"\t\t\n"
"\t\tif(debug==1){\n"
"\t\t\t\tfor(int i=0;i<deg_eq;i++){\n"
"\t\t\t\t\t\tlineEqns[(p0*nPts+p1)*deg_eq+i]=0.0;\n"
"\t\t\t\t}\n"
"\t\t}\n"
"\n"
"\t\tif(p1 <= p0)return;\n"
"\t\t\n"
"\t\t// getting the line equation\n"
"\t\tfloat x1 = x[p0];\n"
"\t\tfloat y1 = y[p0];\n"
"\t\tfloat x2 = x[p1];\n"
"\t\tfloat y2 = y[p1];\n"
"\t\t\n"
"\t\tfloat a = (y2 - y1);\n"
"\t\tfloat b = (x1 - x2);\n"
"\t\tfloat c = (x2*y1 - x1*y2);\t\t\n"
"\t\tfloat d = sqrt(a*a+b*b);\n"
"\n"
"\t\t// if points are too close, they may represent the same point\n"
"\t\tif(d<=5.0)return;\n"
"	\n"
"	float th_dist = d/10.0;\n"
"	if (th_dist < 5.0)th_dist = 5.0;\n"
"\t\t\n"
"\t\ta /= d;\n"
"\t\tb /= d;\n"
"\t\tc /= d;\n"
"\t\t\n"
"\t\tfloat d_pts = d;//(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);\n"
"\t\t\n"
"\t\t// direction of line is (-b, a) from p0 to p1\n"
"\t\t// get points on line at 4 equidistant places spread across the two points\n"
"\t\tfloat xm[4];\n"
"\t\tfloat ym[4]; \n"
"\t\tint xmt[4], ymt[4], xmb[4], ymb[4];\n"
"\t\txm[0] = x1 - d_pts/4.0*(-b);\n"
"\t\tym[0] = y1 - d_pts/4.0*(a);\n"
"\t\txm[1] = x1 + d_pts/4.0*(-b);\n"
"\t\tym[1] = y1 + d_pts/4.0*(a);\n"
"\t\txm[2] = x1 + 3.0*d_pts/4.0*(-b);\n"
"\t\tym[2] = y1 + 3.0*d_pts/4.0*(a);\n"
"\t\txm[3] = x1 + 5.0*d_pts/4.0*(-b);\n"
"\t\tym[3] = y1 + 5.0*d_pts/4.0*(a);\n"
"\n"
"\t\t// get points and colors just top and bottom of the line\n"
"\t\tfor(int i=0;i<4;i++){\n"
"\t\t\t\txmt[i] = int(floor(xm[i] + 5.0*a));\n"
"\t\t\t\tymt[i] = int(floor(ym[i] + 5.0*b));\n"
"\t\t\t\t\n"
"\t\t\t\txmb[i] = int(floor(xm[i] - 5.0*a));\n"
"\t\t\t\tymb[i] = int(floor(ym[i] - 5.0*b));\n"
"\t\t}\n"
"\n"
"\t\tint ct[4],cb[4];\n"
"\t\tfor(int i=0;i<4;i++){\n"
"\t\t\t\tct[i] = getColVal_colMajor(xmt[i], ymt[i], w_img, h_img);\n"
"\t\t\t\tcb[i] = getColVal_colMajor(xmb[i], ymb[i], w_img, h_img);\n"
"\t\t}\n"
"\t\t\n"
"\t\tfor(int i=0;i<4;i++)\n"
"\t\t{\n"
"\t\t\t\tif(debug==1){\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+i*4+0 ] = float(xmt[i]);\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+i*4+1 ] = float(ymt[i]);\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+i*4+2 ] = float(xmb[i]);\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+i*4+3 ] = float(ymb[i]);\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+16+2*i ] = float(ct[i]);\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+16+2*i+1 ] = float(cb[i]);\n"
"\t\t\t\t}\n"
"\t\t}\n"
"\t\t\n"
"\t\tif(ct[1]==0 || cb[1]==0 || ct[2]==0 || cb[2]==0){\n"
"\t\t\t\tif(debug==1){\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 0.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;\n"
"\t\t\t\t}\n"
"\t\t\t\treturn;\n"
"\t\t}\n"
"\t\tif(ct[0]!=0 && cb[0]!=0 && ct[0]==cb[0]){\n"
"\t\t\t\tif(debug==1){\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 1.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;\n"
"\t\t\t\t}\n"
"\t\t\t\treturn;\n"
"\t\t}\n"
"\t\tif(ct[3]!=0 && cb[3]!=0 && ct[3]==cb[3]){\n"
"\t\t\t\tif(debug==1){\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 2.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;\n"
"\t\t\t\t}\n"
"\t\t\t\treturn;\n"
"\t\t}\n"
"\t\tif(ct[1]==cb[1] || ct[2]==cb[2] || ct[1]==ct[2] || cb[1]==cb[2]){\n"
"\t\t\t\tif(debug==1){\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+24 ] = 3.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+25 ] = 0.0;\n"
"\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 1.0;\n"
"\t\t\t\t}\n"
"\t\t\t\treturn;\n"
"\t\t}\n"
"\t\t \n"
"\t\t// find the line ID(s)\n"
"\t\t// colors of 4x4 grid pattern\n"
"\t\tint colset[4][4];\n"
"\t\t/*\n"
"\t\tcolset[0][0] = 1;\n"
"\t\tcolset[0][1] = 2;\n"
"\t\tcolset[0][2] = 1;\n"
"\t\tcolset[0][3] = 3;\n"
"\n"
"\t\tcolset[1][0] = 2;\n"
"\t\tcolset[1][1] = 3;\n"
"\t\tcolset[1][2] = 2;\n"
"\t\tcolset[1][3] = 1;\n"
"\n"
"\t\tcolset[2][0] = 1;\n"
"\t\tcolset[2][1] = 2;\n"
"\t\tcolset[2][2] = 1;\n"
"\t\tcolset[2][3] = 3;\n"
"\n"
"\t\tcolset[3][0] = 2;\n"
"\t\tcolset[3][1] = 3;\n"
"\t\tcolset[3][2] = 2;\n"
"\t\tcolset[3][3] = 1;\n"
"*/\n"
"\t\tcolset[0][0] = 1;\n"
"\t\tcolset[0][1] = 2;\n"
"\t\tcolset[0][2] = 3;\n"
"\t\tcolset[0][3] = 1;\n"
"\n"
"\t\tcolset[1][0] = 3;\n"
"\t\tcolset[1][1] = 1;\n"
"\t\tcolset[1][2] = 2;\n"
"\t\tcolset[1][3] = 3;\n"
"\n"
"\t\tcolset[2][0] = 1;\n"
"\t\tcolset[2][1] = 2;\n"
"\t\tcolset[2][2] = 3;\n"
"\t\tcolset[2][3] = 1;\n"
"\n"
"\t\tcolset[3][0] = 2;\n"
"\t\tcolset[3][1] = 3;\n"
"\t\tcolset[3][2] = 1;\n"
"\t\tcolset[3][3] = 2;\n"
"\t\t\n"
"\t\tint linet[6][4];\n"
"\t\tint lineb[6][4];\n"
"\t\t\n"
"\t\t// colors for top and bottom of each of the horizontal and vertical lines\n"
"\t\tfor(int i=0;i<3;i++){\n"
"\t\t\t\tfor(int j=0;j<4;j++){\n"
"\t\t\t\t\t\t// horizontal lines\n"
"\t\t\t\t\t\tlinet[i][j]=colset[i][j];\n"
"\t\t\t\t\t\tlineb[i][j]=colset[i+1][j];\n"
"\t\t\t\t\t\t\n"
"\t\t\t\t\t\t// vertical lines\n"
"\t\t\t\t\t\tlinet[3+i][j]=colset[j][i+1];\n"
"\t\t\t\t\t\tlineb[3+i][j]=colset[j][i];\n"
"\t\t\t\t}\n"
"\t\t}\n"
"\t \n"
"\t\tint linePts[6][3];\n"
"\n"
"\t\tfor(int i=0;i<3;i++){\n"
"\t\t\t\tfor(int j=0;j<3;j++){\n"
"\t\t\t\t\t\tlinePts[i][j] = j*3+i;\n"
"\t\t\t\t\t\tlinePts[3+i][j] = j+3*i;\n"
"\t\t\t\t}\n"
"\t\t}\n"
"\t\t\n"
"\t\t// check lines\n"
"\t\tfor(int i=0;i<6;i++)\n"
"\t\t{\n"
"\t\t\t\tint flag=1;\n"
"\t\t\t\tfor(int j=0;j<4;j++){\n"
"\t\t\t\t\t\tif(ct[j]!=0 && cb[j]!=0 && (ct[j]!=linet[i][j] || cb[j]!=lineb[i][j]) ){\n"
"\t\t\t\t\t\t\t\tflag=0;\n"
"\t\t\t\t\t\t\t\tbreak;\n"
"\t\t\t\t\t\t}\n"
"\t\t\t\t}\n"
"\t\t\t\tif(flag==0){ // reverse lin\n"
"\t\t\t\t\t\tflag=2;\n"
"\t\t\t\t\t\tfor(int j=0;j<4;j++){\n"
"\t\t\t\t\t\t\t\tif(ct[j]!=0 && cb[j]!=0 && (ct[j]!=lineb[i][3-j] || cb[j]!=linet[i][3-j]) ){\n"
"\t\t\t\t\t\t\t\t\t\tflag=0;\n"
"\t\t\t\t\t\t\t\t\t\tbreak;\n"
"\t\t\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t}\n"
"\t\t\t\t}\n"
"\t\t\t\t\n"
"\t\t\t\t// found matching line\n"
"\t\t\t\tif(flag>0){\t\t\t\t\t\t\n"
"\t\t\t\t\t\t// find middle point\n"
"\t\t\t\t\t\t// binary search for crossing\n"
"\t\t\t\t\t\tint x1_mid,x2_mid,y1_mid,y2_mid;\n"
"\t\t\t\t\t\tint px1_mid,px2_mid,py1_mid,py2_mid;\n"
"\t\t\t\t\t\tpx1_mid = x1_mid;\n"
"\t\t\t\t\t\tpx2_mid = x2_mid;\n"
"\t\t\t\t\t\tpy1_mid = y1_mid;\n"
"\t\t\t\t\t\tpy2_mid = y2_mid;\n"
"\n"
"\t\t\t\t\t\tbinarySearch(xmt[1], xmt[2], ymt[1], ymt[2], px1_mid, py1_mid, w_img, h_img);\n"
"\t\t\t\t\t\tbinarySearch(xmb[1], xmb[2], ymb[1], ymb[2], px2_mid, py2_mid, w_img, h_img);\n"
"\n"
"\t\t\t\t\t\tx1_mid = px1_mid;\n"
"\t\t\t\t\t\tx2_mid = px2_mid;\n"
"\t\t\t\t\t\ty1_mid = py1_mid;\n"
"\t\t\t\t\t\ty2_mid = py2_mid;\n"
"\t\t\t\t\t\t\n"
"\t\t\t\t\t\t// find intersection point - alpha is described in definition of x_mid and y_mid - put these values in eq of the line and find the value of alpha\n"
"\t\t\t\t\t\tfloat alpha = (a*float(x2_mid)+b*float(y2_mid)+c)/(a*float(x2_mid)+b*float(y2_mid)-a*float(x1_mid)-b*float(y1_mid));\n"
"\t\t\t\t\t\tint x_mid = int(floor(float(x1_mid)+alpha*float(x2_mid-x1_mid)));\n"
"\t\t\t\t\t\tint y_mid = int(floor(float(y1_mid)+alpha*float(y2_mid-y1_mid)));\n"
"\t\t\t\t\t\t\n"
"\t\t\t\t\t\tif(debug==1){\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+16 ] = float(p0);\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+17 ] = float(p1);\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+18 ] = x1;\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+19 ] = y1;\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+20 ] = x2;\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+21 ] = y2;\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+22 ] = float(x_mid);\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+23 ] = float(y_mid); \n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+24 ] = float(i);\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+25 ] = float(flag);\n"
"\t\t\t\t\t\tlineEqns[ (p0*nPts+p1)*deg_eq+26 ] = 2.0;\n"
"\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t\n"
"\t\t\t\t\t\t// find all points near mid\n"
"			int id_pt0 = -1;\n"
"			int id_pt1 = -1;\n"
"			int id_pt2 = -1;\n"
"\t\t\t\t\t\tfor(int t=0;t<nPts;t++){\n"
"\t\t\t\t\t\t\t\tif(abs(x[t]-float(x_mid))<th_dist && abs(y[t]-float(y_mid))<th_dist){\n"
"\t\t\t\t\t\t\t\t\t\tatomicAdd(ptsVote[t*9+linePts[i][1]],1);\n"
"					id_pt1 = t;\n"
"\t\t\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t\t\tif(abs(x[t]-float(x1))<th_dist && abs(y[t]-float(y1))<th_dist){\n"
"\t\t\t\t\t\t\t\t\t\tif(flag==1){\n"
"\t\t\t\t\t\t\t\t\t\t\t\tatomicAdd(ptsVote[t*9+linePts[i][0]],1);\n"
"						id_pt0 = t;\n"
"\t\t\t\t\t\t\t\t\t\t\t\t}else{\n"
"\t\t\t\t\t\t\t\t\t\t\t\tatomicAdd(ptsVote[t*9+linePts[i][2]],1);\n"
"						id_pt2 = t;\n"
"\t\t\t\t\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t\t\tif(abs(x[t]-float(x2))<th_dist && abs(y[t]-float(y2))<th_dist){\n"
"\t\t\t\t\t\t\t\t\t\tif(flag==1){\n"
"\t\t\t\t\t\t\t\t\t\t\t\tatomicAdd(ptsVote[t*9+linePts[i][2]],1);\n"
"						id_pt2 = t;\n"
"\t\t\t\t\t\t\t\t\t\t\t\t}else{\n"
"\t\t\t\t\t\t\t\t\t\t\t\tatomicAdd(ptsVote[t*9+linePts[i][0]],1);\n"
"						id_pt0 = t;\n"
"\t\t\t\t\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t\t\t}\n"
"\t\t\t\t\t\t}\n"
"			\n"
"			if(id_pt1 != -1){\n"
"				for(int t=0;t<nPts;t++){\n"
"					if(abs(x[t]-float(x_mid))<th_dist && abs(y[t]-float(y_mid))<th_dist){\n"
"						atomicAdd(ptsVote[t*9+linePts[i][1]],1);\n"
"					}\n"
"					if(abs(x[t]-float(x1))<th_dist && abs(y[t]-float(y1))<th_dist){\n"
"						if(flag==1){\n"
"							atomicAdd(ptsVote[t*9+linePts[i][0]],1);\n"
"							}else{\n"
"							atomicAdd(ptsVote[t*9+linePts[i][2]],1);\n"
"						}\n"
"					}\n"
"					if(abs(x[t]-float(x2))<th_dist && abs(y[t]-float(y2))<th_dist){\n"
"						if(flag==1){\n"
"							atomicAdd(ptsVote[t*9+linePts[i][2]],1);\n"
"							}else{\n"
"							atomicAdd(ptsVote[t*9+linePts[i][0]],1);\n"
"						}\n"
"					}\n"
"				}	\n"
"			}\n"
"\t\t\t\t}\n"
"\t\t}\n"
"}\n"
"\n"
"void binarySearch(int x1, int x2, int y1, int y2, inout int x_ret, inout int y_ret, int w, int h){\n"
"\t\tint x_mid = (x1+x2)/2;\n"
"\t\tint y_mid = (y1+y2)/2;\n"
"\t\t\t\t\n"
"\t\tint id1 = x1*h + y1;\n"
"\t\tint id2 = x2*h + y2;\n"
"\t\t\n"
"\t\tint c1 = D[id1];\n"
"\t\tint c2 = D[id2];\n"
"\t\t\t\t\n"
"\t\tint id_mid = x_mid*h + y_mid;\n"
"\t\tint c_mid = D[id_mid];\n"
"\t\t\n"
"\t\twhile( abs(x1-x2) + abs(y1-y2) >2 ){\n"
"\t\t\t\tif(c_mid==c1){\n"
"\t\t\t\t\t\tx1=x_mid;\n"
"\t\t\t\t\t\ty1=y_mid;\n"
"\t\t\t\t\t\tid1 = x1*h + y1;\n"
"\t\t\t\t}\n"
"\t\t\t\tif(c_mid==c2){\n"
"\t\t\t\t\t\tx2=x_mid;\n"
"\t\t\t\t\t\ty2=y_mid;\n"
"\t\t\t\t\t\tid2 = x2*h + y2;\n"
"\t\t\t\t}\n"
"\t\t\t\tif(c_mid==0) {x_ret=-1; y_ret=-1;return;}\n"
"\t\t\t\tif(c_mid!=c1 && c_mid!=c2 && c_mid!=0){x_ret=x_mid; y_ret=y_mid; return;}\n"
"\t\t\t\tx_mid = (x1+x2)/2;\n"
"\t\t\t\ty_mid = (y1+y2)/2;\n"
"\t\t\t\tid_mid = x_mid*h + y_mid;\n"
"\t\t\t\tc_mid = D[id_mid];\n"
"\t\t}\n"
"\t\tx_ret=x_mid; y_ret=y_mid;\n"
"}\n"
"\n"
"int getColVal_colMajor(int x_this, int y_this, int w, int h)\n"
"{\n"
"\t\tint c=4;\n"
"\t\tif(x_this>=0 && x_this<w && y_this>=0 && y_this<h)\n"
"\t\t{\n"
"\t\t\t\tint id = x_this*h + y_this;\n"
"\t\t\t\tc = D[id];\n"
"\t\t}\n"
"\t\treturn c;\n"
"}\n"
;
