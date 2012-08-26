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

crossCube = Image.open('Minnehaha.png')
w = crossCube.size[0] / 3
h = crossCube.size[1] / 4

if w != h:
	print "Error: cubemap faces are non-square"
	exit()
	
print "Cubemap faces are %d x %d" % (w, h)

faces = []
faces.append(crossCube.crop((w * 2, h, w * 3, h * 2))) # +X
faces.append(crossCube.crop((0, h, w, h * 2))) # -X
faces.append(crossCube.crop((w, h * 2, w * 2, h * 3))) # +Y
faces.append(crossCube.crop((w, 0, w * 2, h))) # -Y
faces.append(crossCube.crop((w, h, w * 2, h * 2))) # +Z
faces.append(crossCube.crop((w, h * 3, w * 2, h * 4))) # -Z

for face in faces:
	face = thumbnail((256, 256))

faces[-1] = faces[-1].transpose(Image.FLIP_TOP_BOTTOM)
faces[-1] = faces[-1].transpose(Image.FLIP_LEFT_RIGHT)


for face in xrange(len(faces)):
	name = 'Face%d.png' % face
	faces[face].save(name)
	ShowImage(name)
	
path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/'
textool = 'PVRTexTool/PVRTexToolCL/MacOS/PVRTexTool -h -yflip1 -fOGL565 -i'

system(path + textool + "Metal.jpg")

for face in xrange(len(faces)):
	system(path + textool + "Face%d.png" % face)
	
images = ["Face%d.h" % face for face in xrange(len(faces))]
images.append('Metal.h')

fromStr = "static const unsigned long" 
toStr = "const unsigned int" 

for filename in images:
	for line in fileinput.FileInput(filename, inplace=1):
		print line.replace(fromStr, toStr),
