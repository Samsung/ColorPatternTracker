using UnityEngine;

public class AnimShaderAlpha : Anim {
	Material mMaterial = null;

	Color mColor = new Color();
	float mStartAlpha = 0;
	float mEndAlpha = 0;
	private float mOriginalValue;

	void Awake() {
		mMaterial = GetComponent<MeshRenderer>().GetComponent<Renderer>().material;
		mOriginalValue = getCurrentAlpha();
	}
	
	protected override void updateAnim( float factor, float deltaTime ) {
		mColor.a = Mathf.Lerp( mStartAlpha, mEndAlpha, factor );
		setColor ( mColor );
	}

	public void animateToValue( float alpha ) {
		mColor = getCurrentColor();
		mStartAlpha = mColor.a;
		mEndAlpha = alpha;
		startAnimation();
	}

	public Color getCurrentColor() {
		return mMaterial.GetColor ( "_Color" );
	}
	
	public float getCurrentAlpha() {
		return getCurrentColor().a;
	}

	void setColor( Color newColor ) {
		mMaterial.SetColor ("_Color", newColor);
	}

	public void setAlpha( float alpha ) {
		Color c = getCurrentColor();
		c.a = alpha;
		setColor ( c );
	}

	public float getOriginalValue() {
		return mOriginalValue;
	}

}
