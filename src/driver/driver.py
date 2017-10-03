import sys
sys.path.append('lib')
import MatterSim
import time
import math

import cv2
cv2.namedWindow('displaywin')
sim = MatterSim.Simulator()
sim.setDatasetPath('../data')
sim.setScanId('2t7WUuJeko7')
sim.setScreenResolution(640, 480)
sim.init()

heading = 0
elevation = 0
while True:
    sim.makeAction(0, heading, elevation)
    cv2.imshow('displaywin', sim.getState().rgb)
    k = cv2.waitKey(1)
    if k == 113:
        break
    elif k == 81:
        heading -= math.pi / 180
    elif k == 82:
        elevation += math.pi / 180
    elif k == 83:
        heading += math.pi / 180
    elif k == 84:
        elevation -= math.pi / 180
