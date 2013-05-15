package com.ol.research.measurement;

import org.json.JSONObject;
/**
 * Interface for measurement results. <br>
 * The class, witch implements it, must create his JSON, to be able to serialize the object.
 * */
public interface MeasurementResult {
	abstract JSONObject getJSON();
}
