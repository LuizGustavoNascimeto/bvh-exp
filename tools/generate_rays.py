import os
from math import sqrt
import random

ROOT = os.path.dirname(os.path.dirname(__file__))
OBJ_PATH = os.path.join(ROOT, 'dataset', 'scenes', 'bunny.obj')
RAYS_PATH = os.path.join(ROOT, 'dataset', 'rays')
BACKUP_PATH = RAYS_PATH + '.bak'

# parse vertices
xs = []
ys = []
zs = []
with open(OBJ_PATH, 'r', encoding='utf-8') as f:
    for line in f:
        if line.startswith('v '):
            parts = line.strip().split()
            if len(parts) >= 4:
                x, y, z = map(float, parts[1:4])
                xs.append(x); ys.append(y); zs.append(z)

if not xs:
    raise RuntimeError('No vertices found in ' + OBJ_PATH)

minx, maxx = min(xs), max(xs)
miny, maxy = min(ys), max(ys)
minz, maxz = min(zs), max(zs)
center = ((minx+maxx)/2.0, (miny+maxy)/2.0, (minz+maxz)/2.0)
print('Bunny bbox: x=[%g,%g] y=[%g,%g] z=[%g,%g]' % (minx,maxx,miny,maxy,minz,maxz))
print('Center:', center)

# backup existing rays
if os.path.exists(RAYS_PATH) and not os.path.exists(BACKUP_PATH):
    os.replace(RAYS_PATH, BACKUP_PATH)
    print('Backed up', RAYS_PATH, '->', BACKUP_PATH)

# generate 100 random rays whose origins lie on a plane behind the bunny
# and whose directions point toward random points inside the bunny bbox.
# This gives a good chance to intersect the bunny but does not guarantee hits.
plane_z = center[2] - max((maxz - minz) * 0.5, 0.25)

lines = []
lines.append('# Dataset of rays (randomized to have chance to hit bunny)')
lines.append('# Format per line: ox oy oz dx dy dz tmin tmax')
lines.append('# center = %g %g %g' % center)

n = 100
random.seed(12345)
for i in range(n):
    # origin x,y uniformly over bunny x/y extents
    ox = random.uniform(minx, maxx)
    oy = random.uniform(miny, maxy)
    oz = plane_z

    # pick a random target point inside the bbox
    tx = random.uniform(minx, maxx)
    ty = random.uniform(miny, maxy)
    tz = random.uniform(minz, maxz)

    dx = tx - ox
    dy = ty - oy
    dz = tz - oz
    mag = sqrt(dx*dx + dy*dy + dz*dz)
    if mag == 0:
        dx,dy,dz = 0.0,0.0,1.0
    else:
        dx,dy,dz = dx/mag, dy/mag, dz/mag

    lines.append('%.8f %.8f %.8f %.8f %.8f %.8f 0.0 100.0' % (ox,oy,oz,dx,dy,dz))

with open(RAYS_PATH, 'w', encoding='utf-8') as f:
    f.write('\n'.join(lines) + '\n')

print('Wrote', RAYS_PATH, 'with', n, 'rays')
print('Done.')
