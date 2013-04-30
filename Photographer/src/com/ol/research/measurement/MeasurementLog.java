package com.ol.research.measurement;

import java.io.DataOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.util.Iterator;
import java.util.Vector;

import org.json.JSONArray;
import org.json.JSONObject;


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
	
	public void WriteJSON(OutputStream os)
	{
		try {
			
			JSONArray jArray = new JSONArray();
			JSONObject json = new JSONObject();
			Iterator<MeasurementResult> i = ResultVector.iterator();
			while(i.hasNext())
			{
				json = i.next().WriteJSON();
				jArray.put(json);
			}
			String JSONArrayString = new String();
			JSONArrayString = jArray.toString();
			StringBuilder sb = new StringBuilder("{\"type\":\"measurementlog\":"); 
			sb.append(JSONArrayString);
			sb.append("}#");
			String JSONMessage = new String();
			JSONMessage = sb.toString();
			DataOutputStream output = new DataOutputStream(os);     
            output.writeUTF(JSONMessage);
            output.flush();
            ResultVector.clear(); //TODO necessarily?
						
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
}
