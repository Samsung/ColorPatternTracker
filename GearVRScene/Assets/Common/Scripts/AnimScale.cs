using UnityEngine;
using System.Collections;

public class AnimScale : Anim {

	private Vector3 mStartValue = new Vector3();
	private Vector3 mEndValue = new Vector3();

	protected override void updateAnim( float factor, float deltaTime ) {
		gameObject.transform.localScale = Vector3.Lerp( mStartValue, mEndValue, factor );
	}

	public void animateToValue( Vector3 v ) {
		animateToValue (v.x, v.y, v.z);
	}

	public void animateToValue( float x, float y, float z ) {
		mEndValue.Set (x,y,z);
		Vector3 current = gameObject.transform.localScale;
		mStartValue.Set (current.x, current.y, current.z);
		startAnimation();
	}
	
}
