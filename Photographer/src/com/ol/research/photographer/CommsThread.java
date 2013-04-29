package com.ol.research.photographer;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Calendar;

import org.json.JSONException;
import org.json.JSONObject;
import org.opencv.core.Core;

import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.os.Handler;
import android.os.Message;
import android.util.Log;


class CommsThread implements Runnable {
	Handler handler;
	Camera mCamera;
	PictureCallback mPicture;
	static OutputStream socket_os;
	static Socket s = null;
	static boolean isSendComplete;	// Used by SendImageService
	ShutterCallback shutter;
	
    private static final String TAG = "COMM";
	
	Calendar right_now;
//	Calendar last_midnight = MainActivity.last_midnight;
	long calendar_offset = MainActivity.calendar_offset;
	
	public CommsThread(Handler hand, Camera mCamera, PictureCallback mPicture, ShutterCallback shutter)
	{
		handler=hand;
		this.mCamera = mCamera;
		this.mPicture = mPicture;
		this.shutter = shutter;
	}
	
	public void run() {
        
		// Wait until OpenCV is loaded (needed for time measurement)
		while(TempTickCountStorage.GetTimeStamp() == 0)
		{
			try {
				Thread.sleep(500);
			} catch (InterruptedException ex)
			{
				// Interruption is not a problem...
			}
		}
		
		//offset measurement
		//right_now = Calendar.getInstance();
		//long actual_time = right_now.getTimeInMillis() - last_midnight.getTimeInMillis();
		//long actual_time = (right_now.getTimeInMillis() + calendar_offset) % (24 * 60 * 60 * 1000);
		long actual_time = TempTickCountStorage.GetTimeStamp();
		mCamera.takePicture(shutter, null, null);
		/*long[] timearray = new long[10];
		for(int i=0; i<5; i++)
		{
			mCamera.takePicture(shutter, null, null);
			try {
				Thread.sleep(1000L);
			} catch (InterruptedException e1) {
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
			timearray[i]=MainActivity.millis_since_midnight;
		}*/
		try {
			Thread.sleep(1000L);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		long time_offset = MainActivity.timestamp - actual_time;
		
        try {
        		MainActivity.ss = new ServerSocket(MainActivity.SERVERPORT);
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        while (!Thread.currentThread().isInterrupted()) {
        	s = null;
        	Message m = new Message();
            InputStream is = null;
            OutputStream out = null;
            m.what = MainActivity.MSG_ID;
            try
            {          	 	
        		Log.i(TAG, "Waiting for connection...");
                s = MainActivity.ss.accept();
                TempTickCountStorage.ConnectionReceived = TempTickCountStorage.GetTimeStamp();
                Log.i(TAG, "Receiving...");
                is = s.getInputStream();
                ByteArrayOutputStream bos = new ByteArrayOutputStream();
                int ch;
                while ((ch=is.read())!=-1) {
                	bos.write(ch);
                }
                   
               	String message = new String(bos.toByteArray());
                TempTickCountStorage.CommandReceived = TempTickCountStorage.GetTimeStamp();
               	
                Log.i(TAG, "Processing...");
               	try {
               		JSONObject jObj = new JSONObject(message);
               		String type = jObj.getString("type");
               		long desired_timestamp = jObj.getLong("desiredtimestamp");
               		
    	    		actual_time = TempTickCountStorage.GetTimeStamp();
    	    		
	               	if (type.equals("takepicture"))// ----------- TAKE PICTURE command
	               	{
	                    Log.i(TAG, "Cmd: take picture...");
	               		/*if(time_to_wait>0)
	               		{
	               		Thread.sleep(time_to_wait); //sleep-nél is ugyanúgy megjelenik a kb 300ms-os késés 
	               		}*/
	                    Log.i(TAG, "Waiting for desired timestamp...");
	                    TempTickCountStorage.StartWait = TempTickCountStorage.GetTimeStamp();
	                    TempTickCountStorage.DesiredTimeStamp = desired_timestamp; 
	               		if(desired_timestamp != 0 && desired_timestamp > actual_time)
	               		{
	               			// TODO: time_offset is now calculated before OpenCV initializes, so
	               			//	its value is of no meaning... should be fixed later.
//	               			while(desired_timestamp >= (actual_time+time_offset))
	               			while(desired_timestamp >= actual_time) //esetleges megoldás: offset kezelése -> (actual_time+300)
	               			{
	               				actual_time = TempTickCountStorage.GetTimeStamp();
	               				// TODO: add sleep if the desired timestamp is still far away...
	               			}
	               		}
	                    Log.i(TAG, "Taking picture...");
	                    isSendComplete = false;	// SendImageService will set this true...
	                    TempTickCountStorage.TakingPicture = TempTickCountStorage.GetTimeStamp();
	               		mCamera.takePicture(shutter, null, mPicture);
	               		
	                    Log.i(TAG, "Waiting for sync...");
	                    while(!isSendComplete)
	                    {
		               		synchronized (s)
		               		{
		               			// Wait() may also be interrupted,
		               			// does not necessarily mean that send is complete.
		               			s.wait();
		               		}
		               		Log.i(TAG,"Wait() finished");
	                    }
	                    Log.i(TAG, "Sync received, data sent.");
	               	} else if(type.equals("ping"))	// ----------- PING command
	               	{
	                    Log.i(TAG, "Cmd: ping...");
	               		out = s.getOutputStream();       
	                    DataOutputStream output = new DataOutputStream(out);     
	                    output.writeUTF("pong");
	                    output.flush();
	               	}
	               	MainActivity.mClientMsg = message;
	                Log.i(TAG, "Sending response...");
	               	handler.sendMessage(m);
	               	// Save timing info
	               	TempTickCountStorage.WriteToLog();
               	} catch (JSONException e) {
                    Log.e("JSON Parser", "Error parsing data " + e.toString());
                }   	
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			} /*finally {
            	if (is != null) {
            		try {
						is.close();
					} catch (IOException e) {
						e.printStackTrace();
					}
            	}
            	if (out != null) {
            		try {
						out.close();
					} catch (IOException e) {
						e.printStackTrace();
					}
            	}
            }*/
        }
        
    }
}
