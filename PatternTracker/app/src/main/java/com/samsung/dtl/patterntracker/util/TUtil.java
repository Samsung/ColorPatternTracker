package com.samsung.dtl.patterntracker.util;

import android.util.Log;

// TODO: Auto-generated Javadoc
/**
 * The Class TUtil.
 */
public class TUtil {
	/**
	 * Spent.
	 *
	 * @param ts the starting time
	 * @param str the description string
	 */
	public static void spent(long ts, String str){
		long te = System.nanoTime();  
		Log.i("2Track", "Time: "+ str + ": "+ (te-ts)/1000000.0 + " ms");		
	}
}
