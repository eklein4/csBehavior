from picamera import PiCamera
import datetime
import RPi.GPIO as GPIO
import io
import sys
from psychopy import visual, core, event, data, clock, monitors
import configparser
import numpy as np
import serial 

myMon = monitors.Monitor('manga', width=14, distance=20,verbose=True, autoLog=True)

# An optional configuration file can be supplied at startup.
try:
	config = configparser.ConfigParser()
	config.read(sys.argv[1])
	useGUI = int(config['settings']['useGUI'])
	savePath = config['sesVars']['savePath']
	animalID = config['sesVars']['animalID']
	res_X = int(config['camera']['res_X'])
	res_Y = int(config['camera']['res_Y'])
	frameRate = int(config['camera']['frameRate'])
	iso = int(config['camera']['iso'])
	serialPath = config['sesVars']['serialPath']
	serialBaud = int(config['sesVars']['serialBaud'])
	serialPath = config['sesVars']['serialPath']
	onPin = int(config['GPIO']['onPin'])
	offPin = int(config['GPIO']['offPin'])
	win_x1=int(config['visual']['win_x1'])
	win_y1=int(config['visual']['win_y1'])
	init_contrast=float(config['visual']['init_contrast'])
	init_orientation=int(config['visual']['init_orientation'])
	useSerial=int(config['sesVars']['useSerial'])


	if useGUI==1:
		print("using gui and user config")
	if useGUI == 0:
		print("not using gui and user config")
except: 
	useGUI=0
	print("not using gui; or config")
	savePath = '/home/pi/'
	animalID = 'animalX'
	res_X = 1640
	res_Y = 1232
	frameRate = 30
	iso = 0
	serialPath = "/dev/ttyS0"
	serialBaud = 115200
	onPin = 11
	offPin = 12
	win_x1=1000
	win_y1=1000
	init_contrast = 0
	init_orientation = 0
	useSerial = 1
	useCam = 0

# ******** Make a raspberry pi camera object if using a pi
if useCam == 1:
	camera = PiCamera()
	camera.resolution = (res_X,res_Y)
	camera.framerate = frameRate
	#camera.exposure = iso

# ******** initialize Pi GPIO To Control Camera
GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # 11, GPIO17 is on trigger
GPIO.setup(12, GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # 12, GPIO18 is off trigger

# ****** Make a serial object.
if useSerial==1:
	teensyObj = serial.Serial(serialPath,serialBaud)
	teensyObj.close()
	teensyObj.open()


# ***** make a psychopy/pyglet window
mywin = visual.Window([win_x1,win_y1],allowGUI=False, monitor = myMon)
exp = data.ExperimentHandler(dataFileName="ydo")


# timing etc.
startFlag=0
timeOffset=0
curTime=0
lc=0
serTrack=0
runSession=1
cameraOn = 0


# stim 1 params
gabor_1={'phaseDelta':0.02,'Xpos':0.0,'Ypos':0.0,'spFreq':[4,0],'mask':'circle',
	'size':1,'contrast':init_contrast,'orientation':init_orientation}


#create some stimuli
grating1= visual.GratingStim(win=mywin,mask=gabor_1['mask'],pos=[gabor_1['Xpos'],gabor_1['Ypos']],\
	sf=gabor_1['spFreq'],ori=gabor_1['orientation'])
grating1.size = gabor_1['size']
grating1.contrast = gabor_1['contrast']
grating1.autoDraw=True

# grating2 = visual.GratingStim(win=mywin, mask='circle', size=9, pos=[-4,0], sf=3)
# grating2.contrast = gabor_2['contrast']
# grating2.autoDraw=True

def startCamera():
	cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
	output = np.empty((res_Y,res_X,3),dtype=np.uint16)
	filename = savePath + animalID + 'video_' + cStr + '.h264'
	camera.start_recording(filename,sps_timing=True)

def stopCamera():
	camera.stop_recording()

# ******* This is the program main loop 
while runSession:

	if startFlag==0:
		timeOffset=clock.getTime()
		startFlag=1
	grating1.phase += gabor_1['phaseDelta']
	curTime=clock.getTime()-timeOffset
	mywin.flip()
	lc=lc+1

	# save
	exp.addData('clockTime', curTime)
	exp.addData('g1_phase', gabor_1['Xpos'])
	exp.addData('g1_spFreq', gabor_1['spFreq'])
	exp.addData('g1_size', gabor_1['size'])
	exp.addData('g1_contrast', gabor_1['contrast'])
	exp.addData('serTrack',serTrack)
	exp.nextEntry()
	
	if len(event.getKeys())>0:
		break
	event.clearEvents()
	cam_onPin=GPIO.input(onPin)
	cam_offPin=GPIO.input(offPin)

	if useCam ==1 and cameraOn == 0 and cam_onPin==1:
		cameraOn = 1
		startCamera()
	elif useCam ==1 and cameraOn == 1 and cam_offPin==1:
		cameraOn = 0
		stopCamera()
		runSession = 0
	elif useCam == 0 and cam_offPin==1:
		runSession = 0
	if useSerial==1:
		serTrack=0
		bytesAvail=teensyObj.inWaiting()
		if bytesAvail>0:

			sR=teensyObj.readline().strip().decode()
			sR=sR.split(',')
			if len(sR)==8 and sR[0]=='v':
				if int(sR[2])==999:
					# runSession=0
					sR[2]=0
				gabor_1['contrast']=int(sR[2])/100
				gabor_1['orientation']=int(sR[1])
				gabor_1['spFreq']=int(sR[3])
				gabor_1['phaseDelta'] = int(sR[4])/100
				gabor_1['Xpos']=int(sR[6])/10
				gabor_1['Ypos']=int(sR[5])/10
				gabor_1['size']=int(sR[7])/10
				
				
				serTrack=1
				
				grating1.contrast = gabor_1['contrast']
				grating1.ori=gabor_1['orientation']
				grating1.size = gabor_1['size']
				grating1.pos=[gabor_1['Xpos'],gabor_1['Ypos']]
				grating1.sf=[gabor_1['spFreq'],0]

# end session
mywin.close()
exp.close()

cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
filename = savePath + animalID + 'stim_' + cStr
exp.saveAsWideText(filename)
core.quit()
