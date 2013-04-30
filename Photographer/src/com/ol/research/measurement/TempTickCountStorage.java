package com.ol.research.measurement;

import org.opencv.core.Core;

import android.util.Log;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Vector;

/**
 * Temporal storage of tick counts for time measurement.
 * Used to collect data during picture taking and to avoid
 * 	LogCat operations during time measurement.
 * 
 * Events:
 * 	ConnectionReceived:	received connection, no data received yet
 *  CommandReceived:	command entirely received, no execution yet
 *  TakingPicture:		just before issuing takePicture
 *  OnShutterEvent:		onShutter event
 *  OnPictureTakenEvent:	onPictureTaken event
 *  OnSendingResponse:	just starting to send the response
 *  OnResponseSent:		just finished sending the response
 *  
 *  Warning, TickFrequency may change! TODO: check it at every GetTickCount() ?
 */
public class TempTickCountStorage {
	
	static public boolean isOpenCVLoaded = false;
	
	static public long ConnectionReceived;
	static public long CommandReceived;

	static public long StartWait;
	static public long DesiredTimeStamp;	// Received in the command
	
	//static public Vector<Long> TakingPicture;
	static public long TakingPicture;
	static public long OnShutterEvent;
	static public long OnPictureTakenEvent;
	static public long OnSendingResponse;
	static public long OnSendingJPEG;
	static public long OnResponseSent;

	static public double TickFrequency;
	
	final static String TAG = "TMEAS";
	final static String TAGCSV = "TMEAS_CSV";
	
	public static long GetTimeStamp()
	{
		if (isOpenCVLoaded)
		{
			TickFrequency = Core.getTickFrequency();
			double divider = TickFrequency / 1000000.0;
			long tick = Core.getCPUTickCount();
			long timestamp = (long)(Math.round(tick / divider));
			return timestamp;	// Returns in microseconds
		}
		else
		{
			return 0;
		}
	}

	public static void WriteToLog() {
		double divider = TickFrequency / 1000000.0;

		Log.i(TAG,"TickFrequency: "+TickFrequency);
		Log.i(TAG,"ConnectionReceived: "+ConnectionReceived);
		Log.i(TAG,"CommandReceived: "+CommandReceived);
		Log.i(TAG,"StartWait: "+StartWait+")");
		Log.i(TAG,"(DesiredTimeStamp: "+DesiredTimeStamp+")");
		Log.i(TAG,"TakingPicture: "+TakingPicture);
		Log.i(TAG,"OnShutterEvent: "+OnShutterEvent);
		Log.i(TAG,"OnPictureTakenEvent: "+OnPictureTakenEvent);
		Log.i(TAG,"OnSendingResponse: "+OnSendingResponse);
		Log.i(TAG,"OnSendingJpeg: "+OnSendingJPEG);
		Log.i(TAG,"OnResponseSent: "+OnResponseSent);
		
		
		double ReceptionMs = (double)(CommandReceived - ConnectionReceived)/1000.0;
		double PreProcessMs = (double)(StartWait - CommandReceived)/1000.0;
		double WaitingMs = (double)(TakingPicture - StartWait)/1000.0;
		double TakePictureMs = (double)(OnShutterEvent - TakingPicture)/1000.0;
		double PostProcessJPEGMs = (double)(OnPictureTakenEvent - OnShutterEvent)/1000.0;
		double PostProcessPostJpegMs = (double)(OnSendingResponse - OnPictureTakenEvent)/1000.0;
		double SendingJsonMs = (double)(OnSendingJPEG - OnSendingResponse)/1000.0;
		double SendingJpegMs = (double)(OnResponseSent - OnSendingJPEG)/1000.0;
		double AllMs = (double)(OnResponseSent - ConnectionReceived)/1000.0;
		double AllNoCommMs = (double)(OnSendingResponse - CommandReceived)/1000.0;
		
		// Delays regarding DesiredTimeStamp
		double DelayTakePicture = (double)(TakingPicture - DesiredTimeStamp)/1000.0;
		double DelayOnShutter = (double)(OnShutterEvent - DesiredTimeStamp)/1000.0;
		
		Log.i(TAG,"Reception: "+ReceptionMs);
		Log.i(TAG,"PreProcess: "+PreProcessMs);
		Log.i(TAG,"WaitingMs: "+WaitingMs);
		Log.i(TAG,"TakePicture: "+TakePictureMs);
		Log.i(TAG,"PostProcessJPEG: "+PostProcessJPEGMs);
		Log.i(TAG,"PostProcessPostJpeg: "+PostProcessPostJpegMs);
		Log.i(TAG,"SendingJson: "+SendingJsonMs);
		Log.i(TAG,"SendingJpeg: "+SendingJpegMs);
		Log.i(TAG,"All: "+AllMs);
		Log.i(TAG,"AllNoComm: "+AllNoCommMs);
		Log.i(TAG,"DelayTakePicture: "+DelayTakePicture);
		Log.i(TAG,"DelayOnShutter: "+DelayOnShutter);
		
		Log.i(TAGCSV,"CSV;"+ReceptionMs+";"+PreProcessMs+";"+WaitingMs+";"
				+TakePictureMs+";"+PostProcessJPEGMs+";"+PostProcessPostJpegMs+";"
				+SendingJsonMs+";"+SendingJpegMs+";"+AllMs+";"+AllNoCommMs+";"+DesiredTimeStamp
				+";"+DelayTakePicture+";"+DelayOnShutter);
	}

}
