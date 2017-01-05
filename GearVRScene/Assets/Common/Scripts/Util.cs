using UnityEngine;
using System.Collections;
using System;

public class Util {

	static public void copyTransform( Transform source, Transform target ) {
		target.localPosition = source.localPosition;
		target.localRotation = source.localRotation;
		target.localScale = source.localScale;
	}

	static public void switchParent( Transform aObjectTransform, Transform newParentTransform ) {
		Vector3 localPosition = aObjectTransform.localPosition;
		Vector3 localScale = aObjectTransform.localScale;
		Quaternion localRotation = aObjectTransform.localRotation;
		aObjectTransform.parent = newParentTransform;
		aObjectTransform.localPosition = localPosition;
		aObjectTransform.localScale = localScale;
		aObjectTransform.localRotation = localRotation;
	}

	static public GameObject cloneGameObject( GameObject aGameObject, string newName ) {
		GameObject newGameObject = new GameObject( newName );
		newGameObject.transform.parent = aGameObject.transform.parent;
		Util.copyTransform( aGameObject.transform, newGameObject.transform );
		return newGameObject;
	}

	static public string extractFileExtension( string url ) {
		//Debug.Log("extractFileExtension: " + url);
		int questionIndex = url.IndexOf( "?" );
		string s = ( questionIndex < 0 ) ? url : url.Substring( 0, questionIndex );
		int index = s.LastIndexOf (".");
		return (index < 0) ? string.Empty : s.Substring( index );
	}

	public static Vector3 shortenAngles( Vector3 angles ) {
		angles.x  = shortenAngle ( angles.x );
		angles.y  = shortenAngle ( angles.y );
		angles.z  = shortenAngle ( angles.z );
		return angles;
	}

	public static float shortenAngle( float angle ) {
		if ( angle > 180 ) return angle - 360.0f;
		if ( angle < -180 ) return 360.0f + angle ;
		return angle;
	}

	private static bool kLogging = true;
	public static void Log( string s ) {
		if ( kLogging ) {
			Debug.Log( s );
		}
	}

	// utc string: "2014-09-04T05:55:46.700760"
	static public DateTime parseUTCDateTime( string utc ) {
		//Debug.Log("parseUTCDateTime( string utc ): '" + utc + "'");
		try {
			int index = 0;
			string yearString = utc.Substring( index, utc.IndexOf( '-' ) );
			int year = parseInt( yearString );
			index += yearString.Length + 1;

			string monthString = utc.Substring( index, utc.Substring( index ).IndexOf( '-' ) );
			int month = parseInt( monthString );
			index += monthString.Length + 1;

			string dayString = utc.Substring( index, utc.Substring( index ).IndexOf( 'T' ) );
			int day = parseInt( dayString );
			index += dayString.Length + 1;

			//Debug.Log("dateTime parse: " + year + ":" + month + ": " + day);

			string hourString = utc.Substring( index, utc.Substring( index ).IndexOf( ':' ) );
			int hour = parseInt( hourString );
			index += hourString.Length + 1;

			string minuteString = utc.Substring( index, utc.Substring( index ).IndexOf( ':' ) );
			int minute = parseInt( minuteString );
			index += minuteString.Length + 1;

			string secondString = utc.Substring( index );
			int second = (int)parseFloat( secondString );

			DateTime dateTime = new DateTime( year, month, day, hour, minute, second, DateTimeKind.Utc );
			return dateTime;
		} catch( System.Exception e ) {
			Debug.Log(e.Message);
		}

		return DateTime.Now;
	}

	static public TimeSpan getTimeDeltaFromNow( string utc ) {
		DateTime dateTime = parseUTCDateTime( utc );
		DateTime now = DateTime.Now.ToUniversalTime();
		return now - dateTime;
	}

	static float parseFloat( string s ) {
		return ( s == null ) ? 0 : float.Parse( s );
	}

	static long parseLong( string s ) {
		return ( s == null ) ? 0 : long.Parse( s );
	}

	static int parseInt( string s ) {
		return ( s == null ) ? 0 : int.Parse( s );
	}
	
	static public float getDiskSpaceAvailableInMegabytes( string path ) {
		float megabytes = float.MaxValue;
#if ( UNITY_ANDROID && !UNITY_EDITOR )
		AndroidJavaClass jobj = new AndroidJavaClass("com.samsung.srad.vr360videoutil.VideoUtil" );
		megabytes = jobj.CallStatic<float>( "getDiskSpaceAvailableInMegabytes", path );
#else
		//megabytes = 0;
#endif
		//Debug.Log("getDiskSpaceAvailableInMegabytes: " + megabytes + "MB");
		return megabytes;
	}
}
	