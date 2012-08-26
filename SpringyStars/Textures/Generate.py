#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image
from PIL import ImageFilter
from PIL import ImageEnhance

path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/'
#path = '/Users/prideout/Samples/PowerVR-ES2/Utilities/'

textool = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL8888 -i'
filewrap = 'Filewrap/MacOS/Filewrap -h -o '

system(path + textool + "Star.png")
system(path + filewrap + 'Background.h Background.pvrtc')
