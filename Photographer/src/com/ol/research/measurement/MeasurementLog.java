package com.ol.research.measurement;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Iterator;
import java.util.Vector;

import org.json.JSONArray;
import org.json.JSONObject;

import android.util.Log;

/**
 * Logs the results of the measurements. With WriteJSON() will be the log serialized and sent through the given stream.
 * */
public class MeasurementLog {
	
	Vector<MeasurementResult> ResultVector;
	public MeasurementLog()
	{
		ResultVector = new Vector<MeasurementResult> ();
	}
	
	void push(MeasurementResult Result)
	{
		ResultVector.add(Result);
	}
	void clear()
	{
		ResultVector.clear();
	}
	
	/**Sends the log.*/
	public void WriteJSON(OutputStream os)
	{
		try {
			
			JSONArray jArray = new JSONArray();
			JSONObject json = new JSONObject();
			Iterator<MeasurementResult> i = ResultVector.iterator();
			while(i.hasNext())
			{
				json = i.next().getJSON();
				jArray.put(json);
			}
			String JSONArrayString = new String();
			JSONArrayString = jArray.toString();
			StringBuilder sb = new StringBuilder("{\"type\":\"measurementlog\",\"items\":"); 
			sb.append(JSONArrayString);
			sb.append("}#");
			String JSONMessage = new String();
			JSONMessage = sb.toString();
			DataOutputStream output = new DataOutputStream(os);     
            output.writeUTF(JSONMessage);
            output.flush();
            ResultVector.clear(); //TODO necessarily?
						
		} catch (IOException e) {
			 Log.e("JSON Parser", "Error parsing data " + e.toString());
		}
	}
}
