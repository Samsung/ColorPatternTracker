using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class MovieCoverflowAnimator : SingletonMonoBehaviour<MovieCoverflowAnimator>, Anim.Listener, InputListener {
	public float singleScrollDuration = 0.25f;
	public float distancePerDegrees = 4;
	public int swipeMaxSlots = 3;
	public float rotationSpeedLimit = 100;
	public float SingleIconAngle = 15;

	List<Coverflow> mCategoryIcons = new List<Coverflow>();

	AnimRotation mAnimRotation = null;
	
	// Starting index to keep track of many icons have been scrolled
	int mCurrentIconIndex = 0;
	int mStartedIconIndex = 0;
	
	// Number of icons will be scrolled
	int mScrollIconsCount = 0;
	int mScrollDirection = 1; // [-1,1]
	
	Vector3 mIntialRotation = Vector3.zero;

	float kSingleIconSpeedSwipe = 1000;
	
	void Start() {
		Coverflow[] coverflows = transform.GetComponentsInChildren<Coverflow>();
		for ( int i = 0; i < coverflows.Length; i++ ) {
			mCategoryIcons.Add( coverflows[i] );
		}
		
		mAnimRotation = GetComponent<AnimRotation>();
		mAnimRotation.setListener( this );
		
		mIntialRotation = transform.localEulerAngles;

		InputController.addInputListener( this );
	}
	
	void OnDestroy() {
		InputController.removeInputListener( this );
	}
	
	// Update is called once per frame
	void Update () {
		if ( mIsTouchMoved ) {
			float speed = mTouchDeltaX / distancePerDegrees;

			// Check for speed limit
			if ( speed >= 0 ) {
				speed = Mathf.Min ( rotationSpeedLimit, speed );
			}
			else {
				speed = Mathf.Max ( -rotationSpeedLimit, speed );
			}

			float angles = speed * Time.deltaTime;
			if ( angles != 0 ) {
				transform.Rotate( 0, angles, 0 );
			}
		}
	}
	
	public float getSingleIconAngle() {
		return SingleIconAngle;//360.0f/(float)mCategoryIcons.Count;
	}
	
	public bool isBusy() {
		return mAnimRotation.isAnimating();
	}
	
	void scrollCoverflows( float degreesPerIcon, float speed ) {
		// Calculate duration/degrees based on speed
		//Debug.Log("distance: " + InputController.getInstance().swipeDistance);
		//Debug.Log("time: " + InputController.getInstance().swipeTime);
		// factor: [1..10]
		mScrollIconsCount = (int)Mathf.Min( swipeMaxSlots, Mathf.Round( Mathf.Max ( 1.0f, (speed / kSingleIconSpeedSwipe) ) ) );
		mScrollDirection = (degreesPerIcon>0) ? -1 : 1;
		float angle = mScrollIconsCount * degreesPerIcon;
		mAnimRotation.duration = Mathf.Pow( mScrollIconsCount, 0.8f ) * singleScrollDuration;
		mAnimRotation.animate( transform.up, angle );
		//Debug.Log("scrollCoverflows: speed/factor: " + speed + "/" + mScrollIconsCount);
		
		mStartedIconIndex = mCurrentIconIndex;
	}
	
	public void startLeftRotationForCategories( float speed ) {
		Debug.Log(name + " - startLeftRotationForCategories: " + speed );
		scrollCoverflows( -getSingleIconAngle() + getNearestSlotAngle(), speed );
	}
	
	public void startRightRotationForCategories( float speed ) {
		Debug.Log(name + " - startRightRotationForCategories: " + speed );
		scrollCoverflows( getSingleIconAngle() + getNearestSlotAngle(), speed );
	}
	
	public void resetRotations() {
		transform.localEulerAngles = mIntialRotation;
	}
	
	public void settleToNearestSlot() {
		if ( isBusy() ) {
			return;
		}
		
		float deltaAngle = getNearestSlotAngle();
		if ( Mathf.Abs( deltaAngle ) > 0.01f ) {
			scrollCoverflows( deltaAngle, kSingleIconSpeedSwipe );
		}
	}
	
	float getNearestSlotAngle() {
		float rotation = transform.localEulerAngles.y;
		float deltaAngle = rotation % getSingleIconAngle();
		if ( deltaAngle < -0.5f*getSingleIconAngle() ) {
			deltaAngle = -getSingleIconAngle() - deltaAngle;
		}
		else if ( deltaAngle > 0.5f*getSingleIconAngle() ) {
			deltaAngle = getSingleIconAngle() - deltaAngle;
		}
		else {
			deltaAngle = -deltaAngle;
		}
		return deltaAngle;
	}

	//-----------------------
	// AnimRotation.Listener methods
	//-----------------------
	public void animationStarted( Anim anim ) {
	}
	
	public void animationUpdated( Anim anim, float factor ) {
		if ( mScrollIconsCount > 0 ) {
			// index may be negative
			int index = (int)(mStartedIconIndex + (mScrollDirection * factor * mScrollIconsCount));
			index = index % mCategoryIcons.Count;
			if ( index < 0 ) {
				index += mCategoryIcons.Count;
			}
			
			if ( index != mCurrentIconIndex ) {
				mCurrentIconIndex = index;
				//Debug.Log("new index: " + mCurrentIconIndex + " - end: " + getEndSphereIndex());
			}

			//Debug.Log("index reached: " + index);
		}
	}
	
	// rotatedObject is SphereThumb
	public void animationEnded( Anim anim, float factor ) {
		//Debug.Log("animationEnded index reached: " + mCurrentIconIndex);
		//Debug.Log (name + "::rotationAnimationEnded() - " + rotatedObject.name);
		//settleToNearestSlot();
		//Debug.Log("animationEnded singleicon: " + getSingleIconAngle());
		//Debug.Log("animationEnded angle: " + transform.localEulerAngles.y);
	}
	
	public bool isScrollingLeft() {
		return mScrollDirection > 0;
	}
	
	public bool isScrollingRight() {
		return mScrollDirection < 0;
	}
	
	// Return the index of "end" sphere 
	int getEndSphereIndex() {
		int index = mCurrentIconIndex + mCategoryIcons.Count/2;
		return index % mCategoryIcons.Count;
	}
	
	int getLeftSphereIndexOf( int index ) {
		index = index - 1 + mCategoryIcons.Count;
		return index % mCategoryIcons.Count;
	}
	
	int getRightSphereIndexOf( int index ) {
		index = index + 1;
		return index % mCategoryIcons.Count;
	}

	public void animateToAngle( float targetAngle, float duration ) {
		float deltaAngles = Util.shortenAngle( targetAngle ) - Util.shortenAngle( transform.localEulerAngles.y );
		mAnimRotation.duration = duration;
		mAnimRotation.animate( transform.up, deltaAngles );
	}


	//------------------------------------------------------
	// InputListener methods
	//------------------------------------------------------
	bool mIsTouchMoved = false;
	Vector2 mTouchDownStart = new Vector2();
	float mTouchDeltaX = 0;
	public void touchDown( float x, float y ) {
		mIsTouchMoved = true;
		mTouchDeltaX = 0;
		mTouchDownStart.x = x;
		mTouchDownStart.y = y;
	}
	public void touchUp( float x, float y ){
		mIsTouchMoved = false;
		// Delay call to settleToNearestSlot to insure swipe is considered
		Invoke( "settleToNearestSlot", 0.02f );
	}
	public void touchMove( float x, float y ){
		mTouchDeltaX = x - mTouchDownStart.x;
		mIsTouchMoved = true;
		//Debug.Log("touchmove: mTouchDeltaX : " + mTouchDeltaX);
	}
	public void touchClick( float x, float y ){}
	public void touchSwipe( float dx, float dy ){}
	public void touchLongPress( float x, float y ) {}
	public void touchLongClick( float x, float y ) {}
	public void backPress() {}
	public void backLongPress() {}
}
