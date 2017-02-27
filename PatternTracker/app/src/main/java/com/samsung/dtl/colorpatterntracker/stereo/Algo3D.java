package com.samsung.dtl.colorpatterntracker.stereo;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;
import org.opencv.core.Scalar;

import com.samsung.dtl.colorpatterntracker.util.TUtil;

// TODO: Auto-generated Javadoc
/**
 * The Class Algo3D.
 */
public class Algo3D {
    
    /**
     * Gets the Homography from point correspondences.
     *
     * @param p the points in first plane
     * @param q the points in second plane
     * @return the Homography matrix
     */
    public static Mat getHFromCorres(Mat p, Mat q){
    	Mat B=Mat.zeros(p.rows()*2, 9, CvType.CV_32FC1);
    	
    	float [] B2vec = new float[p.rows()*2*9];
    	float [] pVec = new float[p.rows()*2];
    	float [] qVec = new float[q.rows()*2];
    	p.get(0, 0,pVec);
    	q.get(0, 0,qVec);
    	
    	for (int i = 0; i < p.rows(); i++)
    	{
    		B2vec[2*i*9+3] = -pVec[i*2];
    		B2vec[2*i*9+4] = -pVec[i*2+1];
    		B2vec[2*i*9+5] = -1;
    		
    		B2vec[2*i*9+6] = pVec[i*2]*qVec[2*i+1];
    		B2vec[2*i*9+7] = pVec[i*2+1]*qVec[2*i+1];
    		B2vec[2*i*9+8] = qVec[2*i+1];
    		
    		B2vec[(2*i+1)*9+0] = pVec[i*2];
    		B2vec[(2*i+1)*9+1] = pVec[i*2+1];
    		B2vec[(2*i+1)*9+2] = 1;
    		
    		B2vec[(2*i+1)*9+6] = -pVec[i*2]*qVec[2*i];
    		B2vec[(2*i+1)*9+7] = -pVec[i*2+1]*qVec[2*i];
    		B2vec[(2*i+1)*9+8] = -qVec[2*i];
    	}
    	
    	B.put(0, 0, B2vec);
    	
    	Mat s = new Mat();
    	Mat u = new Mat();
    	Mat vt = new Mat();
    	
    	Core.SVDecomp(B,s,u,vt,Core.SVD_FULL_UV);
    	Mat h = vt.row(8);
    	
    	Mat H = h.reshape(0, 3);

    	Mat s2 = new Mat();
    	Mat u2 = new Mat();
    	Mat vt2 = new Mat();
    	Core.SVDecomp(H, s2, u2, vt2);
    	
    	H.convertTo(H, H.type(), 1.0/s2.get(1, 0)[0]);
    	
    	// sign correction
    	Mat q4 = Mat.ones(1,3, CvType.CV_32FC1);
    	Mat p4 = Mat.ones(1,3, CvType.CV_32FC1);
    	
    	q4.put(0, 0, q.get(4, 0)[0]);
    	q4.put(0, 1, q.get(4, 1)[0]);
    	p4.put(0, 0, p.get(4, 0)[0]);
    	p4.put(0, 1, p.get(4, 1)[0]);
    	
    	Mat temp = new Mat(1,3,CvType.CV_32FC1);
    	
    	Core.gemm(q4, H, 1, new Mat(), 0, temp);
    	double res = temp.dot(p4);
    	
    	if(res<0)H.convertTo(H, H.type(), -1);
    	
    	return H;
    }
    
