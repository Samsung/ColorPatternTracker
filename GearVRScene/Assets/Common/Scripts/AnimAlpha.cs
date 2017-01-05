using UnityEngine;

public class AnimAlpha : Anim {
	private Color mStartValue;
	private float mEndValue;
	private Color mNewValue;
	private float mOriginalValue;

	void Awake() {
		mOriginalValue = getCurrentAlpha();
		//Debug.Log(name + "::Awake animalpha mOriginalValue: " + mOriginalValue);
	}

	protected override void updateAnim( float factor, float deltaTime ) {
		mNewValue.a = Mathf.Lerp( mStartValue.a, mEndValue, factor );
		setColor ( mNewValue );
	}

	virtual public Color getCurrentColor() {
		return base.gameObject.GetComponent<MeshRenderer>().GetComponent<Renderer>().material.color;
	}

	virtual public void setColor( Color newColor ) {
		base.gameObject.GetComponent<MeshRenderer>().GetComponent<Renderer>().material.color = newColor;
	}
	
	virtual public void setAlpha( float alpha ) {
		Color c = getCurrentColor();
		c.a = alpha;
		base.gameObject.GetComponent<MeshRenderer>().GetComponent<Renderer>().material.color = c;
	}

	public void animateToValue( float alpha ) {
		mEndValue = alpha;
		mStartValue = getCurrentColor ();
		mNewValue = mStartValue;
		startAnimation();
	}

	public float getCurrentAlpha() {
		return getCurrentColor().a;
	}

	public float getOriginalValue() {
		return mOriginalValue;
	}
}
