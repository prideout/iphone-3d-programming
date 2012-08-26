#!/usr/bin/python

import os
import math
from PIL import Image

inside, outside = (0,0), (9999, 9999)

def invert(c):
	return 255 - c
	
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
	height = len(grid)
	width = len(grid[0])
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

def GenerateDistanceField(inFile, outFile, spread):
	
	print "Allocating the destination image..."
	channels = Image.open(inFile).split()
	if len(channels) == 4: alphaChannel = channels[3]
	else: alphaChannel = channels[0]
	w = alphaChannel.size[0] + spread * 2
	h = alphaChannel.size[1] + spread * 2
	img = Image.new("L", (w, h), 0)
	img.paste(alphaChannel, (spread, spread))
	width, height = img.size

	print "Creating the two grids..."
	pixels = img.load()
	grid0 = [[initCell(pixels[x, y]) for x in xrange(width)] for y in xrange(height)] 
	grid1 = [[initCell(invert(pixels[x, y])) for x in xrange(width)] for y in xrange(height)] 

	print "Propagating grids..."
	propagate(grid0)
	propagate(grid1)

	print "Subtracting grids..."
	signedDistance = [[0 for x in xrange(width)] for y in xrange(height)]
	for y in xrange(height):
		for x in xrange(width):
			dist1 = math.sqrt(distSq(grid0[y][x]))
			dist0 = math.sqrt(distSq(grid1[y][x]))
			signedDistance[y][x] = dist0 - dist1

	print "Normalizing..."
	maxDist, minDist = spread, -spread
	for y in xrange(height):
		for x in xrange(width):
			dist = signedDistance[y][x]
			if dist < 0: dist = -128 * (dist - minDist) / minDist
			else: dist = 128 + 128 * dist / maxDist
			if dist < 0: dist = 0
			elif dist > 255: dist = 255
			signedDistance[y][x] = int(dist)
			pixels[x, y] = signedDistance[y][x]

	print "Saving %s..." % outFile
	img.save(outFile)

if __name__ == "__main__":
	inFile, outFile = 'SmallAum.png', 'BookScriptOut.png'
	GenerateDistanceField(inFile, outFile, spread = 15)
