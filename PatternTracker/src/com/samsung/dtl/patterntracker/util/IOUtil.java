package com.samsung.dtl.patterntracker.util;

import org.opencv.core.Mat;

import android.util.Log;

// TODO: Auto-generated Javadoc
/**
 * The Class IOUtil.
 */
public class IOUtil {
    
    /**
     * Prints the rotation and translation matrices.
     *
     * @param s the decription string
     * @param R the Rotation matrix
     * @param Tr the Translation matrix
     */
    public static void printRotTr(String s, Mat R, Mat Tr){
    	if(R.rows()==3 && R.cols()==3 && Tr.rows()==3){
    		//Log.i("2Track", "" +s + " "+ Tr.get(0, 0)[0]+" "+Tr.get(1, 0)[0]+" "+Tr.get(2, 0)[0]+" "+R.get(0, 0)[0]+" "+R.get(1, 0)[0]+" "+R.get(2, 0)[0]+" "+R.get(0, 1)[0]+" "+R.get(1, 1)[0]+" "+R.get(2, 1)[0]+" "+R.get(0, 2)[0]+" "+R.get(1, 2)[0]+" "+R.get(2, 2)[0]);
    		Log.i("2Track", " "+s+" "+(float)(Math.acos(R.get(0,0)[0])*180/Math.PI)+"  "+(float)(Math.acos(R.get(1,1)[0])*180/Math.PI)+" "+(float)(Math.acos(R.get(2,2)[0])*180/Math.PI));
    	}
    }
}
