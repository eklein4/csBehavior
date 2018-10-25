# csGabor -- cdeister@brown.edu
#
# csGabor is a simple python program that displays visual stimuli whose parameters can be changed over serial.
# It can also trigger acquisiton from a raspberry pi camera. 
# It is fairly specific, but is intended to be run from a 
# dedicated raspberry pi with a thing-printer.com "MangaScreen2" attached. 

from picamera import PiCamera
import datetime
import RPi.GPIO as GPIO
import io
import sys
from psychopy import visual, core, event, data, clock, monitors
import configparser
import numpy as np
import serial 

usedConfig = 0
sesVars = {'useGUI':0,'savePath':'/home/pi/','animalID':'an1','savePath':'/home/pi/captures/',\
'serialPath':"/dev/ttyS0",'serialBaud':115200,'res_X':1640,'res_Y':1232,'frameRate':30,'iso':0,\
'onPin':11,'offPin':12,'win_x1':1000,'win_y1':1000,'init_contrast':1,'init_orientation':0,\
'useSerial':1,'useCam':1,'monWidth':14,'monDistance':20,'curMask':'circle'}


# An optional configuration file can be supplied at startup.
try:
	config = configparser.ConfigParser()
	config.read(sys.argv[1])

	sesVars['useGUI'] = bool(config['settings']['useGUI'])
	sesVars['savePath'] = config['sesVars']['savePath']
	sesVars['animalID'] = config['sesVars']['animalID']
	sesVars['res_X'] = int(config['camera']['res_X'])
	sesVars['res_Y'] = int(config['camera']['res_Y'])
	sesVars['frameRate'] = int(config['camera']['frameRate'])
	sesVars['iso'] = int(config['camera']['iso'])
	sesVars['serialPath'] = config['sesVars']['serialPath']
	sesVars['serialBaud'] = int(config['sesVars']['serialBaud'])
	sesVars['serialPath'] = config['sesVars']['serialPath']
	sesVars['onPin'] = int(config['GPIO']['onPin'])
	sesVars['offPin'] = int(config['GPIO']['offPin'])
	sesVars['win_x1'] = int(config['visual']['win_x1'])
	sesVars['win_y1']= int(config['visual']['win_y1'])
	sesVars['init_contrast'] = float(config['visual']['init_contrast'])
	sesVars['init_orientation'] = int(config['visual']['init_orientation'])
	sesVars['useSerial'] = int(config['sesVars']['useSerial'])
	sesVars['curMask'] = config['sesVars']['curMask']
	sesVars['monDistance'] = int(config['visual']['monDistance'])
	sesVars['monWidth'] = int(config['visual']['monWidth'])
	usedConfig = 1
	print("using config")

except: 
	pass
	print("not using config")

# specify a monitor for psyhcopy
myMon = monitors.Monitor('manga', width=sesVars['monWidth'],\
	distance=sesVars['monDistance'],verbose=True, autoLog=True)

# ******** Make a raspberry pi camera object if using a pi
if sesVars['useCam'] == 1:
	camera = PiCamera()
	camera.resolution = (sesVars['res_X'],sesVars['res_Y'])
	camera.framerate = sesVars['frameRate']
	#camera.exposure = iso

# ******** initialize Pi GPIO To Control Camera
GPIO.setmode(GPIO.BOARD)
GPIO.setup(sesVars['onPin'], GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # 11, GPIO17 is on trigger
GPIO.setup(sesVars['offPin'], GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # 12, GPIO18 is off trigger

# ****** Make a serial object.
if sesVars['useSerial']==1:
	teensyObj = serial.Serial(sesVars['serialPath'],sesVars['serialBaud'])
	teensyObj.close()
	teensyObj.open()

# ***** make a psychopy/pyglet window
cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
mywin = visual.Window([sesVars['win_x1'],sesVars['win_y1']],allowGUI=False, monitor = myMon)
tExp = data.ExperimentHandler(dataFileName=sesVars['animalID'] + '_psychopyData_' + cStr)

# stim 1 params
gabor_1={'phaseDelta':0.02,'Xpos':0.0,'Ypos':0.0,'spFreq':[4,0],'mask':sesVars['curMask'],
	'size':1,'contrast':sesVars['init_contrast'],'orientation':sesVars['init_orientation']}

# local variables for timing etc.
startFlag=0
timeOffset=0
curTime=0
lc=0
serTrack=0
runSession=1
cameraOn = 0


#create some stimuli
grating1= visual.GratingStim(win=mywin,mask=gabor_1['mask'],pos=[gabor_1['Xpos'],gabor_1['Ypos']],\
	sf=gabor_1['spFreq'],ori=gabor_1['orientation'])
grating1.size = gabor_1['size']
grating1.contrast = gabor_1['contrast']
grating1.autoDraw=True

def startCamera(camObj,varDict):
	cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
	output = np.empty((varDict['res_Y'],varDict['res_X'],3),dtype=np.uint16)
	filename = varDict['savePath'] + varDict['animalID'] + '_subjVideo_' + cStr + '.h264'
	camObj.start_recording(filename,sps_timing=True)

def stopCamera(camObj):
	camObj.stop_recording()

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
	tExp.addData('clockTime', curTime)
	tExp.addData('g1_phase', gabor_1['Xpos'])
	tExp.addData('g1_spFreq', gabor_1['spFreq'])
	tExp.addData('g1_size', gabor_1['size'])
	tExp.addData('g1_contrast', gabor_1['contrast'])
	tExp.addData('serTrack',serTrack)
	tExp.nextEntry()
	
	if len(event.getKeys())>0:
		break
	event.clearEvents()
	cam_onPin=GPIO.input(sesVars['onPin'])
	cam_offPin=GPIO.input(sesVars['offPin'])

	if sesVars['useCam'] ==1 and cameraOn == 0 and cam_onPin==1:
		cameraOn = 1
		startCamera(camera,sesVars)
	elif sesVars['useCam'] ==1 and cameraOn == 1 and cam_offPin==1:
		cameraOn = 0
		stopCamera(camera)
		runSession = 0
	elif sesVars['useCam'] == 0 and cam_offPin==1:
		runSession = 0
	if sesVars['useSerial']==1:
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
				gabor_1['size']=int(sR[7])/10
				gabor_1['Xpos']=int(sR[6])/100
				# gabor_1['phaseDelta'] = float(int(sR[4])/100)
				# gabor_1['Xpos']=float(int(sR[6])/10)
				# print(gabor_1['Xpos'])
				# gabor_1['Ypos']=float(int(sR[5])/10)
				# print(gabor_1['Ypos'])
				# gabor_1['size']=float(int(sR[7])/10)
				# print(gabor_1['Size'])
				
				
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
filename = sesVars['savePath'] + sesVars['animalID'] + 'stim_' + cStr
exp.saveAsWideText(filename)
core.quit()
