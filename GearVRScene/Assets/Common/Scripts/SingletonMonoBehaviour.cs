using UnityEngine;
using System.Collections;

abstract public class SingletonMonoBehaviour<Type> : MonoBehaviour where Type : MonoBehaviour {
	static Type gInstance = null;
	static public Type getInstance() {
		if ( gInstance == null ) {
			gInstance = FindObjectOfType<Type> ();
		}
		return gInstance;
	}

}
