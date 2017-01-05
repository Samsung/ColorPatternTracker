using UnityEngine;

public class AnimSpriteAlpha : Anim {
	private Color mStartValue;
	private float mEndValue;
	private Color mNewValue;
	private float mOriginalValue;
	
	void Awake() {
		mOriginalValue = getCurrentAlpha();
		//Debug.Log(name + "::Awake spritealpha mOriginalValue: " + mOriginalValue);
	}

	protected override void updateAnim( float factor, float deltaTime ) {
		mNewValue.a = Mathf.Lerp( mStartValue.a, mEndValue, factor );
		base.gameObject.GetComponent<SpriteRenderer>().color = mNewValue;
	}

	public void animateToValue( float alpha ) {
		mEndValue = alpha;
		mStartValue = base.gameObject.GetComponent<SpriteRenderer>().color;
		mNewValue = mStartValue;
		startAnimation();
	}

	public Color getCurrentColor() {
		return base.gameObject.GetComponent<SpriteRenderer>().color;
	}

	public void setAlpha( float alpha ) {
		Color c = getCurrentColor();
		c.a = alpha;
		base.gameObject.GetComponent<SpriteRenderer>().color = c;
	}

	public float getCurrentAlpha() {
		return getCurrentColor().a;
	}

	public float getOriginalValue() {
		return mOriginalValue;
	}
}
