package com.ol.research.photographer;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import org.opencv.core.Core;

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;
import com.ol.research.measurement.*;

/** Intent service to send an image using the SMEyeL JSON data format.
 *	It notifies the CommThread.s socket after the send is complete.
 *	TODO: Strange: socket of service is in a different object and thread? (Intent parameter?)   
 */
public class SendImageService extends IntentService{
		
	public SendImageService() 
	{
		super("SendImageService");
	}
	
	@Override
	protected synchronized void onHandleIntent(Intent intent) 
	{
		try {
			// Get output stream from the CommsThread
			OutputStream os = CommsThread.s.getOutputStream();
			// Prepare data to send

			//byte [] mybytearray = intent.getByteArrayExtra("BYTE_ARRAY");
			byte[] mybytearray = MainActivity.lastPhotoData;
			
			long timestamp = intent.getLongExtra("TIMESTAMP", 0);
	        String buff = Integer.toString(mybytearray.length);
	        
	        StringBuilder sb = new StringBuilder("{\"type\":\"JPEG\",\"size\":\""); 
	        sb.append(buff);
	        sb.append("\",\"timestamp\":\"");
	        sb.append(Long.toString(timestamp));
	        sb.append("\"}#");
	        String JSON_message = sb.toString();
	        
	        
	        // Send data
            //TempTickCountStorage.OnSendingResponse = TempTickCountStorage.GetTimeStamp();
	        CommsThread.TM.Stop(CommsThread.PostProcessPostJpegMsID);
	        CommsThread.TM.Stop(CommsThread.AllNoCommMsID);
	        CommsThread.TM.Start(CommsThread.SendingJsonMsID);
	        
	        Log.i("COMM","Sending JSON and image to PC");
	        DataOutputStream output = new DataOutputStream(os);     
	        output.writeUTF(JSON_message);
	        output.flush();
            //TempTickCountStorage.OnSendingJPEG = TempTickCountStorage.GetTimeStamp();
	        CommsThread.TM.Stop(CommsThread.SendingJsonMsID);
	        CommsThread.TM.Start(CommsThread.SendingJpegMsID);
	       // CommsThread.ActualResult.PostProcessPostJpegMs = PostProcessPostJpegMs.TimeMeasurementStop();
	        // ??? Ezt nem az output-ba kellene írni?
	        os.write(mybytearray,0,mybytearray.length);
	        
	        /*try {
				os.close();
			} catch (IOException e) {
				e.printStackTrace();
			}*/

	        // Flush output stream
	        os.flush();
            //TempTickCountStorage.OnResponseSent = TempTickCountStorage.GetTimeStamp();
	        CommsThread.TM.Stop(CommsThread.SendingJpegMsID);
	        CommsThread.TM.Stop(CommsThread.AllMsID);
	        // Notify CommsThread that data has been sent
	        Log.i("COMM","Data sent, sending notification to CommsThread...");
	        synchronized (CommsThread.s)
	   		{
	        	CommsThread.isSendComplete = true;
	        	CommsThread.s.notifyAll();
	   		}
	        //super.onDestroy();
	        
	        
		} catch (IOException e) {
            e.printStackTrace();
            
		} 
		/*finally{
			try {
				Thread.sleep(500);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}*/
	}
}
