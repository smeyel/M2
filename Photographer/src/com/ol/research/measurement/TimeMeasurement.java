package com.ol.research.measurement;

import org.opencv.core.Core;


import android.content.Context;
import android.util.Log;

/**
 * Responsible for measuring and logging time intervals.<br>
 * <br>
 * Initialization:<br>
 * Before using this class, the OpenCV has to be initialized, and the public variable "isOpenCVLoaded" must be set on true!<br>
 * After creating a TimeMeasurement object you have to set the names of the measurements with setName() function.<br>
 * The measurement log must be set with the setMeasurementLog(). Therefore a MeasurementLog object is needed. <br>
 * 
 * */


public class TimeMeasurement {
	
	//FIXME is OpenCV loaded => maybe init() funcition
	
	/**Must be set on true, when OpenCV is initialized.*/
	public static boolean isOpenCVLoaded =false;
	double TickFrequency;
	private static final int MAX_TIMING_INDEX = 1000;
	long[] StartTickValues;
	double[] SumValues; // possibility of overflow?
	int[] NumValues;	
	String[] Names;
	MeasurementLog UsedLog;
	static Context appContext;
	
	/**Creates the needed intern arrays for the TimeMeasurenet object. */
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
	
	/** * Before using the TimeMeasurement object, the leg has to be set with this function.
	 * @param UsedLog The MeasurementLog must be given*/
	public void setMeasurementLog(MeasurementLog UsedLog)
	{
		
		this.UsedLog = new MeasurementLog();
		this.UsedLog = UsedLog;
	}
	
	/**The name of the measurement has to be set before the usage of the TimeMeasurement object.
	 * @param MeasurmentID ID of measurement
	 * @param Name Name of measurement*/
	public void setName(int MeasurementID, String Name)
	{
		Names[MeasurementID] = Name;
	}
	
	/**The time measurement interval begins with this function.
	 * @param MeasurementID ID of measurement*/
	public void Start(int MeasurementID)
	{
		if (isOpenCVLoaded)
		{ 
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
	
	/** The time measurement interval ends with this function, the measurementresult will be logged.
	 * @param MeasurementID ID of measurement
	 * @return Measured time intervall if OpenCV is loeaded, otherwise -1.*/
	public double Stop(int MeasurementID)
	{
		if (isOpenCVLoaded)
		{
			long StopTick = Core.getCPUTickCount();
			TickFrequency = Core.getTickFrequency();
			double divider = TickFrequency / 1000000.0;
			double currentResult = (double)(Math.round((StopTick - StartTickValues[MeasurementID]) / divider) / 1000.0);
			SumValues[MeasurementID] += currentResult;
			NumValues[MeasurementID]++;
			String ActualName = Names[MeasurementID];
			if(ActualName.length() == 0)
			{
				Log.e("TimeMeasurement", "The name of the measurement was not set!");
				Log.e("TimeMeasurement", "The measurment result was not logged!");
			}
			else
			{
				ElapsedTimeResult Result = new ElapsedTimeResult(ActualName, currentResult);
				UsedLog.push(Result); //TODO Prove, if the log is set.
			}
			return currentResult;
		}
		else
		{
			Log.e("openCV", "OpenCV is not loaded!");
			return -1;
		}
	}
	
	/**Determines the actual timestamp in milliseconds.
	 * @return Timestamp, if OpenCV is loaded, -1 if it is not.*/
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
	
	/**Determines the time interval between two timestamps.
	 * @param StartTimeStamp time stamp, when the interval starts
	 * @param FinishTimeStampú time stamp, when the interval ends
	 * @return Time interval in milliseconds*/
	public static double calculateIntervall(long StartTimeStamp, long FinishTimeStamp) //Ticks are in microseconds
	{
		double TimeInterval = (double)(Math.round(FinishTimeStamp - StartTimeStamp) / 1000.0); // in milliseconds
		return TimeInterval;
	}
	
	
	/** Determines the average of the given measurement.
	 * @param MeasurementID ID of measurement
	 * @return Average of measurement
	 * */
	public double getAvg(int MeasurementID)
	{
		return SumValues[MeasurementID]/(double)NumValues[MeasurementID];
	}
	
	
	/**Time Intervals can be saved to the log with this function.
	 * @param Name Name of time interval
	 * @param Intervall Time interval*/
	public void pushIntervallToLog(String Name, double Intervall)
	{
		ElapsedTimeResult Result = new ElapsedTimeResult(Name,Intervall);
		UsedLog.push(Result);
	}
}
