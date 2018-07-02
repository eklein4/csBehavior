import sys
from psychopy import visual, core, event, data, clock 

import numpy as np
import serial 

teensyObj = serial.Serial("/dev/tty.usbmodem3650661",19200)
teensyObj.close()
teensyObj.open()
teensyObj.write(bytes(b'a0>'))


#create a window
win_x1=600
win_y1=600


mywin = visual.Window([win_x1,win_y1],monitor="testMonitor",fullscr=False, units="deg")
exp = data.ExperimentHandler(dataFileName="ydo")


# timing etc.
startFlag=0
timeOffset=0
curTime=0
lc=0
serTrack=0

# stim 1 params
g1={'phaseDelta':0.05,'Xpos':0,'Ypos':0,'spFreq':0.7,'mask':'gauss',
    'size':(10,10),'contrast':1.0,'orientation':0}

#create some stimuli
grating1= visual.GratingStim(win=mywin, mask=g1['mask'], 
    pos=[g1['Xpos'],g1['Ypos']],sf=g1['spFreq'],colorSpace='rgb',maskParams={'sd': 5},
    ori=g1['orientation'])


# draw stim refresh monitor
while True: #this creates a never-ending loop
    serTrack=0
    bytesAvail=teensyObj.inWaiting()
    # 4,3,5,6,7
    if bytesAvail>0:

        sR=teensyObj.readline().strip().decode()
        sR=sR.split(',')
        if len(sR)==6 and sR[0]=='v':
            g1['contrast']=int(sR[2])/100
            g1['orientation']=int(sR[1])
            g1['spFreq']=int(sR[3])/100
            serTrack=1
 

    if startFlag==0:
        timeOffset=clock.getTime()
        teensyObj.write(bytes(b'a3>'))
        startFlag=1
    curTime=clock.getTime()-timeOffset
    # if curTime>1 and curTime<2:
    #     g1['contrast']=0.2
    # elif curTime>=2:
    #     g1['contrast']=1.0
    #     g1['orientation']=120
    grating1.contrast = g1['contrast']
    grating1.ori=g1['orientation']
    grating1.size = g1['size']
    grating1.setPhase(g1['phaseDelta'], '+')#advance phase by 0.05 of a cycle
    grating1.draw()


    mywin.flip()
    print(lc)
    print(curTime)
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

# exp.saveAsPickle("testSave")
exp.saveAsWideText("testSaveTxt")
mywin.close()
core.quit()