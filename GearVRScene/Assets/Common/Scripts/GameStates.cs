using UnityEngine;
using System.Collections;
using System.Collections.Generic;

// This base class can be instantiate so that the animation only makes callbacks
// Subclass override updateAnim to implement specific property change
public class GameStates : SingletonMonoBehaviour<GameStates> {

	public GameObject[] GameObjects = null;
	public bool[] State1 = null;
	public bool[] State2 = null;
	public bool[] State3 = null;
	public bool[] State4 = null;

	List<bool[]> mStates = new List<bool[]>();
	int mCurrentStateIndex = 0;

	void Awake() {
		mStates.Add( State1 );
		mStates.Add( State2 );
		mStates.Add( State3 );
	}

	void Start() {
		setupForCurrentState();
	}

	void setupForCurrentState() {
		for ( int i = 0; i < GameObjects.Length; i++ ) {
			if ( GameObjects[i] != null ) {
				GameObjects[i].SetActive( mStates[mCurrentStateIndex][i] );
			}
		}
	}

	public void nextState() {
		mCurrentStateIndex++;
		if ( mCurrentStateIndex >= mStates.Count ) {
			mCurrentStateIndex = 0;
		}
		setupForCurrentState();
	}

	public void previousState() {
		mCurrentStateIndex--;
		if ( mCurrentStateIndex < 0 ) {
			mCurrentStateIndex = mStates.Count-1;
		}
		setupForCurrentState();
	}
}
