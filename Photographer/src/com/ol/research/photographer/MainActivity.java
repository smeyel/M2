package com.ol.research.photographer;


import java.io.IOException;
import java.net.InetAddress;
import java.net.NetworkInterface;
import java.net.ServerSocket;
import java.net.SocketException;
import java.util.Calendar;
import java.util.Enumeration;

import org.apache.http.client.methods.HttpGet;
import org.apache.http.impl.client.BasicResponseHandler;
import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;

import android.app.Activity;
import android.content.Intent;
import android.hardware.Camera;
import android.hardware.Camera.PictureCallback;
import android.hardware.Camera.ShutterCallback;
import android.net.http.AndroidHttpClient;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {

	 Camera mCamera;
	
	 static ServerSocket ss = null;
	 static String mClientMsg = "";
	 Thread myCommsThread = null;
	 String current_time = null;
	 protected static final int MSG_ID = 0x1337;
	 public static final int SERVERPORT = 6000;
	 protected static final int TIME_ID = 0x1338;
	 
	 static byte[] lastPhotoData;
	 
	// static Calendar last_midnight;
	 static long calendar_offset;
	 static Calendar right_now;
	 static long timestamp;
	  
	 private PictureCallback mPicture = new PictureCallback() {

	        @Override
	        public void onPictureTaken(byte[] data, Camera camera) {
	        	TempTickCountStorage.OnPictureTakenEvent = TempTickCountStorage.GetTimeStamp();
	        	//SD kártyára lementés
	        	/*String pictureFile = Environment.getExternalStorageDirectory().getPath()+"/custom_photos"+"/__1.jpg";
	            try {
	                FileOutputStream fos = new FileOutputStream(pictureFile);
	                fos.write(data);
	                fos.close();   
	                
	            } catch (FileNotFoundException e) {
	                Log.d("Photographer", "File not found: " + e.getMessage());
	            } catch (IOException e) {
	                Log.d("Photographer", "Error accessing file: " + e.getMessage());
	            }
	            Log.v("Photographer", "Picture saved at path: " + pictureFile);*/
	            
	            Intent intent = new Intent(MainActivity.this, SendImageService.class);
				//intent.putExtra("BYTE_ARRAY", data);
	            lastPhotoData = data;

				/*synchronized(this)
				{
					try {
						wait();
					} catch (InterruptedException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}*/
				intent.putExtra("TIMESTAMP",timestamp);
				startService(intent);            	            
	        }
	    };
	    
	    private ShutterCallback shutter = new ShutterCallback()
	    {
	    	@Override
	    	public void onShutter()
	    	{
    			TempTickCountStorage.OnShutterEvent = TempTickCountStorage.GetTimeStamp();
	    		//right_now = Calendar.getInstance();
	    		//millis_since_midnight = (right_now.getTimeInMillis() + calendar_offset) % (24 * 60 * 60 * 1000);
	    		timestamp = TempTickCountStorage.GetTimeStamp();
	            current_time = String.valueOf(timestamp); 
	    		
	           /* synchronized(this)
				{
					notify();
				}*/
	    		
	    		Message m = new Message();
	            m.what = TIME_ID;
	            //m.obj = millis_since_midnight;
	            myUpdateHandler.sendMessage(m);
	    	}
	    };
	    
	   
  
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		
		
		//String timebuff = rightNow.getInstance();
		
		// EZ ROSSZ!!!!!!!!
		/*last_midnight = Calendar.getInstance();
		//rightNow.set(0, 0, 0);
		int year = last_midnight.get(Calendar.YEAR);
		int month = last_midnight.get(Calendar.MONTH);
		int day = last_midnight.get(Calendar.DAY_OF_MONTH);
		last_midnight.set(year,month,day,0,0); //ms 1970 óta
		
		long[] midnight_array = new long[20];*/
		
		/*for(int i=0; i<20; i++)
		{
			last_midnight = Calendar.getInstance();
			//rightNow.set(0, 0, 0);
			year = last_midnight.get(Calendar.YEAR);
			month = last_midnight.get(Calendar.MONTH);
			day = last_midnight.get(Calendar.DAY_OF_MONTH);
			last_midnight.set(year,month,day,0,0); //ms 1970 óta
			midnight_array[i]= last_midnight.getTimeInMillis();
		}*/
		
		right_now = Calendar.getInstance();
		calendar_offset = right_now.get(Calendar.ZONE_OFFSET) + right_now.get(Calendar.DST_OFFSET);
		
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
//		mPreview = new CameraPreview(this, mCamera);
//		preview=(FrameLayout) findViewById(R.id.cameraPreview);
//		preview.addView(mPreview);
		
		// az elõnézeti képet így nem látjuk
		SurfaceView mview = new SurfaceView(getBaseContext());
		try {
			mCamera.setPreviewDisplay(mview.getHolder());
			mCamera.startPreview();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		//thread a socket üzenetek számára
		myCommsThread = new Thread(new CommsThread(myUpdateHandler, mCamera, mPicture, shutter));
		myCommsThread.start();	
	}

    private static final String  TAG = "TMEAS";

    private BaseLoaderCallback mLoaderCallback = new BaseLoaderCallback(this) {
        @Override
        public void onManagerConnected(int status) {
            switch (status) {
                case LoaderCallbackInterface.SUCCESS:
                {
                	TempTickCountStorage.isOpenCVLoaded = true;
                    Log.i(TAG, "OpenCV loaded successfully");

                    double freq = Core.getTickFrequency();	// May change!!! OK to poll it every time? (And accept is equals at begin and end...) 
                    Log.i(TAG,"getTickFrequency() == "+freq);
                    long prevTickCount = 0;
                    for(int i=0; i<10; i++)
                    {
                        long currentTickCount = TempTickCountStorage.GetTimeStamp();
                        long delta = currentTickCount - prevTickCount;
                        prevTickCount = currentTickCount;
                        Log.i(TAG,"delta microseconds: "+delta);
                    }
                } break;
                default:
                {
                    super.onManagerConnected(status);
                } break;
            }
        }
    };
	
	
    @Override
    public void onResume()
    {
        super.onResume();
        OpenCVLoader.initAsync(OpenCVLoader.OPENCV_VERSION_2_4_3, this, mLoaderCallback);

//        timeMeasurement = new TimeMeasurement();
//        timeMeasurement.loadOpenCVAsync(this);
    }
	
	@Override
	protected void onStop(){
		if(mCamera != null)
			{
				mCamera.release();
			}
		super.onStop();
		myCommsThread.interrupt(); //a socketkapcsolatra várakozó thread-et hogyan érdemes kezelni?
		
		try {
			ss.close();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	  //Handler a socketüzenet számára
 	Handler myUpdateHandler = new Handler() {
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
 	       /*case 0x1339:
 	    	    TextView tv3 = (TextView) findViewById(R.id.TextView_current_time);
	        	tv3.setText(msg.arg1);*/
 	        default:
 	        	
 	            break;
 	        }
 	        super.handleMessage(msg);
 	    }
 	   };
	
	
	
	public String getLocalIpAddress() {
		try {
			for (Enumeration en = NetworkInterface.getNetworkInterfaces(); en.hasMoreElements();) {
				NetworkInterface intf = (NetworkInterface) en.nextElement();
				for (Enumeration enumIpAddr = intf.getInetAddresses(); enumIpAddr.hasMoreElements();) {
					InetAddress inetAddress = (InetAddress) enumIpAddr.nextElement();
					if (!inetAddress.isLoopbackAddress()) {
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
