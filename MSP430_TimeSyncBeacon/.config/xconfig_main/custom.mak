## THIS IS A GENERATED FILE -- DO NOT EDIT
.configuro: .libraries,430 linker.cmd \
  package/cfg/main_p430.o430 \

linker.cmd: package/cfg/main_p430.xdl
	$(SED) 's"^\"\(package/cfg/main_p430cfg.cmd\)\"$""\"D:/BME/PhD/SMEyeL/new_workspace/M2/MSP430_TimeSyncBeacon/.config/xconfig_main/\1\""' package/cfg/main_p430.xdl > $@
