CC = g++
AR = ar
CFLAGS = 	-c -Wall \
			-I include/ \
			-I ../Framework/libCommunication/include \
			-I ../Framework/libLogConfigTime/include \
			-I ../Framework/libPlatformSpecifics/include \
			-I ../Framework/libCameraAbstraction/include \
			-I ../Framework/libVideoInput/include \
			-I ../Tracking/lib3dWorld/include/ \
			-I ../ImageProcessing/libTimeSyncBeacon/include/


LIBS =	../Framework/libCameraAbstraction/libCameraAbstraction.a \
		../Framework/libLogConfigTime/libLogConfigTime.a \
		../Framework/libCommunication/libCommunication.a \
		../Framework/libPlatformSpecifics/libPlatformSpecifics.a \
		../Framework/libVideoInput/libVideoInput.a \
		../Tracking/lib3dWorld/lib3dWorld.a \
		../ImageProcessing/libTimeSyncBeacon/libTimeSyncBeacon.a
								
LDFLAGS = `pkg-config --libs opencv`

all: M2Host

M2HOST_OBJECTS =  src/main.o 	
								

M2Host: $(M2HOST_OBJECTS)
	$(CC) $(M2HOST_OBJECTS) $(LIBS) $(LDFLAGS) -o M2Host

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
	
clean:
	rm -rf src/*.o M2Host
	
.PHONY: all clean