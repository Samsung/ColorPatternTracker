using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class InputController : SingletonMonoBehaviour<InputController> {
	
	// Actoin must occur in LESS time than threshold.  Time is in seconds
	public float mOnClickTime = 0.25f;
	public float mSwipeTimeLimit = 1.0f;
	public float mLongPressBeginTime = 1.01f;
	public float mLongPressEndTime = 1.5f;
	public float mBackLongPressTime = 1.2f;
	
	private Vector2 mTouchStart = new Vector2();
	private Vector2 mTouchPosition = new Vector2();
	private float mTouchStartTime = 0;
	private float mBackButtonStartTime = 0;
	private float mDT = 0;

	private bool mOnClick = false;
	private bool mIsPressed = false;
	private bool mOnSwipeLeft = false;
	private bool mOnSwipeRight = false;
	private bool mOnSwipeUp = false;
	private bool mOnSwipeDown = false;
	private bool mOnLongPress = false;
	private bool mOnLongClick = false;
	private bool mOnCancelLongClick = false;
	private bool mOnLongPressReported = false;
		
	private float mSwipeDistance = 0;
	private float mSwipeTime = 0;
	
	static List<InputListener> gListeners = new List<InputListener> ();
	
	//
	// Accessors
	//
	public bool OnClick
	{
		get { return mOnClick; }
	}
	
	public bool IsPressed
	{
		get { return mIsPressed; }
	}
	
	public bool OnSwipeLeft
	{
		get { return mOnSwipeLeft; }
	}
	
	public bool OnSwipeRight
	{
		get { return mOnSwipeRight; }
	}
	
	public bool OnSwipeUp
	{
		get { return mOnSwipeUp; }
	}
	
	public bool OnSwipeDown
	{
		get { return mOnSwipeDown; }
	}
	
	public bool OnLongPress
	{
		get { return mOnLongPress; }
	}
	
	public bool OnLongClick
	{
		get { return mOnLongClick; }
	}
	
	public bool OnCancelLongClick
	{
		get { return mOnCancelLongClick; }
	}
	
	public float swipeDistance {
		get { return mSwipeDistance; }
	}
	
	public float swipeTime {
		get { return mSwipeTime; }
	}
	
	public float getSwipeSpeed() {
		return Mathf.Abs( swipeDistance / ( ( swipeTime <= 0 ) ? 0.001f : swipeTime ) );
	}
	
	public float GetLongPressDeltaTime()
	{
		float timeSlice = Mathf.Max((mDT - mLongPressBeginTime), 0.0f);
		float dt = timeSlice / (mLongPressEndTime - mLongPressBeginTime);
		return dt;
	}
	
	// Use this for initialization
	void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
		
		mOnClick = false;
		mOnSwipeLeft = false;
		mOnSwipeRight = false;
		mOnSwipeUp = false;
		mOnSwipeDown = false;
		mOnLongPress = false;
		mOnLongClick = false;
		mOnCancelLongClick = false;
		mSwipeDistance = 0;
		
		if (Input.GetKeyDown (KeyCode.JoystickButton5) || Input.GetKeyDown (KeyCode.R))
		{
			mOnSwipeRight = true;
		}
		
		if( Input.GetKeyDown (KeyCode.JoystickButton4 ) || Input.GetKeyDown (KeyCode.L))
		{
			mOnSwipeLeft = true;
		}
		
		if (Input.GetKeyDown (KeyCode.Mouse0) || Input.GetKeyDown (KeyCode.JoystickButton2)|| Input.GetKeyDown (KeyCode.JoystickButton0))
		{
			mTouchStart.x = Input.mousePosition.x;
			mTouchStart.y = Input.mousePosition.y;
			mTouchStartTime = Time.unscaledTime;
			mDT = 0.0f;
			mIsPressed = true;
			mOnLongPressReported = false;
			mTouchPosition = mTouchStart;
			notifyTouchDown(Input.mousePosition.x, Input.mousePosition.y);
		}
		else if (mTouchStartTime != 0)
		{
			mDT = Time.unscaledTime - mTouchStartTime;
			
			float dx = Input.mousePosition.x - mTouchStart.x;
			float dy = Input.mousePosition.y - mTouchStart.y;
			
			if (Input.GetKeyUp(KeyCode.Mouse0) || Input.GetKeyUp (KeyCode.JoystickButton2) || Input.GetKeyUp (KeyCode.JoystickButton0))
			{
				if (checkForLongClick(dx, dy, mDT))
				{
					notifyTouchLongClick(Input.mousePosition.x,Input.mousePosition.y);
					mOnLongClick = true;
					
				}
				else if (checkForClick(dx, dy, mDT))
				{
					mOnClick = true;
					notifyTouchClicked(Input.mousePosition.x, Input.mousePosition.y);
				}
				else if (checkForSwipeLeftRight(dx, dy, mDT))
				{
					mOnSwipeLeft = (dx < 0.0f);
					mOnSwipeRight = (dx > 0.0f);
					mSwipeDistance = dx;
					mSwipeTime = mDT;
					notifyTouchSwiped(dx,0);
				}
				else if (checkForSwipeUpDown(dx, dy, mDT))
				{
					mOnSwipeDown = (dy < 0.0f);
					mOnSwipeUp = (dy > 0.0f);
					mSwipeDistance = dy;
					mSwipeTime = mDT;
					notifyTouchSwiped(0,dy);
				}
				
				mIsPressed = false;
				mTouchStartTime = 0.0f;
				mDT = 0.0f;
				mOnLongPressReported = false;
				
				notifyTouchUp(Input.mousePosition.x, Input.mousePosition.y);
			}
			else
			{
				if (!mOnCancelLongClick && checkForLongPress(dx, dy, mDT) && !mOnLongPressReported)
				{
					mOnLongPress = true;
					mOnLongPressReported = true;
					
					notifyTouchLongPress(Input.mousePosition.x,Input.mousePosition.y);
				}
				else if (!mOnCancelLongClick && checkForCancelLongClick(dx, dy, mDT))
				{
					mOnCancelLongClick = true;
					//mTouchStartTime = 0.0f;
					//mDT = 0.0f;
				}
				
				if ( mTouchPosition.x != Input.mousePosition.x || mTouchPosition.y != Input.mousePosition.y ) {
					mTouchPosition.x = Input.mousePosition.x;
					mTouchPosition.y = Input.mousePosition.y;
					notifyTouchMoved(mTouchPosition.x, mTouchPosition.y);
				}
			}
		}

		if (Input.GetKeyDown (KeyCode.Escape))
		{
			mBackButtonStartTime = Time.unscaledTime;
		}
		else if (Input.GetKeyUp(KeyCode.Escape) && (mBackButtonStartTime != 0.0f))
		{
			float dt = Time.unscaledTime - mBackButtonStartTime;

			if (dt > mBackLongPressTime)
			{
				//Debug.Log("Notify back long press");
				notifyBackLongPress();
			}
			else
			{
				//Debug.Log("Notify back press");
				notifyBackPress();
			}
			
			mBackButtonStartTime = 0.0f;
		}
	}
	
	public bool IsPossibleLongPressOrClick()
	{
		return (mIsPressed == true) && (mDT >= mLongPressBeginTime);
	}
	
	private bool checkForClick( float dx, float dy, float dt ) {
		const float kClickDistance = 20f;
		return (Mathf.Abs (dx) < kClickDistance && dt <= mOnClickTime);
	}
	
	private bool checkForSwipeLeftRight( float dx, float dy, float dt ) {
		const float kSwipeDirectionFactor = 1.8f;
		const float kSwipeDistance = 90f;
		//Debug.Log("checkForSwipeLeftRight direction factor: " +Mathf.Abs (dx / ((dy==0)?1:dy)));
		return (Mathf.Abs (dx) >= kSwipeDistance && Mathf.Abs (dx / ((dy==0)?1:dy)) > kSwipeDirectionFactor && dt <= mSwipeTimeLimit);
	}
	
	private bool checkForSwipeUpDown( float dx, float dy, float dt ) {
		const float kSwipeDirectionFactor = 1.8f;
		const float kSwipeDistance = 70f;
		//Debug.Log("checkForSwipeUpDown direction factor: " +Mathf.Abs (dy / ((dx==0)?1:dx)));
		return (Mathf.Abs (dy) >= kSwipeDistance && Mathf.Abs (dy / ((dx==0)?1:dx)) > kSwipeDirectionFactor && dt <= mSwipeTimeLimit);
	}
	
	private bool checkForLongPress( float dx, float dy, float dt ) {
		//const float kMoveDistance = 20;
		//return (Mathf.Abs (dx) < kMoveDistance && Mathf.Abs (dy) < kMoveDistance && (dt > mLongPressEndTime));
		return (dt > mLongPressEndTime);
	}
	
	private bool checkForLongClick( float dx, float dy, float dt ) {
		const float kMoveDistance = 20;
		return (Mathf.Abs (dx) < kMoveDistance && Mathf.Abs (dy) < kMoveDistance && (dt > mLongPressEndTime));
	}
	
	private bool checkForCancelLongClick( float dx, float dy, float dt ) {
		const float kMoveDistance = 20f;
		return ((Mathf.Abs (dx) > kMoveDistance || Mathf.Abs (dy) > kMoveDistance) && (dt >= mLongPressBeginTime));
	}
	
	static public void addInputListener( InputListener listener ) {
		if ( listener != null )
			gListeners.Add (listener);
	}
	
	static public void removeInputListener( InputListener listener ) {
		if ( listener != null )
			gListeners.Remove (listener);
	}
	
	static void notifyTouchDown( float x, float y ) {
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchDown(x,y);
		}
	}
	
	static void notifyTouchUp( float x, float y ) {
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchUp(x,y);
		}
	}
	
	static void notifyTouchMoved( float x, float y ) {
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchMove(x,y);
		}
	}
	
	static void notifyTouchClicked( float x, float y ) {
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchClick(x,y);
		}
	}
	
	static void notifyTouchLongPress( float x, float y ) {
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchLongPress(x,y);
		}
	}
	
	static void notifyTouchSwiped( float x, float y ) {
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchSwipe(x,y);
		}
	}
	
	static void notifyTouchLongClick(float x, float y)
	{
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].touchLongClick(x,y);
		}
	}

	static void notifyBackPress()
	{
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].backPress();
		}
	}

	static void notifyBackLongPress()
	{
		for ( int i = 0; i < gListeners.Count; i++ ) {
			gListeners[i].backLongPress();
		}
	}
	
	
}
