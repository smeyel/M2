package com.ol.research.photographer;

import java.io.IOException;
import java.util.List;

//import android.app.Activity;
import android.content.Context;
import android.hardware.Camera;
import android.hardware.Camera.Size;
import android.util.Log;
//import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class CameraPreview extends SurfaceView implements SurfaceHolder.Callback {
	Camera mCamera;
	SurfaceHolder mHolder;

	public CameraPreview(Context context, Camera camera) {
		super(context);

		mCamera = camera;

		// Install a SurfaceHolder.Callback so we get notified when the
		// underlying surface is created and destroyed.
		mHolder = getHolder();
		mHolder.addCallback(this);

		// deprecated setting, but required on Android versions prior to 3.0
		mHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);

	}
	
	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		try {
			mCamera.setPreviewDisplay(holder);
			mCamera.startPreview();
		} catch (IOException e) {
			Log.d("AutoCamera",
					"Error setting camera preview: " + e.getMessage());
		}

	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		// itt nem kell semmit csinalnunk
	}
	
	
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
		// If your preview can change or rotate, take care of those events here.
		// Make sure to stop the preview before resizing or reformatting it.

	/*	if (mHolder.getSurface() == null) {
			// preview surface does not exist
			return;
		}

		// stop preview before making changes
		try {
			mCamera.stopPreview();
		} catch (Exception e) {
			// ignore: tried to stop a non-existent preview
		}

		// Parameterek beallitasa
		Camera.Parameters parameters = mCamera.getParameters();
		List<Size> cameraPreviewSizes = parameters.getSupportedPreviewSizes();
		for (Size size : cameraPreviewSizes) {
			Log.v("AutoCamera", "cameraPreviewSize: " + size.width + "x"
					+ size.height);
		}
		List<String> focusModes = parameters.getSupportedFocusModes();
		for (String mode : focusModes) {
			Log.v("AutoCamera", "cameraFocusMode: " + mode);
		}
		List<String> flashModes = parameters.getSupportedFlashModes();
		for (String mode : flashModes) {
			Log.v("AutoCamera", "cameraFlashMode: " + mode);
		}

		parameters.setFlashMode(Camera.Parameters.FLASH_MODE_AUTO);
		parameters.setPreviewSize(800, 480);
		parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
		parameters.setJpegQuality(80); // TODO: ez legyen allithato WS-bol
		mCamera.setDisplayOrientation(90); // allo preview

		mCamera.setParameters(parameters);

		// start preview with new settings
		try {
			mCamera.setPreviewDisplay(mHolder);
			mCamera.startPreview();
		} catch (Exception e) {
			Log.d("AutoCamera",
					"Error starting camera preview: " + e.getMessage());
		}*/

	}
}
