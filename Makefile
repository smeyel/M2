all: Framework ImageProcessing M2Host Tracking
 
Framework:
	make -C Framework/
	
ImageProcessing:
	make -C ImageProcessing/
	
M2Host: Tracking ImageProcessing Framework
	make -C M2Host/
	
Tracking:
	make -C Tracking/

CamClient: Framework ImageProcessing Tracking
	make -C CamClient/
	
clean:
	make -C Framework/ clean
	make -C ImageProcessing/ clean
	make -C M2Host/ clean
	make -C Tracking/ clean
	make -C CamClient/ clean
	
.PHONY: all Framework ImageProcessing M2Host Tracking CamClient clean
