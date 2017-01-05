using UnityEngine;
using System.Collections;

// This base class can be instantiate so that the animation only makes callbacks
// Subclass override updateAnim to implement specific property change
public class Anim : MonoBehaviour {
	public interface Listener {
		void animationStarted ( Anim anim );
		// Factor: [0..1]
		void animationUpdated ( Anim anim, float factor );
		void animationEnded ( Anim anim, float factor );
	};

	public float duration = 1.0f;
	public float delay = 0.0f;

	protected bool mIsAnimating = false;
	protected float mStartTime;
	protected float mPreviousDT = 0;
	bool mIgnoreSettingAnimationValue = false;
	
	Listener mListener = null;

	virtual protected void updateAnim( float factor, float deltaTime ) {}

	public void setIgnoreSettingAnimationValue( bool b ) {
		mIgnoreSettingAnimationValue = b;
	}

	public bool isAnimating() {
		return mIsAnimating;
	}

	public void setListener( Listener listener ) {
		mListener = listener;
	}

	void notifyAnimationStarted() {
		if ( mListener != null) {
			mListener.animationStarted( this );
		}
	}
	
	void notifyAnimationUpdated( float factor ) {
		if ( mListener != null) {
			mListener.animationUpdated( this, factor );
		}
	}

	void notifyAnimationEnded( float factor ) {
		if ( mListener != null) {
			mListener.animationEnded( this, factor );
		}
	}

	public void stop() {
		mIsAnimating = false;
	}

	// Subclass should call this method
	public void startAnimation() {
		mIsAnimating = true;
		mStartTime = Time.time;
		mPreviousDT = 0;
		notifyAnimationStarted();
	}

	// Update is called once per frame
	void Update () {
		if ( mIsAnimating ) {
			float totalDT = Time.time - (mStartTime + delay);
			if ( totalDT > 0 ) {
				if ( totalDT > duration ) {
					totalDT = duration;
				}
				float factor = ( duration > 0 ) ? totalDT/duration : 1;
				mIsAnimating = ( factor < 1.0f );

				// power curve
				//factor = Mathf.Pow ( factor, factorToThePower );
				factor = 0.5f-0.5f*Mathf.Cos((Mathf.Pow( factor, 0.5f ))*3.14f);

				// Set mIgnoreCallingUpdateAnim to true to avoid calling updateAnim
				// Which can be used with a listener to override updating on animationUpdated() callback
				if ( !mIgnoreSettingAnimationValue ) {
					updateAnim( factor, totalDT - mPreviousDT );
				}
				notifyAnimationUpdated( factor );

				// end of animation:
				if ( !isAnimating() ) {
					notifyAnimationEnded( 1 );
				}

				mPreviousDT = totalDT;
			}
		}
	}

}
