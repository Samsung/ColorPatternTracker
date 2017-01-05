using UnityEngine;

public class AnimColor : Anim {

	private Color mStartValue;
	private Color mEndValue;

	public Color getStartColor() {
		return mStartValue;
	}

	public Color getEndColor() {
		return mEndValue;
	}

	protected override void updateAnim( float factor, float deltaTime ) {
		Color color = Color.Lerp( mStartValue, mEndValue, factor );
		base.gameObject.GetComponent<MeshRenderer>().GetComponent<Renderer>().material.color = color;
	}

	public void animateToValue( Color toColor ) {
		Color from = base.gameObject.GetComponent<MeshRenderer>().GetComponent<Renderer>().material.color;
		animate( from, toColor );
	}

	public void animateToValue( float r, float g, float b, float a ) {
		Color toColor = new Color( r, g, b, a );
		animateToValue( toColor );
	}

	public void animate( Color from, Color to ) {
		mStartValue = from;
		mEndValue = to;
		startAnimation();
	}
}
