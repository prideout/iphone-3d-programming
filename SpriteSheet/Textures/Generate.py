#!/usr/bin/python

import cairo
import math

from  os import system
from platform import uname
from PIL import Image
from PIL import ImageFilter
from PIL import ImageEnhance

#path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/'
path = '/Users/prideout/Samples/PowerVR-ES2/Utilities/'

textool = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL88 -i'
filewrap = 'Filewrap/MacOS/Filewrap -h -o '

system(path + textool + 'Unified.png')
system(path + textool + 'EyesLayer.png')
system(path + textool + 'BodyLayer.png')
system(path + filewrap + 'Background.h Background.pvrtc')

##################################################################

headerFile = open('EyesLayer.h', 'a')
boxes = '''
static float EyeLayerBoxes[] = {
	0, 0, 116, 70,
	116, 0, 228, 70,
	228, 0, 334, 71,
	334, 0, 438, 71,
	0, 71, 102, 143,
	102, 71, 200, 144,
	200, 71, 296, 144,
	296, 71, 390, 145,
	390, 71, 480, 145,
	0, 145, 88, 222,
	88, 145, 174, 224,
	174, 145, 256, 226,
	256, 145, 336, 228,
	336, 145, 414, 228,
	414, 145, 488, 230,
};
'''
headerFile.write(boxes)
headerFile.close()

##################################################################

headerFile = open('BodyLayer.h', 'a')
boxes = '''
static float BodyLayerBoxes[] = {
	0, 0, 116, 70,
	116, 0, 228, 70,
	228, 0, 334, 71,
	334, 0, 438, 71,
	0, 71, 102, 143,
	102, 71, 200, 144,
	200, 71, 296, 144,
	296, 71, 390, 145,
	390, 71, 480, 145,
	0, 145, 88, 222,
	88, 145, 174, 225,
	174, 145, 256, 226,
	256, 145, 336, 228,
	336, 145, 414, 228,
	414, 145, 488, 230,
};
'''
headerFile.write(boxes)
headerFile.close()

##################################################################

headerFile = open('Unified.h', 'a')
boxes = '''
static float UnifiedBodyLayerBoxes[] = {
    0, 0, 116, 70, // 0
    116, 0, 228, 70, // 1
    228, 0, 334, 71, // 2
    334, 0, 438, 71, // 3
    0, 71, 102, 143, // 4
    102, 71, 200, 144, // 5
    200, 71, 296, 144, // 6
    296, 71, 390, 145, // 7
    390, 71, 480, 145, // 8
    0, 145, 88, 222, // 9
    88, 145, 174, 225, // 10
    174, 145, 256, 226, // 11
    256, 145, 336, 228, // 12
    336, 145, 414, 228, // 13
    414, 145, 488, 230, // 14
};

static float UnifiedEyeLayerBoxes[] = {
    0, 230, 116, 300, // 0
    116, 230, 228, 300, // 1
    228, 230, 334, 301, // 2
    334, 230, 438, 301, // 3
    0, 301, 102, 373, // 4
    102, 301, 200, 374, // 5
    200, 301, 296, 374, // 6
    296, 301, 390, 375, // 7
    390, 301, 480, 375, // 8
    0, 375, 88, 452, // 9
    88, 375, 174, 454, // 10
    174, 375, 256, 456, // 11
    256, 375, 336, 458, // 12
    336, 375, 414, 458, // 13
    414, 375, 488, 460, // 14
};'''
headerFile.write(boxes)
headerFile.close()
