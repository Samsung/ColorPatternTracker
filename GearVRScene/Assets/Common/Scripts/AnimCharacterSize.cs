using UnityEngine;

public class AnimCharacterSize : Anim {
	private float mStartValue;
	private float mEndValue;


	protected override void updateAnim( float factor, float deltaTime ) {
		float newValue = Mathf.Lerp( mStartValue, mEndValue, factor );
		setSize( newValue );
	}

	virtual public float getCurrentSize() {
		return base.gameObject.GetComponent<TextMesh>().characterSize;
	}

	virtual public void setSize( float size ) {
		base.gameObject.GetComponent<TextMesh>().characterSize = size;
	}
	
	public void animateToValue( float alpha ) {
		mEndValue = alpha;
		mStartValue = getCurrentSize ();
		startAnimation();
	}

}
