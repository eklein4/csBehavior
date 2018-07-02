import sys
from psychopy import visual, core, event, data, clock 

import numpy as np

# gammGrid=np.zeros([4,6])
# gammGrid[:, 1:3]=1

#create a window
win_x1=1920
win_y1=1080


mywin = visual.Window([1920,1080],monitor="testMonitor",fullscr=False, units="deg")
# mywin.gammaRamp=gammGrid)

exp = data.ExperimentHandler(dataFileName="ydo")


# timing etc.
startFlag=0
timeOffset=0
curTime=0
lc=0

# stim 1 params
g1={'phaseDelta':0.05,'Xpos':0,'Ypos':0,'spFreq':0.2,'mask':'gauss',
    'size':(10,10),'contrast':1.0,'orientation':0}

#create some stimuli
grating1= visual.GratingStim(win=mywin, mask=g1['mask'], 
    pos=[g1['Xpos'],g1['Ypos']],sf=g1['spFreq'],colorSpace='rgb',maskParams={'sd': 5},
    ori=g1['orientation'])


# draw stim refresh monitor
while True: #this creates a never-ending loop
    if startFlag==0:
        timeOffset=clock.getTime()
        startFlag=1
    curTime=clock.getTime()-timeOffset
    if curTime>1 and curTime<2:
        g1['contrast']=0.2
    elif curTime>=2:
        g1['contrast']=1.0
        g1['orientation']=120
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