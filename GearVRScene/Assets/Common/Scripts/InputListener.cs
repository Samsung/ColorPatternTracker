using UnityEngine;
using System.Collections;

public interface InputListener {

	// x/y is current touch point
	void touchDown( float x, float y );
	void touchUp( float x, float y );
	void touchMove( float x, float y );
	void touchClick( float x, float y );
	void touchLongPress ( float x, float y );

	// Fired on long press and release
	// Can be canceled by moving finger a certain delta before release
	void touchLongClick ( float x, float y ); 
											  	
	// dx, dy is delta x/y of swiped distance
	void touchSwipe( float dx, float dy );

	void backPress();
	void backLongPress();
}

public class DefaultInputListener : MonoBehaviour, InputListener
{
	public virtual void touchDown(float x, float y) {}
	public virtual void touchUp(float x, float y) {}
	public virtual void touchMove(float x, float y) {}
	public virtual void touchClick(float x, float y) {}
	public virtual void touchLongPress(float x, float y) {}
	public virtual void touchLongClick(float x, float y) {}
	public virtual void touchSwipe(float dx, float dy) {}
	public virtual void backPress() {}
	public virtual void backLongPress() {}
}


