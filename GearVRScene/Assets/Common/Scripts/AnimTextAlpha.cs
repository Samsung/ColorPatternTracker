using UnityEngine;

public class AnimTextAlpha : Anim {
	private Color mStartValue;
	private float mEndValue;
	private Color mNewValue;
	private float mOriginalValue;
	
	void Awake() {
		mOriginalValue = getCurrentAlpha();
		//Debug.Log(name + "::Awake textalpha mOriginalValue: " + mOriginalValue);
	}

	protected override void updateAnim( float factor, float deltaTime ) {
		mNewValue.a = Mathf.Lerp( mStartValue.a, mEndValue, factor );
		base.gameObject.GetComponent<TextMesh>().color = mNewValue;
	}

	public void animateToValue( float alpha ) {
		mEndValue = alpha;
		mStartValue = base.gameObject.GetComponent<TextMesh>().color;
		mNewValue = mStartValue;
		startAnimation();
	}

	public Color getCurrentColor() {
		return base.gameObject.GetComponent<TextMesh>().color;
	}

	public void setAlpha( float alpha ) {
		Color c = getCurrentColor();
		c.a = alpha;
		base.gameObject.GetComponent<TextMesh>().color = c;
	}
	
	public float getCurrentAlpha() {
		return getCurrentColor().a;
	}

	public float getOriginalValue() {
		return mOriginalValue;
	}
}
