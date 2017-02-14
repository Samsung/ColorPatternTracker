package com.samsung.dtl.patterntracker.predictor;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

// TODO: Auto-generated Javadoc
/**
 * The Class KalamanFilter.
 */
public class KalamanFilter {
    
    
    int kalman_states; /*!< The number of kalman states. */
    int kalman_measurements; /*!< The number of kalman measurements. */ 
    public Mat kalman_m_n1, kalman_P_n1, kalman_y_n, kalman_F_n, kalman_Q_n, kalman_H_n, kalman_R_n; /*!< The intermediate kalman matrix. */
    Mat mLastMovement; /*!< The direction of movement in last frame. */
    
	/**
	 * Instantiates a new kalaman filter.
	 */
	public KalamanFilter(){
		kalman_states = 9;
		kalman_measurements = 3;
		
		kalman_m_n1 = Mat.zeros(kalman_states, 1,CvType.CV_32FC1);
		kalman_P_n1 = Mat.eye(kalman_states, kalman_states, CvType.CV_32FC1);
		kalman_y_n  = Mat.zeros(kalman_measurements, 1, CvType.CV_32FC1);
		kalman_F_n  = Mat.eye(kalman_states, kalman_states, CvType.CV_32FC1);
		kalman_Q_n  = Mat.zeros(kalman_states, kalman_states, CvType.CV_32FC1);
		kalman_H_n  = Mat.zeros(kalman_measurements, kalman_states, CvType.CV_32FC1); 
		kalman_R_n  = Mat.zeros(kalman_measurements, kalman_measurements, CvType.CV_32FC1);
		
		mLastMovement = Mat.ones(3, 1, CvType.CV_32FC1);
		mLastMovement.convertTo(mLastMovement, CvType.CV_32FC1, 100);
	}
	
    /**
     * Initialize kalman filter.
     *
     * @param dt the time spent since last frame
     */
    public void initializeKalmanJava(double dt){
    	dt=0.125;
  	                 /* DYNAMIC MODEL */
  	  //  [1 0 0 dt  0  0 dt2   0   0 0 0 0  0  0  0   0   0   0]
  	  //  [0 1 0  0 dt  0   0 dt2   0 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 1  0  0 dt   0   0 dt2 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  1  0  0  dt   0   0 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  0  1  0   0  dt   0 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  0  0  1   0   0  dt 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  0  0  0   1   0   0 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  0  0  0   0   1   0 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  0  0  0   0   0   1 0 0 0  0  0  0   0   0   0]
  	  //  [0 0 0  0  0  0   0   0   0 1 0 0 dt  0  0 dt2   0   0]
  	  //  [0 0 0  0  0  0   0   0   0 0 1 0  0 dt  0   0 dt2   0]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 1  0  0 dt   0   0 dt2]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 0  1  0  0  dt   0   0]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 0  0  1  0   0  dt   0]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  1   0   0  dt]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  0   1   0   0]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  0   0   1   0]
  	  //  [0 0 0  0  0  0   0   0   0 0 0 0  0  0  0   0   0   1]
  	  // position
  	  kalman_F_n.put(0,3,dt);
  	  kalman_F_n.put(1,4,dt);
  	  kalman_F_n.put(2,5,dt);
  	  kalman_F_n.put(3,6,dt);
  	  kalman_F_n.put(4,7,dt);
  	  kalman_F_n.put(5,8,dt);
  	  kalman_F_n.put(0,6,0.5*dt*dt);
  	  kalman_F_n.put(1,7,0.5*dt*dt);
  	  kalman_F_n.put(2,8,0.5*dt*dt);

	kalman_m_n1 = Mat.zeros(kalman_states, 1,CvType.CV_32FC1);
	kalman_P_n1 = Mat.eye(kalman_states, kalman_states, CvType.CV_32FC1); // 1
	kalman_y_n  = Mat.zeros(kalman_measurements, 1, CvType.CV_32FC1);
	kalman_Q_n  = Mat.eye(kalman_states, kalman_states, CvType.CV_32FC1).mul(Mat.eye(kalman_states, kalman_states, CvType.CV_32FC1), 1e-5); // 1e-5
	kalman_H_n  = Mat.zeros(kalman_measurements, kalman_states, CvType.CV_32FC1); 
	kalman_R_n  = Mat.eye(kalman_measurements, kalman_measurements, CvType.CV_32FC1).mul(Mat.eye(kalman_measurements, kalman_measurements, CvType.CV_32FC1),1e-4); // 1e-4
  
    /* MEASUREMENT MODEL */
//  [1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
//  [0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
//  [0 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
//  [0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0 0]
//  [0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0 0]
//  [0 0 0 0 0 0 0 0 0 0 0 1 0 0 0 0 0 0]
	kalman_H_n.put(0, 0, 1);
	kalman_H_n.put(1, 1, 1);
	kalman_H_n.put(2, 2, 1);	
    }
    
