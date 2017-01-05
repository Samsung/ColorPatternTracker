using UnityEngine;

public class AnimRotation : Anim {
	private Vector3 mAxis;
	private float mAngularVelocity;
	private Vector3 mNewValue;
	
	private Vector3 mCenter = Vector3.zero;
	private Space mSpace = Space.Self;
	
	Vector3 mStartingAngles = Vector3.zero;
	Vector3 mEndAngles = Vector3.zero;
	
	protected override void updateAnim( float factor, float deltaTime ) {
		if ( mSpace == Space.Self ) {
			//transform.Rotate (mAxis, mAngularVelocity * deltaTime);
			transform.localEulerAngles = Vector3.Lerp( mStartingAngles, mEndAngles, factor );
		}
		else {
			transform.RotateAround (mCenter, mAxis, mAngularVelocity * deltaTime);
		}
	}
	
	public void setCenter( Vector3 center ) {
		mCenter = center;
		setSpace ( Space.World );
	}
	
	public void setSpace (Space space ) {
		mSpace = space;
	}
	
	public void animate( Vector3 axis, float degrees) {
		// Self space rotation
		mStartingAngles = transform.localEulerAngles;
		mEndAngles = mStartingAngles + new Vector3(0,degrees,0);
		// world space rotation
		mAxis = axis;
		mAngularVelocity = degrees / duration;
		startAnimation();
	}

}
