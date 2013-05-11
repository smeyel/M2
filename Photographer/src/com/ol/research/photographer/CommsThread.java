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
    ServerSocket ss = null;
    
	static MeasurementLog TimeMeasurementResults = new MeasurementLog();
	static TimeMeasurement TM = new TimeMeasurement();
	
	static int ReceptionMsID = 1;
	static int PreProcessMsID = 2;
	static int WaitingMsID = 3;
	static int TakePictureMsID = 4;
	static int AllMsID = 5;
	static int AllNoCommMsID = 6;
	static int PostProcessJPEGMsID = 7;
	static int PostProcessPostJpegMsID = 8;
	static int SendingJsonMsID = 9;
	static int SendingJpegMsID = 10;
	
	public CommsThread(Handler hand, Camera mCamera, PictureCallback mPicture, ShutterCallback mShutter)
	{
		handler=hand;
		this.mCamera = mCamera;
		this.mPicture = mPicture;
		this.mShutter = mShutter;
		TM.setMeasurementLog(TimeMeasurementResults);
		TM.setName(ReceptionMsID, "ReceptionMs");
		TM.setName(PreProcessMsID, "PreProcessMs");
		TM.setName(WaitingMsID, "WaitingMs");
		TM.setName(TakePictureMsID, "TakePictureMs");
		TM.setName(AllMsID, "AllMs");
		TM.setName(AllNoCommMsID,"AllNoCommMs");
		TM.setName(PostProcessJPEGMsID, "PostProcessJpegGMs");
		TM.setName(PostProcessPostJpegMsID, "PostProcessPostJpegMs");
		TM.setName(SendingJsonMsID, "SendingJsonMs");
		TM.setName(SendingJpegMsID, "SendingJpegMs");
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
		
		
		long actual_time = TimeMeasurement.getTimeStamp();
		try {
			Thread.sleep(1000L);
		} catch (InterruptedException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		
		InputStream is = null;
        OutputStream out = null;
        try
        { 
        
        	while (!Thread.currentThread().isInterrupted()) {	        
		       
        		ss = null;
        		ss = new ServerSocket(MainActivity.SERVERPORT);
	
		        s = null;	    	
		        is = null;
		        out = null;       
		        
	    		Log.i(TAG, "Waiting for connection...");
	            s = ss.accept();
	           // TempTickCountStorage.ConnectionReceived = TempTickCountStorage.GetTimeStamp();
	            
	            out = s.getOutputStream();
	           // MainActivity.TimeMeasurementResults.WriteJSON(out);
	            Log.i(TAG, "Receiving...");
	            is = s.getInputStream();
	            
	            int ch=0;
	            //s.isClosed()
	        
	        
	      //  while (!Thread.currentThread().isInterrupted()) {
	            while(true) //beak condition terminates the loop
	            {	
	        		TM.Start(AllMsID);
	        		TM.Start(ReceptionMsID);
	        		
	        		ByteArrayOutputStream bos = new ByteArrayOutputStream();
		        	Message receivedMessage = new Message(); //TODO: message.obtain
		        	receivedMessage.what = MainActivity.MSG_ID;
	                do {
	                	ch=is.read();
	                	if(ch != '#' && ch != -1) //TODO: change to '\0'
	                	{
	                		bos.write(ch);
	                	}
	                }while(ch != '#' && ch != -1);
	                
	                if(ch==-1) //connection terminated
	                {
	                	Log.i(TAG, "Connection terminated");
	                	break; 
	                }
	                
	               	String message = new String(bos.toByteArray());
	                //TempTickCountStorage.CommandReceived = TempTickCountStorage.GetTimeStamp(); 
	               	TM.Stop(ReceptionMsID);
	               	TM.Start(AllNoCommMsID);
	               	TM.Start(PreProcessMsID);
	               	
	               	
	                Log.i(TAG, "Processing...");
	           		JSONObject jObj = new JSONObject(message);
	           		String type = jObj.getString("type");
	           		long desired_timestamp = jObj.getLong("desiredtimestamp");
	           		
		    		actual_time = TimeMeasurement.getTimeStamp();
		    		
	               	if (type.equals("takepicture"))// ----------- TAKE PICTURE command
	               	{
	                    Log.i(TAG, "Cmd: take picture...");
	                    Log.i(TAG, "Waiting for desired timestamp...");
	                    //TempTickCountStorage.StartWait = TempTickCountStorage.GetTimeStamp();
	                    TM.Stop(PreProcessMsID);
	                    TM.Start(WaitingMsID);
	               		if(desired_timestamp != 0 && desired_timestamp > actual_time)
	               		{
	               			// TODO: time_offset is now calculated before OpenCV initializes, so
	               			//	its value is of no meaning... should be fixed later.
	//	               			while(desired_timestamp >= (actual_time+time_offset))
	               			while(desired_timestamp >= actual_time) 
	               			{
	               				actual_time = TimeMeasurement.getTimeStamp();
	               				if((desired_timestamp - actual_time) > 5000000) //if the desired time stamp is too far away, sleep 4.5 s
	               				{
	               					Thread.sleep(4500);
	               				}
	               			}
	               		}
	                    Log.i(TAG, "Taking picture...");
	                    isSendComplete = false;	// SendImageService will set this true...
	                    //TempTickCountStorage.TakingPicture.add(TempTickCountStorage.GetTimeStamp());
	                   // TakingPicture = TimeMeasurement.GetTimeStamp();
	                    TM.Stop(WaitingMsID);
	                    TM.Start(TakePictureMsID);
	                    mCamera.takePicture(mShutter, null, mPicture); //--------takePicture command
	               		
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
	               	double DelayTakePicture = TimeMeasurement.calculateIntervall(desired_timestamp, TakingPicture);
	               	TM.pushIntervallToLog("TakePictureDelayMs", DelayTakePicture);
	               	double DelayOnShutter = TimeMeasurement.calculateIntervall(desired_timestamp, MainActivity.OnShutterEventTimestamp);
	               	TM.pushIntervallToLog("OnShutterDelayMs", DelayOnShutter);
	              // 	ActualResult.WriteLog();
	            }
	            ss.close(); 
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

