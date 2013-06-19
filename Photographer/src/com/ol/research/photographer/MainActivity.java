package com.ol.research.photographer;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.SocketException;
import java.util.Enumeration;

import org.apache.http.client.methods.HttpGet;
import org.apache.http.conn.util.InetAddressUtils;
import org.apache.http.impl.client.BasicResponseHandler;
import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;

import android.app.Activity;
import android.content.Intent;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.net.http.AndroidHttpClient;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.TextView;
import android.widget.Toast;

import com.ol.research.measurement.*;

/**
 * 
 * MainActivity of the application (UI thread). <br>
 * It starts the CommsThread, and if picture was taken, the SendImageService is started from here.<br>
 * Contains the time measurement points.<br>
 * <br>
 * @author Milan Tenk <br>
 * <br>
 * Callbacks:<br>
 * PictureCallback: If the taken photo is ready, here will be the SendImageService started, the picture can be saved to SD card.<br>
 * ShutterCallback: Called right after taking photo.<br>
 * BaseLoaderCallback: Needed for OpenCV initialization.<br>
 * <br>
 * Handler: Handles the messages, that are written to the screen.<br>
 * <br>
 * Methods:<br>
 * onCreate: Will be called at the start of the activity, Commsthread will be started.<br>
 * onResume: Called, when the user is interfacing with the application. Contains the initialization of OpenCV.<br>
 * onStop: Called, when the activity is stopped.<br>
 * getLocalIpAddress: Determines the IP address of the Phone.<br>
 * httpReg: Registrates the phone on the specified Server.<br>
 */

public class MainActivity extends Activity {

	 protected static final int MSG_ID = 0x1337;
	 protected static final int SERVERPORT = 6000;
	 protected static final int TIME_ID = 0x1338;
	 private boolean saveToSD = false;
	 private static final String  TAG = "TMEAS";
	 ServerSocket ss = null;
	 static String mClientMsg = "";
	 static byte[] lastPhotoData;
	 static long OnShutterEventTimestamp;
	 static Object syncObj = new Object();
	 
	 CameraPreview mPreview;
	 Camera mCamera;
	 Thread myCommsThread = null;
	 String current_time = null;
	 
	 private PictureCallback mPicture = new PictureCallback() {

	        @Override
	        public void onPictureTaken(byte[] data, Camera camera) {
	        	CommsThread.TM.Stop(CommsThread.PostProcessJPEGMsID);
	        	CommsThread.TM.Start(CommsThread.PostProcessPostJpegMsID);
	        	//Option to save the picture to SD card
	        	if(saveToSD)
	        	{
		        	String pictureFile = Environment.getExternalStorageDirectory().getPath()+"/custom_photos"+"/__1.jpg";
		            try {
		                FileOutputStream fos = new FileOutputStream(pictureFile);
		                fos.write(data);
		                fos.close();   
		                
		            } catch (FileNotFoundException e) {
		                Log.d("Photographer", "File not found: " + e.getMessage());
		            } catch (IOException e) {
		                Log.d("Photographer", "Error accessing file: " + e.getMessage());
		            }
		            Log.v("Photographer", "Picture saved at path: " + pictureFile);
	        	}
	            lastPhotoData = data;
	            synchronized (syncObj) //notifys the Commsthread, if the picture is complete
    	   		{
    	        	CommsThread.isPictureComplete = true;
    	        	syncObj.notifyAll();
    	   		}
				mCamera.startPreview();
	        }
	    };
	    
