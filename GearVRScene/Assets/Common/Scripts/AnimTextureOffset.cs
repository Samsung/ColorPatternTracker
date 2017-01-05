using UnityEngine;
using System.Collections;

public class AnimTextureOffset : Anim {		
	private Vector2 mStartValue = new Vector2();
	private Vector2 mEndValue = new Vector2();

	Material mMaterial = null;

	void Awake() {
		mMaterial = GetComponent<MeshRenderer>().material;
	}

	protected override void updateAnim( float factor, float deltaTime ) {
		float newValueX = Mathf.Lerp( mStartValue.x, mEndValue.x, factor );
		float newValueY = Mathf.Lerp( mStartValue.y, mEndValue.y, factor );
		mMaterial.mainTextureOffset = new Vector2( newValueX, newValueY );
	}

	public void animateToValue( Vector2 start, Vector2 end ) {
		mStartValue = start;
		mEndValue = end;
		startAnimation();
	}
	
	public void animateToValue( float startValueX, float startValueY, float endValueX, float endValueY ) {
		animateToValue( new Vector2( startValueX, startValueY ), new Vector2( endValueX, endValueY ) );
	}

	public void animateToValue( float endValueX, float endValueY ) {
		Vector2 current = mMaterial.mainTextureOffset;
		animateToValue( current, new Vector2( endValueX, endValueY ) );
	}

	public Vector2 getCurrentOffset() {
		return mMaterial.mainTextureOffset;
	}

}
