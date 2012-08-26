#!/usr/bin/python

LevelOfDetail = 4

import os
import math

def average(p1, p2):
    (x, y, z) = (u+v for u,v in zip(p1, p2))
    s = 1.0 / math.sqrt(x*x + y*y + z*z)
    return (x * s, y * s, z * s)

def octahedron():
    F = math.sqrt(2.0) / 2.0
    vertices = [(0, -1, 0),
                (-F, 0, F),
                ( F, 0, F),
                ( F, 0, -F),
                (-F, 0, -F),
                (0, 1, 0)]
    faces = [(0, 2, 1),
             (0, 3, 2),
             (0, 4, 3),
             (0, 1, 4),
             (5, 1, 2),
             (5, 2, 3),
             (5, 3, 4),
             (5, 4, 1)]
    return (vertices, faces)

def icosahedron():
    radius = 1
    sqrt5 = math.sqrt(5.0)
    phi = (1.0 + sqrt5) * 0.5
    ratio = math.sqrt(10.0 + (2.0 * sqrt5)) / (4.0 * phi)
    a = (radius / ratio) * 0.5
    b = (radius / ratio) / (2.0 * phi)
    vertices = [ (),
                 ( 0,  b, -a),
                 ( b,  a,  0),
                 (-b,  a,  0),
                 ( 0,  b,  a),
                 ( 0, -b,  a),
                 (-a,  0,  b),
                 ( 0, -b, -a),
                 ( a,  0, -b),
                 ( a,  0,  b),
                 (-a,  0, -b),
                 ( b, -a,  0),
                 (-b, -a,  0)]
    faces = [ (1, 2, 3),
              (4, 3, 2),
              (4, 5, 6),
              (4, 9, 5),
              (1, 7, 8),
              (1, 10, 7),
              (5, 11, 12),
              (7, 12, 11),
              (3, 6, 10),
              (12, 10, 6),
              (2, 8, 9),
              (11, 9, 8),
              (4, 6, 3),
              (4, 2, 9),
              (1, 3, 10),
              (1, 8, 2),
              (7, 10, 12),
              (7, 11, 8),
              (5, 12, 6),
              (5, 9, 11)]
    return (vertices, faces)

(vertices, faces) = icosahedron()

texCoords = [(0, 1),
             (0.5, 1.0 - math.sqrt(3.0) / 2.0),
             (1, 1)] 

for lod in range(LevelOfDetail):
    faceCount = len(faces)
    for faceIndex in range(faceCount):
        face = faces[faceIndex]
        i = len(vertices)
        j = i + 1
        k = j + 1
        a = vertices[face[0]]
        b = vertices[face[1]]
        c = vertices[face[2]]
        vertices.append(average(a, b))
        vertices.append(average(b, c))
        vertices.append(average(a, c))
        faces.append((i, j, k))
        faces.append((face[0], i, k))
        faces.append((i, face[1], j))
        faces[faceIndex] = (k, j, face[2])

# We need a dome, not a sphere.
def faceFilter(face):
    global vertices
    (i,j,k) = (vertices[face[c]][1] < 0.125 for c in (0,1,2))
    return i and j and k
faces = filter(faceFilter, faces)

outfile = open('GeodesicDome.h', 'w')
outfile.write('const int DomeFaceCount = %d;\n' % len(faces))
outfile.write('const int DomeVertexCount = DomeFaceCount * 3;\n')
outfile.write('const float DomeVertices[DomeVertexCount * 5] = {\n')
for (i, j, k) in faces:
    outfile.write('    %f, %f, %f,\n' % vertices[i])
    outfile.write('    %f, %f,\n' % texCoords[0])
    outfile.write('    %f, %f, %f,\n' % vertices[j])
    outfile.write('    %f, %f,\n' % texCoords[1])
    outfile.write('    %f, %f, %f,\n' % vertices[k])
    outfile.write('    %f, %f,\n' % texCoords[2])
outfile.write('};\n')
outfile.close()
os.system("cat GeodesicDome.h")