	    private ShutterCallback mShutter = new ShutterCallback()
	    {
	    	@Override
	    	public void onShutter()
	    	{
	    		OnShutterEventTimestamp = TimeMeasurement.getTimeStamp();
	            current_time = String.valueOf(OnShutterEventTimestamp); 
	    		CommsThread.TM.Stop(CommsThread.TakePictureMsID);   
	    		CommsThread.TM.Start(CommsThread.PostProcessJPEGMsID);
	    		Message timeMessage = new Message();
	    		timeMessage.what = TIME_ID;
	            myUpdateHandler.sendMessage(timeMessage);
	    	}
	    };
	       
	    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
	        @Override
	        public void onManagerConnected(int status) {
	            switch (status) {
	                case LoaderCallbackInterface.SUCCESS:
	                {
	                	TimeMeasurement.isOpenCVLoaded = true;
	                    Log.i(TAG, "OpenCV loaded successfully");
	                    
	                } break;
	                default:
	                {
	                    super.onManagerConnected(status);
	                    
	                } break;
	            }
	        }
	    };   
	    
		//Handler a socketüzenet számára
	 	private Handler myUpdateHandler = new Handler() {
	 		@Override
	 	    public void handleMessage(Message msg) {
	 	        switch (msg.what) {
	 	        case MSG_ID:
	 	            TextView tv = (TextView) findViewById(R.id.TextView_receivedme);
	 	            tv.setText(mClientMsg);
	 	            break;
	 	       case TIME_ID:
		        	TextView tv2 = (TextView) findViewById(R.id.TextView_timegot);
		        	tv2.setText(current_time);
		        	break;
	 	        default:
	 	        	
	 	            break;
	 	        }
	 	        super.handleMessage(msg);
	 	    }
	 	   };
	    
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		final Button btnHttpGet = (Button) findViewById(R.id.btnHttpGet);
		btnHttpGet.setOnClickListener(new OnClickListener(){
			@Override
			public void onClick (View arg0) {
				new Thread() {
					public void run() {
						httpReg();
					}
				}.start();
			}
		});
		
		//take-picture button
		/*final Button captureButton = (Button) findViewById(R.id.buttonPhoto);
		captureButton.setOnClickListener(
				new View.OnClickListener() {
					
					@Override
					public void onClick(View v) {
						mCamera.takePicture(null, null, mPicture);
						//captureButton.setEnabled(false);
					}
				});	*/
		
		mCamera = Camera.open();

// eredeti elõnézet
		
		mPreview = new CameraPreview(this, mCamera);
		FrameLayout preview=(FrameLayout) findViewById(R.id.camera_preview);
		preview.addView(mPreview);
		
		// az elõnézeti képet így nem látjuk
		/*SurfaceView mview = new SurfaceView(getBaseContext());
		try {
			Camera.Parameters parameters = mCamera.getParameters();
			parameters.setFlashMode(Camera.Parameters.FLASH_MODE_AUTO);
			parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
			parameters.setAntibanding(Camera.Parameters.ANTIBANDING_AUTO);

			
			mCamera.setParameters(parameters);
			
			mCamera.setPreviewDisplay(mview.getHolder());
			mCamera.startPreview();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			Log.e("Creating preview", "Error while creating preview " + e.toString());
			//e.printStackTrace();
		}*/
		
		//thread a socket üzenetek számára
			myCommsThread = new Thread(new CommsThread(myUpdateHandler, mCamera, mPicture, mShutter, ss));
			myCommsThread.start();	
	}
	
    @Override
    protected void onResume()
    {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_3, this, mLoaderCallback);
    }
	
	@Override
	protected void onStop(){
		if(mCamera != null)
			{
				mCamera.release();
			}
		super.onStop();
		myCommsThread.interrupt(); //a socketkapcsolatra várakozó thread-et hogyan érdemes kezelni?
	}
	
	private String getLocalIpAddress() {
		try {
			for (Enumeration en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
				NetworkInterface intf = (NetworkInterface) en.nextElement();
				for (Enumeration enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
					InetAddress inetAddress = (InetAddress) enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress() && InetAddressUtils.isIPv4Address(inetAddress.getHostAddress())) {
						// temporary (?) solution to use only ipv4 address
						return inetAddress.getHostAddress().toString();
					}
				}
			}
		} catch (SocketException ex) {
			//Log.e(LOG_TAG, ex.toString());
		}
		return null;
	} 
	
	private void httpReg()
	{
		AndroidHttpClient httpClient = null;
		String IpAddress = new String(getLocalIpAddress());
		try{
			httpClient = AndroidHttpClient.newInstance("Android");
			HttpGet httpGet = new HttpGet("http://avalon.aut.bme.hu/~kristof/smeyel/smeyel_reg.php?IP="+IpAddress);
			final String response = httpClient.execute(httpGet, new BasicResponseHandler());
			runOnUiThread(new Runnable() {
				@Override
				public void run()
				{
					Toast.makeText(MainActivity.this, response, Toast.LENGTH_LONG).show();
				}
			});
		} catch (IOException e)
		{
			e.printStackTrace();
		} finally{
			if (httpClient != null)
				httpClient.close();
		}
	}
}