    /**
     * Gets the Rotation and Translation from Homography.
     *
     * @param H the Homography matrix
     * @param id_validity unused variable, always set to -1.
     * @return the 3x5 Transformation matrix of which top-left 3x3 matrix is Rotation matrix and fourth column vector is Translation vector and fifth column indicates validity and should be set of 0 if valid.
     */
    public static Mat getRTFromH(Mat H, int id_validity)
    {
		Mat retMat = Mat.zeros(3,5,CvType.CV_32FC1);
    	Mat Rot = Mat.zeros(3, 3, CvType.CV_32FC1);
    	Mat Tr  = Mat.zeros(3, 1, CvType.CV_32FC1);
    	retMat.put(0, 4, -1);
    	 Mat u = new Mat();
    	 Mat s = new Mat();
    	 Mat vt = new Mat();
    	 
    	 Mat HtH = new Mat();
    	 Core.gemm(H, H, 1, Mat.zeros(3, 3, CvType.CV_32FC1), 0, HtH,Core.GEMM_1_T);
    	 
    	 Core.SVDecomp(HtH, s, u, vt);

		 if(Core.determinant(u) < 0){
			 Scalar alpha = new Scalar(-1);
			 Core.multiply(u, alpha, u);
		 }
		 
		 double s1 = s.get(0,0)[0];
		 //double s2 = s.get(1,0)[0];
		 double s3 = s.get(2,0)[0];
		 
		 Mat v1 = u.col(0);
		 Mat v2 = u.col(1);
		 Mat v3 = u.col(2);
		 
		 Mat u1 = new Mat(3,1,CvType.CV_32FC1);
		 Mat u2 = new Mat(3,1,CvType.CV_32FC1);
		 
		 double denom = Math.sqrt(s1-s3);
		 for(int i=0;i<3;i++)
		 {
			 u1.put(i, 0,(v1.get(i,0)[0]*Math.sqrt(1-s3) + v3.get(i,0)[0]*Math.sqrt(s1-1))/denom);
			 u2.put(i, 0,(v1.get(i,0)[0]*Math.sqrt(1-s3) - v3.get(i,0)[0]*Math.sqrt(s1-1))/denom);
		 }
		 
		 Mat U1 = new Mat(3,3,CvType.CV_32FC1);
		 Mat U2 = new Mat(3,3,CvType.CV_32FC1);
		 Mat W1 = new Mat(3,3,CvType.CV_32FC1);
		 Mat W2 = new Mat(3,3,CvType.CV_32FC1);
		 
		 Mat skew_v2 = Algo3D.skew(v2);
		 
		 Mat skew_v2_u1 = new Mat();
		 Core.gemm(skew_v2, u1, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, skew_v2_u1, 0);
		 Mat skew_v2_u2 = new Mat();
		 Core.gemm(skew_v2, u2, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, skew_v2_u2, 0);
		 
		 Mat H_v2  = new Mat();
		 Core.gemm(H, v2, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, H_v2, 0);
		 Mat H_u1 = new Mat();
		 Core.gemm(H, u1, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, H_u1, 0);
		 Mat H_u2 = new Mat();
		 Core.gemm(H, u2, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, H_u2, 0);
		 
		 // get U1
		 Mat skew_H_v2 = Algo3D.skew(H_v2);
		 Mat skew_H_v2_H_u1 = new Mat();
		 Core.gemm(skew_H_v2, H_u1, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, skew_H_v2_H_u1, 0);
		 Mat skew_H_v2_H_u2 = new Mat();
		 Core.gemm(skew_H_v2, H_u2, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, skew_H_v2_H_u2, 0);    			 
		 
		 for(int i=0;i<3;i++)
		 {
			 U1.put(i, 0, v2.get(i,0)[0]);
			 U1.put(i, 1, u1.get(i,0)[0]);
			 U1.put(i, 2, skew_v2_u1.get(i,0)[0]);
			 
			 U2.put(i, 0, v2.get(i,0)[0]);
			 U2.put(i, 1, u2.get(i,0)[0]);
			 U2.put(i, 2, skew_v2_u2.get(i,0)[0]);
			 
			 W1.put(i, 0, H_v2.get(i,0)[0]);
			 W1.put(i, 1, H_u1.get(i,0)[0]);
			 W1.put(i, 2, skew_H_v2_H_u1.get(i,0)[0]);

			 W2.put(i, 0, H_v2.get(i,0)[0]);
			 W2.put(i, 1, H_u2.get(i,0)[0]);
			 W2.put(i, 2, skew_H_v2_H_u2.get(i,0)[0]);
		 }
		 
		 Mat N1 = skew_v2_u1;
		 Mat N2 = skew_v2_u2;
		 
		 Mat W1_U1t = new Mat();
		 Mat W2_U2t = new Mat();
		 Core.gemm(W1, U1, 1, Mat.zeros(3, 3, CvType.CV_32FC1), 0, W1_U1t, Core.GEMM_2_T);
		 Core.gemm(W2, U2, 1, Mat.zeros(3, 3, CvType.CV_32FC1), 0, W2_U2t, Core.GEMM_2_T);
		 
		 Mat H_W1_U1t = new Mat();
		 Mat H_W2_U2t = new Mat();
		 Core.subtract(H, W1_U1t, H_W1_U1t);
		 Core.subtract(H, W2_U2t, H_W2_U2t);
		 
		 Mat H_W1_U1t_N1 = new Mat();
		 Mat H_W2_U2t_N2 = new Mat();
		 Core.gemm(H_W1_U1t, N1, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, H_W1_U1t_N1, 0);
		 Core.gemm(H_W2_U2t, N2, 1, Mat.zeros(3, 1, CvType.CV_32FC1), 0, H_W2_U2t_N2, 0);
		 
		 boolean [] validity = new boolean[4];
		 
		 if(id_validity==-1){
			 for(int i=0;i<4;i++)validity[i]=true;
			 if(N1.get(2,0)[0]<0)
			 {
				 validity[0]=false;
			 }else{
				 validity[2]=false;
			 }
			 
			 if(N2.get(2,0)[0]<0){
				 validity[1]=false;
			 }else{
				 validity[3]=false;
			 }			 
			 
			double x1 = Math.atan2(W1_U1t.get(2,1)[0], W1_U1t.get(2,2)[0]);
			double y1 = Math.atan2(-W1_U1t.get(2,0)[0], Math.sqrt(W1_U1t.get(2,1)[0]*W1_U1t.get(2,1)[0] + W1_U1t.get(2,2)[0]*W1_U1t.get(2,2)[0]));
			
			double x2 = Math.atan2(W2_U2t.get(2,1)[0], W2_U2t.get(2,2)[0]);
			double y2 = Math.atan2(-W2_U2t.get(2,0)[0], Math.sqrt(W2_U2t.get(2,1)[0]*W2_U2t.get(2,1)[0] + W2_U2t.get(2,2)[0]*W2_U2t.get(2,2)[0]));
			
		if(Math.abs(x1)>Math.PI/2.0 || Math.abs(y1)>Math.PI/2.0){ 
		     validity[0]=false;
		     validity[2]=false;
		}
		 
		  if(Math.abs(x2)>Math.PI/2.0 || Math.abs(y2)>Math.PI/2.0){ 
		     validity[1]=false;
		     validity[3]=false;
		  }
		  
		  // calculating the most straight normal  
		  float [] n_org = {0,0,1};
		  float th_align = 0.05f;
		  float mindist = 100000;
		  int i_final = -1;
		  for(int i=0;i<4;i++)
		  {
			  if(validity[i])
			  {
				  Mat n = new Mat();
				  if(i==0){n=N1;}
				  if(i==1){n=N2;}
				  if(i==2){Core.multiply(N1, Mat.ones(3,1,CvType.CV_32FC1), n, -1);}
				  if(i==3){Core.multiply(N2, Mat.ones(3,1,CvType.CV_32FC1), n, -1);}
				  
				  float n_norm = (float) Math.sqrt(n.dot(n));
				  Core.multiply(n, Mat.ones(3,1,CvType.CV_32FC1), n, 1.0f/n_norm);
				  
				  float dist = (float) Math.sqrt((n.get(0,0)[0]-n_org[0])*(n.get(0,0)[0]-n_org[0])+(n.get(1,0)[0]-n_org[1])*(n.get(1,0)[0]-n_org[1])+(n.get(2,0)[0]-n_org[2])*(n.get(2,0)[0]-n_org[2]));
			      
				  //Log.e("track", "n "+i+" "+dist);
				  if(dist<mindist){
					  i_final = i;
					  mindist = dist;
				  }
				  if(dist>th_align){
					  validity[i]=false;
				  }
			  }else{
				  //Log.e("track", "n "+i+" 10000");
			  }
		  }
		  
		  if(i_final==-1)return retMat;
		  
		  validity[0]=false;
		  validity[1]=false;
		  validity[2]=false;
		  validity[3]=false;
		  validity[i_final]=true;
		 }else{
			 for(int i=0;i<4;i++)validity[i]=false;
			 validity[id_validity]=true;
		 }
		 if(validity[0]){
			 Rot=W1_U1t;
			 Tr=H_W1_U1t_N1;
			 retMat.put(0, 4, 0);
		 }
		 if(validity[1]){
			 Rot=W2_U2t;
			 Tr=H_W2_U2t_N2;
			 retMat.put(0, 4, 1);
		 }		 
		 if(validity[2]){
			 Rot=W1_U1t;
			 Core.multiply(H_W1_U1t_N1, Mat.ones(3,1,CvType.CV_32FC1), Tr, -1);
			 retMat.put(0, 4, 2);
		 }
		 if(validity[3]){
			 Rot=W2_U2t;
			 Core.multiply(H_W2_U2t_N2, Mat.ones(3,1,CvType.CV_32FC1), Tr, -1);
			 retMat.put(0, 4, 3);
		 }
		 
		 // adjust Tr based on pattern size
    	Mat p1 = Mat.zeros(1, 3, CvType.CV_32FC1);
    	Mat p2 = Mat.zeros(1, 3, CvType.CV_32FC1);
    	p1.put(0, 0, 3);
    	p1.put(0, 1, 2);
    	p1.put(0, 2, 1);
    	
    	p2.put(0, 0, 21);
    	p2.put(0, 1, 14);
    	p2.put(0, 2, 1);
    	
    	Mat x1 = new Mat();
    	Mat x2 = new Mat();
    	Core.gemm(Rot, p1, 1, Tr, 1, x1, Core.GEMM_2_T);
    	Core.gemm(Rot, p2, 1, Tr, 1, x2, Core.GEMM_2_T);
    		
		 for(int i=0;i<3;i++)
		 {
			 for(int j=0;j<3;j++){
				 retMat.put(i, j, Rot.get(i, j)[0]);
			 }
			 retMat.put(i, 3, Tr.get(i, 0)[0]);
		 }
		 
		 return retMat;
    }
    
