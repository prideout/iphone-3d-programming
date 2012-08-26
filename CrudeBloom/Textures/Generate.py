#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image
from PIL import ImageFilter
from PIL import ImageEnhance
import fileinput

def ShowImage(image):
    if uname()[0] == 'Linux': system('gnome-open ' + image)
    elif uname()[0] == 'Darwin': system('open ' + image)
    else: system('start ' + image)

path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/'
textool = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL565 -i'

system(path + textool + "TombWindow.png")

images = []
images.append('TombWindow.h')

fromStr = "static const unsigned long" 
toStr = "const unsigned int" 

for filename in images:
	for line in fileinput.FileInput(filename, inplace=1):
		print line.replace(fromStr, toStr),
