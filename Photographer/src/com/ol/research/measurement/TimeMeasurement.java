package com.ol.research.measurement;

import org.opencv.android.BaseLoaderCallback;
import org.opencv.android.LoaderCallbackInterface;
import org.opencv.android.OpenCVLoader;
import org.opencv.core.Core;


import android.content.Context;
import android.util.Log;

public class TimeMeasurement {
	
	//FIXME is OpenCV loaded => maybe init() funcition
	double TickFrequency;	
	public static boolean isOpenCVLoaded =false;
	private static final int MAX_TIMING_INDEX = 1000;
	long[] StartTickValues;
	double[] SumValues; //TODO Overflow?
	int[] NumValues;	
	String[] Names;
	MeasurementLog UsedLog;
	static Context appContext;
	
	public TimeMeasurement()
	{
		StartTickValues = new long[MAX_TIMING_INDEX];
		SumValues = new double[MAX_TIMING_INDEX];
		NumValues = new int[MAX_TIMING_INDEX];
		Names = new String[MAX_TIMING_INDEX];
		for(int i=0; i<MAX_TIMING_INDEX; i++)
		{
			StartTickValues[i] = 0;
			SumValues[i] = 0;
			NumValues[i] = 0;
		}
	}
	
	public void Start(int MeasurementID)
	{
		if (isOpenCVLoaded)
		{
			
			//assert (MeasurementID<0) : "ID can't be a negativ number"; 
			TickFrequency = Core.getTickFrequency();
			StartTickValues[MeasurementID] = Core.getCPUTickCount();
			return;	
		}
		else
		{
			Log.e("openCV", "OpenCV is not loaded!");
			return;
		}
	}
	
	public double Stop(int MeasurementID)
	{
		if (isOpenCVLoaded)
		{
			//assert MeasurementID<0 : "ID can't be a negativ number"; 
			long StopTick = Core.getCPUTickCount();
			TickFrequency = Core.getTickFrequency();
			double divider = TickFrequency / 1000000.0;
			double currentResult = (double)(Math.round((StopTick - StartTickValues[MeasurementID]) / divider) / 1000.0);
			SumValues[MeasurementID] += currentResult;
			NumValues[MeasurementID]++;
			String ActualName = Names[MeasurementID];//FIXME vizsgálni, hogy nevet beállítottak-e
			ElapsedTimeResult Result = new ElapsedTimeResult(ActualName, currentResult);
			UsedLog.push(Result);//FIXME vizsgálni, hogy log-ot beállítottak-e már
			return currentResult; // in milliseconds TODO return value needed?
		}
		else
		{
			Log.e("openCV", "OpenCV is not loaded!");
			return -1;
		}
	}
	
	public static long getTimeStamp()
	{
		if (isOpenCVLoaded)
		{
			double TickFrequency = Core.getTickFrequency();
			double divider = TickFrequency / 1000000.0;
			long tick = Core.getCPUTickCount();
			long timestamp = (long)(Math.round(tick / divider));
			return timestamp;	// Returns in microseconds
		}
		else
		{
			return -1;
		}
	}
	
	public static double calculateIntervall(long StartTick, long FinishTick) //Ticks are in microseconds
	{
		double TimeInterval = (double)(Math.round(FinishTick - StartTick) / 1000.0); // in milliseconds
		return TimeInterval;
	}
	
	public double getAvg(int MeasurementID)
	{
		return SumValues[MeasurementID]/(double)NumValues[MeasurementID];
	}
	
	public void setMeasurementLog(MeasurementLog UsedLog)
	{
		
		this.UsedLog = new MeasurementLog();
		this.UsedLog = UsedLog;
	}
	
	public void setName(int MeasurementID, String Name)
	{
		Names[MeasurementID] = Name;
	}
	
	public void pushIntervallToLog(String Name, double Intervall)
	{
		ElapsedTimeResult Result = new ElapsedTimeResult(Name,Intervall);
		UsedLog.push(Result);
	}
}
