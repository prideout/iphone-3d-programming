#!/usr/bin/python

import os
import math
from PIL import Image

spreadFactor = 25
scale = 0.25
sourceFile = "Aum.png"
destFile = "DistanceField.png"
comparisonFile = "SmallAum.png"

def invert(c):
    return 255 - c
    
def nextPot(n):
    n = n - 1
    n |= n >> 1
    n |= n >> 2
    n |= n >> 4
    n |= n >> 8
    n |= n >> 16
    n = n + 1
    return n

def initCell(pixel):
    if pixel == 0: return inside
    return outside

def distSq(cell):
    return cell[0] * cell[0] + cell[1] * cell[1]

def getCell(grid, x, y):
    if y < 0 or y >= len(grid): return outside
    if x < 0 or x >= len(grid[y]): return outside
    return grid[y][x]

def compare(grid, cell, x, y, ox, oy):
    other = getCell(grid, x + ox, y + oy)
    other = (other[0] + ox, other[1] + oy)
    if distSq(other) < distSq(cell): return other
    return cell

def propagate(grid):
    for y in xrange(0, height):
        for x in xrange(0, width):
            cell = grid[y][x]
            cell = compare(grid, cell, x, y, -1,  0)
            cell = compare(grid, cell, x, y,  0, -1)
            cell = compare(grid, cell, x, y, -1, -1)
            cell = compare(grid, cell, x, y, +1, -1)
            grid[y][x] = cell
        for x in xrange(width - 1, -1, -1):
            cell = grid[y][x]
            cell = compare(grid, cell, x, y, 1, 0)
            grid[y][x] = cell
    for y in xrange(height - 1, -1, -1):
        for x in xrange(width - 1, -1, -1):
            cell = grid[y][x]
            cell = compare(grid, cell, x, y, +1,  0)
            cell = compare(grid, cell, x, y,  0, +1)
            cell = compare(grid, cell, x, y, -1, +1)
            cell = compare(grid, cell, x, y, +1, +1)
            grid[y][x] = cell
        for x in xrange(0, width):
            cell = grid[y][x]
            cell = compare(grid, cell, x, y, -1,  0)
            grid[y][x] = cell

print "Allocating the destination image..."
inside, outside = (0,0), (9999, 9999)
alphaChannel = Image.open(sourceFile).split()[3]
w = alphaChannel.size[0] + spreadFactor * 2
h = alphaChannel.size[1] + spreadFactor * 2
srcImage = Image.new("L", (w, h), 0)
srcImage.paste(alphaChannel, (spreadFactor, spreadFactor))
width, height = srcImage.size

print "Creating the two grids..."
pixels = srcImage.load()
grid0 = [[initCell(pixels[x, y]) for x in xrange(width)] for y in xrange(height)] 
grid1 = [[initCell(invert(pixels[x, y])) for x in xrange(width)] for y in xrange(height)] 

print "Propagating grid 0..."
propagate(grid0)

print "Propagating grid 1..."
propagate(grid1)

print "Subtracting grids..."
signedDistance = [[0 for x in xrange(width)] for y in xrange(height)] 
for y in xrange(height):
    for x in xrange(width):
        dist1 = math.sqrt(distSq(grid0[y][x]))
        dist0 = math.sqrt(distSq(grid1[y][x]))
        signedDistance[y][x] = dist0 - dist1

print "Normalizing..."
maxDist, minDist = spreadFactor, -spreadFactor
for y in xrange(height):
    for x in xrange(width):
        dist = signedDistance[y][x]
        if dist < 0: dist = -128 * (dist - minDist) / minDist
        else: dist = 128 + 128 * dist / maxDist
        if dist < 0: dist = 0
        elif dist > 255: dist = 255
        signedDistance[y][x] = int(dist)
        pixels[x, y] = signedDistance[y][x]

print "Scaling..."
srcImage.thumbnail((int(width * scale), int(height * scale)))
destSize = (nextPot(srcImage.size[0]), nextPot(srcImage.size[1]))

print "Expanding canvas from (%d, %d) to (%d, %d)..." % (srcImage.size + destSize)
destImage = Image.new("L", destSize, 255)
x = (destSize[0] - srcImage.size[0]) / 2
y = (destSize[1] - srcImage.size[1]) / 2
destImage.paste(srcImage, (x, y))

print "Saving %s..." % destFile
destImage.save(destFile)
os.system("open " + destFile)

print "Converting to a header file..."
path = '/Users/prideout/Documents/PowerVR-ES2/Utilities/PVRTexTool/PVRTexToolCL/MacOS/'
os.system(path + 'PVRTexTool -h -yflip1 -fOGL8 -i' + destFile)

print "Generating comparison image..."
alphaChannel = Image.open(sourceFile).split()[3]
w = alphaChannel.size[0] + spreadFactor * 2
h = alphaChannel.size[1] + spreadFactor * 2
srcImage = Image.new("L", (w, h), 0)
srcImage.paste(alphaChannel, (spreadFactor, spreadFactor))
width, height = srcImage.size
srcImage.thumbnail((int(width * scale), int(height * scale)))
srcImage = Image.eval(srcImage, invert)
destImage = Image.new("L", destSize, 255)
x = (destSize[0] - srcImage.size[0]) / 2
y = (destSize[1] - srcImage.size[1]) / 2
destImage.paste(srcImage, (x, y))
destImage.save(comparisonFile)
os.system(path + 'PVRTexTool -h -yflip1 -fOGL8 -i' + comparisonFile)
