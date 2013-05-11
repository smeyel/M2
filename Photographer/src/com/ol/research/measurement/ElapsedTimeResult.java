package com.ol.research.measurement;

import org.json.JSONException;
import org.json.JSONObject;
import org.opencv.core.Core;
//import flexjson.JSONSerializer;

import android.util.JsonWriter;
import android.util.Log;

public class ElapsedTimeResult implements MeasurementResult {
	
		//int MeasurementID;
		String Name;
		double Result;
	
		//ElapsedTimeResult(int MeasurementID, double TimeIntervall)
		ElapsedTimeResult(String Name, double TimeIntervall)
		{
			//this.MeasurementID = MeasurementID;
			this.Name = Name;
			this.Result = TimeIntervall;
		}
		
		
		@Override
		public JSONObject getJSON() {
			/*StringBuilder sb = new StringBuilder("{\"measurementid\":\"");
			sb.append(MeasurementID);
			sb.append("\",\"result\":\"");
			sb.append(Result);
			sb.append("\"}#");*/
			JSONObject json = new JSONObject();
			try {
				String temp = new String();
				//temp = Integer.toString(MeasurementID); //if json.put gets string, it will use ""
				//json.put("measurementid",temp);
				json.put("measurementname",Name);
				temp = Double.toString(Result);
				json.put("result", temp);
			} catch (JSONException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}			
			return json;
		}	
}
