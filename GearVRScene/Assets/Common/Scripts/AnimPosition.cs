using UnityEngine;

public class AnimPosition : Anim {

	private Vector3 mStartValue;
	private Vector3 mEndValue;

	protected override void updateAnim( float factor, float deltaTime ) {
		base.gameObject.transform.localPosition = Vector3.Lerp( mStartValue, mEndValue, factor );
	}

	public void animateDeltaValue( Vector3 dv ) {
		animateDeltaValue (dv.x, dv.y, dv.z);
	}

	public void animateDeltaValue( float dx, float dy, float dz ) {
		Vector3 pos = base.gameObject.transform.localPosition;
		animateToValue (pos.x + dx, pos.y + dy, pos.z + dz);
	}

	public void animateToValue( Vector3 v ) {
		animateToValue (v.x, v.y, v.z);
	}
	
	public void animateToValue( float x, float y, float z ) {
		mStartValue = base.gameObject.transform.localPosition;
		mEndValue.x = x;
		mEndValue.y = y;
		mEndValue.z = z;
		startAnimation();
	}


}
