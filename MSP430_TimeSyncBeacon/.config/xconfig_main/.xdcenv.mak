#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = C:/ti/ccs5/grace_1_10_00_17/packages;D:/BME/PhD/SMEyeL/new_workspace/M2/MSP430_TimeSyncBeacon/.config
override XDCROOT = C:/ti/ccs5/xdctools_3_22_04_46
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = C:/ti/ccs5/grace_1_10_00_17/packages;D:/BME/PhD/SMEyeL/new_workspace/M2/MSP430_TimeSyncBeacon/.config;C:/ti/ccs5/xdctools_3_22_04_46/packages;..
HOSTOS = Windows
endif