	/**
	 * Gets the shortest side.
	 *
	 * @param x0 the x coordinate of point 0
	 * @param y0 the y coordinate of point 0
	 * @param x1 the x coordinate of point 1
	 * @param y1 the y coordinate of point 1
	 * @param x2 the x coordinate of point 2
	 * @param y2 the y coordinate of point 2
	 * @param x3 the x coordinate of point 3
	 * @param y3 the y coordinate of point 3
	 * @return the shortest side
	 */
	public static double getShortestSide(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3){
		double d_min=500000;
		double d;
		d = Math.sqrt((x1-x0)*(x1-x0)+(y1-y0)*(y1-y0));
		if(d<d_min)d_min=d;
		d = Math.sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
		if(d<d_min)d_min=d;
		d = Math.sqrt((x3-x2)*(x3-x2)+(y3-y2)*(y3-y2));
		if(d<d_min)d_min=d;
		d = Math.sqrt((x0-x3)*(x0-x3)+(y0-y3)*(y0-y3));
		if(d<d_min)d_min=d;
		
		// check plane orientation - direction of cross product of the perp lines
		double dx1 = x1-x0;
		double dy1 = y1-y0;
		double dx2 = x2-x1;
		double dy2 = y2-y1;
		
		double d1 = Math.sqrt(dx1*dx1+dy1*dy1);
		double d2 = Math.sqrt(dx2*dx2+dy2*dy2);
		
		dx1 /= d1;
		dy1 /= d1;
		dx2 /= d2;
		dy2 /= d2;
		double crossProd = dx1*dy2-dy1*dx2;
		double angle = Math.asin(crossProd);
		
		if(angle<Math.PI/6.0)return -1;
		
		return d_min;
	}
    
