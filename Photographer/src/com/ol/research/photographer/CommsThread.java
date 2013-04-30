package com.ol.research.photographer;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;

import org.json.JSONException;
import org.json.JSONObject;

import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

import com.ol.research.measurement.*;


class CommsThread implements Runnable {
	
	Handler handler;
	Camera mCamera;
	PictureCallback mPicture;
	static OutputStream socket_os;
	static Socket s = null;
	static boolean isSendComplete;	// Used by SendImageService
	ShutterCallback mShutter;
	
    private static final String TAG = "COMM";   
    long TakingPicture; 
    
	 static MeasurementLog TimeMeasurementResults = new MeasurementLog();
	 TimeMeasurement TM = new TimeMeasurement();
		
	public CommsThread(Handler hand, Camera mCamera, PictureCallback mPicture, ShutterCallback mShutter)
	{
		handler=hand;
		this.mCamera = mCamera;
		this.mPicture = mPicture;
		this.mShutter = mShutter;
	}
	
	public void run() {
		
		// Wait until OpenCV is loaded (needed for time measurement)
		while(TimeMeasurement.isOpenCVLoaded == false)
		{
			try {
				Thread.sleep(500);
			} catch (InterruptedException ex)
			{
				// Interruption is not a problem...
			}
		}
		TM.setMeasurementLog(TimeMeasurementResults);
		TM.Start(1);
		long actual_time = TimeMeasurement.getTimeStamp();
		try {
			Thread.sleep(1000L);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		
		//long time_offset = MainActivity.timestamp - actual_time;
		
        try {
        		MainActivity.ss = new ServerSocket(MainActivity.SERVERPORT);
        } catch (IOException e) {
            e.printStackTrace();
        }
        
        s = null;
    	
        InputStream is = null;
        OutputStream out = null;
        
        try
        {          	 	
    		Log.i(TAG, "Waiting for connection...");
            s = MainActivity.ss.accept();
           // TempTickCountStorage.ConnectionReceived = TempTickCountStorage.GetTimeStamp();
            out = s.getOutputStream();
           // MainActivity.TimeMeasurementResults.WriteJSON(out);
            Log.i(TAG, "Receiving...");
            is = s.getInputStream();
            
            int ch=0;
        TM.Stop(1);
        
        
        while (!Thread.currentThread().isInterrupted()) {
        		TM.Start(2);
        		ByteArrayOutputStream bos = new ByteArrayOutputStream();
	        	Message receivedMessage = new Message(); //TODO: message.optain
	        	receivedMessage.what = MainActivity.MSG_ID;
                do {
                	ch=is.read();
                	if(ch != '#') //TODO: change to '\0'
                	{
                		bos.write(ch);
                	}
                }while(ch != '#');
                   
               	String message = new String(bos.toByteArray());
                //TempTickCountStorage.CommandReceived = TempTickCountStorage.GetTimeStamp(); 
               	
                Log.i(TAG, "Processing...");
           		JSONObject jObj = new JSONObject(message);
           		String type = jObj.getString("type");
           		long desired_timestamp = jObj.getLong("desiredtimestamp");
           		
	    		//actual_time = TimeMeasurement.GetTimeStamp();
	    		
               	if (type.equals("takepicture"))// ----------- TAKE PICTURE command
               	{
                    Log.i(TAG, "Cmd: take picture...");
                    Log.i(TAG, "Waiting for desired timestamp...");
                    //TempTickCountStorage.StartWait = TempTickCountStorage.GetTimeStamp();
               		if(desired_timestamp != 0 && desired_timestamp > actual_time)
               		{
               			// TODO: time_offset is now calculated before OpenCV initializes, so
               			//	its value is of no meaning... should be fixed later.
//	               			while(desired_timestamp >= (actual_time+time_offset))
               			while(desired_timestamp >= actual_time) 
               			{
               				actual_time = TimeMeasurement.getTimeStamp();
               				if((desired_timestamp - actual_time) > 5000000) //5 sec
               				{
               					Thread.sleep(4500);
               				}
               			}
               		}
                    Log.i(TAG, "Taking picture...");
                    isSendComplete = false;	// SendImageService will set this true...
                    //TempTickCountStorage.TakingPicture.add(TempTickCountStorage.GetTimeStamp());
                   // TakingPicture = TimeMeasurement.GetTimeStamp();
                    mCamera.takePicture(mShutter, null, mPicture);
               		
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
                    output.writeUTF("pong#");
                    output.flush();
               	} else if (type.equals("sendlog"))// ----------- SENDLOG command
               	{
               		Log.i(TAG, "Cmd: sendlog...");
               		out = s.getOutputStream();
               		TimeMeasurementResults.WriteJSON(out);
               	}
               	              	
               	MainActivity.mClientMsg = message;
                Log.i(TAG, "Sending response...");
               	handler.sendMessage(receivedMessage);
               	// Save timing info
               	//TempTickCountStorage.WriteToLog();
               	/*divider = Core.getTickFrequency() / 1000000.0;
               	ActualResult.DelayTakePicture = (double)(Math.round((TakingPicture - ActualResult.DesiredTimestamp)/divider/1000.0));
               	ActualResult.DelayOnShutter = (double)(Math.round((MainActivity.OnShutterEventTimestamp - ActualResult.DesiredTimestamp)/divider/1000.0));*/
               	TM.Stop(2);
              // 	ActualResult.WriteLog();
            }
       	} catch (JSONException e) {
            Log.e("JSON Parser", "Error parsing data " + e.toString());
        } catch (IOException e) {
            e.printStackTrace();
        } catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} 
    }      
}

