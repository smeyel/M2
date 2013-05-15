package com.ol.research.measurement;

import org.json.JSONException;
import org.json.JSONObject;

/**
 * Elapsed time results for the log. It is used by TimeMeasurement, to put the results in the log.
 * */
public class ElapsedTimeResult implements MeasurementResult {
	
		String Name;
		double Result;
	
		ElapsedTimeResult(String Name, double TimeIntervall)
		{
			this.Name = Name;
			this.Result = TimeIntervall;
		}
		
		@Override
		public JSONObject getJSON() {
			JSONObject json = new JSONObject();
			try {
				String temp = new String();
				json.put("measurementname",Name);
				temp = Double.toString(Result);
				json.put("result", temp);
			} catch (JSONException e) {
				e.printStackTrace();
			}			
			return json;
		}	
}
