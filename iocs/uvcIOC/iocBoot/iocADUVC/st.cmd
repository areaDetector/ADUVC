#!../../bin/linux-x86_64/uvcApp

errlogInit(20000)

< envPaths
#epicsThreadSleep(20)
dbLoadDatabase("$(TOP)/dbd/uvcApp.dbd")
uvcApp_registerRecordDeviceDriver(pdbbase) 

# Prefix for all records
epicsEnvSet("PREFIX", "XF:10IDC-BI{UVC-Cam:1}")
# The port name for the detector
epicsEnvSet("PORT",   "UVC1")
# The queue size for all plugins
epicsEnvSet("QSIZE",  "20")
# The maximim image width; used for row profiles in the NDPluginStats plugin
epicsEnvSet("XSIZE",  "640")
# The maximim image height; used for column profiles in the NDPluginStats plugin
epicsEnvSet("YSIZE",  "480")
# The framerate at which the stream will operate
epicsEnvSet("FRAMERATE", "30");
# The maximum number of time seried points in the NDPluginStats plugin
epicsEnvSet("NCHANS", "2048")
# The maximum number of frames buffered in the NDPluginCircularBuff plugin
epicsEnvSet("CBUFFS", "500")
# The search path for database files
epicsEnvSet("EPICS_DB_INCLUDE_PATH", "$(ADCORE)/db")
# Size of data allowed 
epicsEnvSet("EPICS_CA_MAX_ARRAY_BYTES", 20000000)
#epicsThreadSleep(15)

#/*
# * Constructor for ADUVC driver. Most params are passed to the parent ADDriver constructor. 
# * Connects to the camera, then gets device information, and is ready to aquire images.
# * 
# * @params: portName -> port for NDArray recieved from camera
# * @params: serial -> serial number of device to connect to
# * @params: productID -> id number for device to connect to
# * @params: framerate -> framerate at which camera should operate
# * @params: xsize -> width of image
# * @params: ysize -> height of image
# * @params: maxBuffers -> max buffer size for NDArrays
# * @params: maxMemory -> maximum memory allocated for driver
# * @params: priority -> what thread priority this driver will execute with
# * @params: stackSize -> size of the driver on the stack
# */

# If searching for device by serial number, put 0 and 0 for vendor/productID
# ADUVCConfig(const char* portName, const char* serial, int productID, int framerate, int xsize, int ysize, int maxBuffers, size_t maxMemory, int priority, int stackSize)
#ADUVCConfig("$(PORT)", "10e536e9e4c4ee70", 0, $(FRAMERATE), $(XSIZE), $(YSIZE), 0, 0, 0, 0)
#epicsThreadSleep(2)

# If searching for device by product ID put "" or empty string for serial number
ADUVCConfig("$(PORT)", "", 25344, $(FRAMERATE), $(XSIZE), $(YSIZE), 0, 0, 0, 0)
epicsThreadSleep(2)

asynSetTraceIOMask($(PORT), 0, 2)
#asynSetTraceMask($(PORT), 0, 0xff)

dbLoadRecords("$(ADCORE)/db/ADBase.template", "P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")
dbLoadRecords("$(ADUVC)/db/ADUVC.template","P=$(PREFIX),R=cam1:,PORT=$(PORT),ADDR=0,TIMEOUT=1")

#
# Create a standard arrays plugin, set it to get data from Driver.
# int NDStdArraysConfigure(const char *portName, int queueSize, int blockingCallbacks, const char *NDArrayPort, int NDArrayAddr, int maxBuffers, size_t maxMemory,
#                          int priority, int stackSize, int maxThreads)
NDStdArraysConfigure("Image1", 3, 0, "$(PORT)", 0)
dbLoadRecords("$(ADCORE)/db/NDStdArrays.template", "P=$(PREFIX),R=image1:,PORT=Image1,ADDR=0,NDARRAY_PORT=$(PORT),TIMEOUT=1,TYPE=Int16,FTVL=SHORT,NELEMENTS=6000000")

#
# Load all other plugins using commonPlugins.cmd
< $(ADCORE)/iocBoot/commonPlugins.cmd

set_requestfile_path("$(ADUVC)/uvcApp/Db")

#asynSetTraceMask($(PORT),0,0x09)
#asynSetTraceMask($(PORT),0,0x11)
iocInit()

# save things every thirty seconds
create_monitor_set("auto_settings.req", 30, "P=$(PREFIX)")