    /**
     * Update kalman filter.
     *
     * @param dt the time spent since last frame
     * @param MeasurementNoiseEstimate the measurement noise estimate: currently not used 
     */
    public void updateKalmanJava(double dt, Mat MeasurementNoiseEstimate){
  	  // position
  	  kalman_F_n.put(0,3,dt);
  	  kalman_F_n.put(1,4,dt);
  	  kalman_F_n.put(2,5,dt);
  	  kalman_F_n.put(3,6,dt);
  	  kalman_F_n.put(4,7,dt);
  	  kalman_F_n.put(5,8,dt);
  	  kalman_F_n.put(0,6,0.5*dt*dt);
  	  kalman_F_n.put(1,7,0.5*dt*dt);
  	  kalman_F_n.put(2,8,0.5*dt*dt);

		// Predict
		Mat kalman_m_nn1, tempMat, kalman_P_nn1;
		kalman_m_nn1 = Mat.zeros(kalman_states, 1, CvType.CV_32FC1);
		tempMat = Mat.zeros(kalman_states, kalman_states, CvType.CV_32FC1);
		kalman_P_nn1 = Mat.zeros(kalman_states, kalman_states, CvType.CV_32FC1);

		// predict m
		Core.gemm(kalman_F_n, kalman_m_n1, 1, Mat.zeros(kalman_states, 1, CvType.CV_32FC1), 0, kalman_m_nn1, 0);

		// predict P
		Core.gemm(kalman_F_n, kalman_P_n1, 1, Mat.zeros(kalman_states, kalman_states, CvType.CV_32FC1), 0, tempMat, 0);
		Core.gemm(tempMat, kalman_F_n.t(), 1, kalman_Q_n, 1, kalman_P_nn1, 0);

		// Update
		Mat tempMat2 = Mat.zeros(kalman_measurements, kalman_states, CvType.CV_32FC1);
		Mat S_n = Mat.zeros(kalman_measurements, kalman_measurements, CvType.CV_32FC1);
		Mat K_n = Mat.zeros(kalman_states, kalman_measurements, CvType.CV_32FC1);
		Core.gemm(kalman_H_n, kalman_P_nn1, 1, Mat.zeros(kalman_measurements, kalman_states, CvType.CV_32FC1), 0, tempMat2, 0);
		Core.gemm(tempMat2, kalman_H_n.t(), 1, kalman_R_n, 1, S_n, 0);
		
		Mat tempMat3 = Mat.zeros(kalman_states, kalman_measurements, CvType.CV_32FC1);
		Core.gemm(kalman_P_nn1, kalman_H_n.t(), 1, Mat.zeros(kalman_states, kalman_measurements, CvType.CV_32FC1), 0,tempMat3, 0);
		Mat Sinv = S_n.inv(Core.DECOMP_SVD);
		Core.gemm(tempMat3, Sinv, 1, Mat.zeros(kalman_states, kalman_measurements, CvType.CV_32FC1), 0, K_n, 0);
		
		// update m
		Mat tempMat4 = Mat.zeros(kalman_measurements, 1, CvType.CV_32FC1);
		Mat kalman_m_n1_temp = Mat.zeros(kalman_m_n1.rows(), kalman_m_n1.cols(), CvType.CV_32FC1);
		Core.gemm(kalman_H_n, kalman_m_nn1, -1, kalman_y_n, 1, tempMat4, 0);
		Core.gemm(K_n, tempMat4, 1, kalman_m_nn1, 1, kalman_m_n1_temp, 0);
		
		Mat movement = Mat.zeros(3,1,CvType.CV_32FC1);
		movement.put(0, 0, kalman_m_n1.get(0, 0)[0]-kalman_m_n1_temp.get(0, 0)[0]);
		movement.put(1, 0, kalman_m_n1.get(1, 0)[0]-kalman_m_n1_temp.get(1, 0)[0]);
		movement.put(2, 0, kalman_m_n1.get(2, 0)[0]-kalman_m_n1_temp.get(2, 0)[0]);
		// check if moving
		int flag_moving=1;
		if(Math.abs(movement.get(0, 0)[0])>0.01)flag_moving=1;//0.02
		if(Math.abs(movement.get(1, 0)[0])>0.01)flag_moving=1;//0.02
		if(Math.abs(movement.get(2, 0)[0])>0.02)flag_moving=1;//0.04

		if(flag_moving==1){
			kalman_m_n1_temp.copyTo(kalman_m_n1);
		}else{
			// if new movement is along the last movement
			if(movement.get(0, 0)[0]*mLastMovement.get(0,0)[0]>=0 && movement.get(1, 0)[0]*mLastMovement.get(1,0)[0]>=0 && movement.get(2, 0)[0]*mLastMovement.get(2,0)[0]>=0){
				kalman_m_n1_temp.copyTo(kalman_m_n1);
			}
		}
				
		// update P
		Mat tempMat5 = Mat.zeros(kalman_states, kalman_states, CvType.CV_32FC1);
		Core.gemm(K_n, kalman_H_n, 1, Mat.zeros(kalman_states, kalman_states, CvType.CV_32FC1), 0, tempMat5, 0);
		Core.gemm(tempMat5, kalman_P_nn1, -1, kalman_P_nn1, 1, kalman_P_n1, 0);
		
		movement.copyTo(mLastMovement);
	}
}
