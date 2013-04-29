package com.ol.research.photographer;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import org.opencv.core.Core;

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

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
	        // Assemble JSON message TODO: use StringBuilder!
	        String JSON_message = new String("{\"type\":\"JPEG\",\"size\":\"");
	        JSON_message = JSON_message.concat(buff);
	        JSON_message = JSON_message.concat("\",\"timestamp\":\"");
	        JSON_message = JSON_message.concat(Long.toString(timestamp));
	        JSON_message = JSON_message.concat("\"}#");
	        
	        // Send data
            TempTickCountStorage.OnSendingResponse = TempTickCountStorage.GetTimeStamp();
	        System.out.println("Sending...");	// TODO: LogCat?
	        DataOutputStream output = new DataOutputStream(os);     
	        output.writeUTF(JSON_message);
	        output.flush();
            TempTickCountStorage.OnSendingJPEG = TempTickCountStorage.GetTimeStamp();
	        // ??? Ezt nem az output-ba kellene írni?
	        os.write(mybytearray,0,mybytearray.length);
	        
	        /*try {
				os.close();
			} catch (IOException e) {
				e.printStackTrace();
			}*/

	        // Flush output stream
	        os.flush();
	        //CommsThread.socket_flag = false;
            TempTickCountStorage.OnResponseSent = TempTickCountStorage.GetTimeStamp();

	        // Notify CommsThread that data has been sent
	        Log.i("COMM","Data sent, sending notification to CommsThread...");
	        synchronized (CommsThread.s)
	   		{
	        	CommsThread.isSendComplete = true;
	        	CommsThread.s.notifyAll();
	   		}
        
        
		} catch (IOException e) {
            e.printStackTrace();
            
		} 
		// Send response
		/*Intent respIntent = new Intent(MainActivity.KEY_REST_FILTER);
		respIntent.putExtra(MainActivity.KEY_REST_RESPONSE, result);
	    LocalBroadcastManager.getInstance(this).sendBroadcast(respIntent);*/
	}
}
