using UnityEngine;
using System.Collections;

public class AppManager : MonoBehaviour, InputListener {

	void Start() {

		InputController.addInputListener( this );
	}

	// Update is called once per frame
	void Update () {
	
	}

	// InputListener methods
	public void touchDown(float x, float y) {}
	public void touchUp(float x, float y) {}
	public void touchMove(float x, float y) {}
	public void touchClick(float x, float y) {}
	public void touchLongPress(float x, float y) {}
	public void touchLongClick(float x, float y) {}
	public void touchSwipe(float dx, float dy) {
	}
	public void backPress() {
		GameStates.getInstance().nextState();
	}
	public void backLongPress() {
	}
}