    /**
     * Gets the calibrated points.
     *
     * @param p the input points
     * @param mCamModelParams the camera model parameters
     * @return the calibrated points
     */
    public static Mat getCalibratedPoints(Mat p, CameraModelParams mCamModelParams)
    {
		long ts = System.nanoTime();
    	// copy points
    	float pts[] = new float[p.rows()*2];
    	float kkinv[] = new float[9];
    	
    	mCamModelParams.mKKInv.get(0, 0, kkinv);
    	p.get(0, 0, pts);
    	
    	for(int i=0;i<p.rows();i++){
    		if(pts[2*i]==-1)continue;
    		float p1 = pts[2*i]*kkinv[0]+pts[2*i+1]*kkinv[1]+kkinv[2];
    		float p2 = pts[2*i]*kkinv[3]+pts[2*i+1]*kkinv[4]+kkinv[5];

    		float p1_org=p1;
    		float p2_org=p2;
    		for(int t=0;t<5;t++){
    			float r2 = p1*p1 + p2*p2;
    			float r4 = r2*r2;
    			float k_radial = 1 + mCamModelParams.mKc1*r2 + mCamModelParams.mKc2*r4;
    			p1 = p1_org/k_radial;
    			p2 = p2_org/k_radial;
    		}
    		pts[2*i]=p1;
    		pts[2*i+1]=p2;
    	}
    	
    	Mat p_calib = Mat.zeros(p.rows(),2,CvType.CV_32FC1);
    	p_calib.put(0, 0, pts);
    	
    	TUtil.spent(ts,"calibrate points");
    	return p_calib;
    }
    
