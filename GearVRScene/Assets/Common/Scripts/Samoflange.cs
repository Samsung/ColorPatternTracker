using UnityEngine;
using System.Collections;

public class Samoflange : MonoBehaviour
{
	void Update ()
	{
		transform.Rotate(new Vector3(0, 30 * Time.deltaTime, 0));
	}
}
