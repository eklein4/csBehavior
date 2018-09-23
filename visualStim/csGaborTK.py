
import datetime

import io
import sys
from psychopy import visual, core, event, data, clock 
import configparser
import numpy as np
import serial 


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
    win_x1=900
    win_y1=900
    init_contrast = 0.0
    init_orientation = 0
    useSerial = 0


# ****** Make a serial object.
if useSerial==1:
    teensyObj = serial.Serial(serialPath,serialBaud)
    teensyObj.close()
    teensyObj.open()


# ***** make a psychopy/pyglet window
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
gabor_1={'phaseDelta':0.05,'Xpos':0,'Ypos':0,'spFreq':[4,0],'mask':'gauss',
    'size':(1.0,1.0),'contrast':init_contrast,'orientation':init_orientation}

#create some stimuli
grating1= visual.GratingStim(win=mywin, mask=gabor_1['mask'], 
    pos=[gabor_1['Xpos'],gabor_1['Ypos']],sf=gabor_1['spFreq'],
    ori=gabor_1['orientation'])
grating1.contrast = gabor_1['contrast']
grating1.autoDraw=True

# ******* This is the program main loop 
while runSession:
    if useSerial==1:
        serTrack=0
        bytesAvail=teensyObj.inWaiting()
        if bytesAvail>0:

            sR=teensyObj.readline().strip().decode()
            sR=sR.split(',')
            if len(sR)==7 and sR[0]=='v':
                if int(sR[2])==999:
                    stopCamera()
                    runSession=0
                    sR[2]=0
                gabor_1['contrast']=int(sR[2])/100
                gabor_1['orientation']=int(sR[1])
                gabor_1['spFreq']=int(sR[3])/100
                
                serTrack=1
                
                grating1.contrast = gabor_1['contrast']
                grating1.ori=gabor_1['orientation']
                grating1.size = gabor_1['size']

 

    if startFlag==0:
        timeOffset=clock.getTime()
        startFlag=1
    grating1.phase += gabor_1['phaseDelta']
    curTime=clock.getTime()-timeOffset
    # grating1.draw()


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

# end session

exp.close()

cStr=datetime.datetime.now().strftime("%Y%m%d%H%M%S")
filename = savePath + animalID + 'stim_' + cStr
exp.saveAsWideText(filename)
mywin.close()
core.quit()