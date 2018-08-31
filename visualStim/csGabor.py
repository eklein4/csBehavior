from picamera import PiCamera
import datetime
import RPi.GPIO as GPIO
import io
import sys
from psychopy import visual, core, event, data, clock 

import numpy as np
import serial 

# ******** Make a raspberry pi camera object if using a pi
camera = PiCamera()
camera.resolution = (1640,1232)
camera.framerate = 30
camera.exposure_mode='off'
camera.iso=0

# ******** initialize Pi GPIO To Control Camera
GPIO.setmode(GPIO.BOARD)
GPIO.setup(11, GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # 11, GPIO17 is on trigger
GPIO.setup(12, GPIO.IN,pull_up_down=GPIO.PUD_DOWN) # 12, GPIO18 is off trigger

# ****** Make a serial object.
teensyObj = serial.Serial("/dev/ttyS0",115200)
teensyObj.close()
teensyObj.open()


# ***** make a psychopy/pyglet window
win_x1=500
win_y1=500
mywin = visual.Window([win_x1,win_y1],allowGUI=False)
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
g1={'phaseDelta':0.05,'Xpos':0,'Ypos':0,'spFreq':[4,0],'mask':'gauss',
    'size':(1.0,1.0),'contrast':0.0,'orientation':0}

#create some stimuli
grating1= visual.GratingStim(win=mywin, mask=g1['mask'], 
    pos=[g1['Xpos'],g1['Ypos']],sf=g1['spFreq'],
    ori=g1['orientation'])
grating1.contrast = g1['contrast']
grating1.autoDraw=True

def startCamera():
    cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
    output = np.empty((1232,1640,3),dtype=np.uint8)
    filename = '/home/pi/Desktop/Captures/video_'+cStr+'.h264'
    camera.start_recording(filename,sps_timing=True)

def stopCamera():
    camera.stop_recording()

# ******* This is the program main loop 
while runSession:
    cam_onPin=GPIO.input(11)
    cam_offPin=GPIO.input(12)

    if cameraOn == 0 and cam_onPin==1:
        cameraOn = 1
        startCamera()
    elif cameraOn == 1 and cam_offPin==1:
        cameraOn = 0
        stopCamera()

    serTrack=0
    bytesAvail=teensyObj.inWaiting()
    if bytesAvail>0:

        sR=teensyObj.readline().strip().decode()
        sR=sR.split(',')
        if len(sR)==7 and sR[0]=='v':
            if int(sR[2])==999:
                runSession=0
                sR[2]=0
            g1['contrast']=int(sR[2])/100
            g1['orientation']=int(sR[1])
            g1['spFreq']=int(sR[3])/100
            
            serTrack=1
            
            grating1.contrast = g1['contrast']
            grating1.ori=g1['orientation']
            grating1.size = g1['size']

 

    if startFlag==0:
        timeOffset=clock.getTime()
        startFlag=1
    grating1.phase += g1['phaseDelta']
    curTime=clock.getTime()-timeOffset
    # grating1.draw()


    mywin.flip()
    lc=lc+1

    # save
    exp.addData('clockTime', curTime)
    exp.addData('g1_phase', g1['Xpos'])
    exp.addData('g1_spFreq', g1['spFreq'])
    exp.addData('g1_size', g1['size'])
    exp.addData('g1_contrast', g1['contrast'])
    exp.addData('serTrack',serTrack)
    exp.nextEntry()
    if len(event.getKeys())>0:
        break
    event.clearEvents()

# end session

exp.close()

cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
filename = '/home/pi/Desktop/Captures/stim_'+cStr
exp.saveAsWideText(filename)
mywin.close()
core.quit()
