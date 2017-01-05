using UnityEngine;
using System.Collections;

// Require the following components when using this script
[RequireComponent(typeof(AudioSource))]
public class LaserEyes : MonoBehaviour
{
	public LineRenderer laserPrefab; 	// public variable for Laser prefab 

	private BotControlScript botCtrl;	// control script
	private Transform EyeL;				// Left Eye position transform
	private Transform EyeR;				// Right Eye position transform
	private LineRenderer laserL;		// Left Eye Laser Line Renderer
	private LineRenderer laserR;		// Right Eye Laser Line Renderer
	private bool shot;					// a toggle for when we have shot the laser

	
	void Start()
	{		
		// creating the two line renderers to initialise our variables
		laserL = new LineRenderer();
		laserR = new LineRenderer();
		
		// initialising eye positions
		EyeL = transform.Find("EyeL");
		EyeR = transform.Find("EyeR");
		
		// finding the BotControlScript on the root parent of the character
		botCtrl = transform.root.GetComponent<BotControlScript>(); 
		
		// setting up the audio component
		GetComponent<AudioSource>().loop = true;
		GetComponent<AudioSource>().playOnAwake = false;
	}
	
	
	void Update ()
	{
		// if the look weight has been increased to 0.9, and we have not yet shot..
		if(botCtrl.lookWeight >= 0.9f && !shot)
		{
			// instantiate our two lasers
			laserL = Instantiate(laserPrefab) as LineRenderer;
			laserR = Instantiate(laserPrefab) as LineRenderer;
			
			// register that we have shot once
			shot = true;
			// play the laser beam effect
			GetComponent<AudioSource>().Play ();
		}
		// if the look weight returns to normal
		else if(botCtrl.lookWeight < 0.9f)
		{
			// Destroy the laser objects
			Destroy(laserL);
			Destroy(laserR);
			
			// reset the shot toggle
			shot = false;
			// stop audio playback
			GetComponent<AudioSource>().Stop();
		}
		// if our laser line renderer objects exist..
		if(laserL != null)
		{
			// set positions for our line renderer objects to start at the eyes and end at the enemy position, registered in the bot control script
			laserL.SetPosition(0, EyeL.position);
			laserL.SetPosition(1, botCtrl.enemy.position);
			laserR.SetPosition(0, EyeR.position);
			laserR.SetPosition(1, botCtrl.enemy.position);
		}
	}
}
