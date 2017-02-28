package com.samsung.dtl.colorpatterntracker.util;

import java.lang.ref.WeakReference;

import com.samsung.dtl.colorpatterntracker.MainTrackerActivity;

import android.os.Handler;
import android.os.Message;

// TODO: Auto-generated Javadoc
/**
 * The Class HandlerExtension.
 */
// ref: http://www.intertech.com/Blog/android-non-ui-to-ui-thread-communications-part-3-of-5/
public class HandlerExtension extends Handler{
	  
  	/** The current activity. */
  	private final WeakReference<MainTrackerActivity> currentActivity;
	  
	  /**
  	 * Instantiates a new handler extension.
  	 *
  	 * @param activity the activity
  	 */
  	public HandlerExtension(MainTrackerActivity activity){
	    currentActivity = new WeakReference<MainTrackerActivity>(activity);
	  }
	  
	  /* (non-Javadoc)
  	 * @see android.os.Handler#handleMessage(android.os.Message)
  	 */
  	@Override
	  public void handleMessage(Message message){
		  MainTrackerActivity activity = currentActivity.get();
	    if (activity!= null){
	       activity.updateText(message.getData().getString("result"));
	    }
	  }
}