    /**
     * Gets the un-calibrated points.
     *
     * @param p the input points
     * @param mCamModelParams the camera model parameters
     * @return the un-calibrated points
     */
    public static Mat getUnCalibratedPoints(Mat p, CameraModelParams mCamModelParams)
    {
    	// copy points
    	float pts[] = new float[p.rows()*2];
    	float kk[] = new float[9];
    	
    	mCamModelParams.mKK.get(0, 0, kk);
    	p.get(0, 0, pts);
    	
    	for(int i=0;i<p.rows();i++){
    		if(pts[2*i]==-1)continue;
    		
			float r2 = pts[2*i]*pts[2*i] + pts[2*i+1]*pts[2*i+1];
			float r4 = r2*r2;
			float k_radial = 1 + mCamModelParams.mKc1*r2 + mCamModelParams.mKc2*r4;
			float p1 = pts[2*i]*k_radial;
			float p2 = pts[2*i+1]*k_radial;

    		pts[2*i]   = p1*kk[0]+p2*kk[1]+kk[2];
    		pts[2*i+1] = p1*kk[3]+p2*kk[4]+kk[5];
    	}
    	
    	Mat p_uncalib = Mat.zeros(9,2,CvType.CV_32FC1);
    	p_uncalib.put(0, 0, pts);
    	
    	return p_uncalib;
    }
    
    /**
     * Skew.
     *
     * @param m_in the input vector
     * @return the skew matrix corresponding to the input vector
     */
    public static Mat skew(Mat m_in)
    {
    	Mat m_out = new Mat(3,3,CvType.CV_32FC1);
    	m_out.put(0, 0, 0);
    	m_out.put(0, 1, m_in.get(2, 0)[0] * -1);
    	m_out.put(0, 2, m_in.get(1, 0)[0]);
    	m_out.put(1, 0, m_in.get(2, 0)[0]);
    	m_out.put(1, 1, 0);
    	m_out.put(1, 2, m_in.get(0, 0)[0] * -1);
    	m_out.put(2, 0, m_in.get(1, 0)[0] * -1);
    	m_out.put(2, 1, m_in.get(0, 0)[0]);
    	m_out.put(2, 2, 0);
    	
    	return m_out;
    }
}
