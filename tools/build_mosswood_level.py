"""Rebuild Mosswood Hollow and its tile-exact preview. Requires Pillow."""
import json
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[1] / 'bin/fantasySideScroller'
W, H = 60, 34
layers, sets, atlases = [], [], []

def tileset(name, image):
    im = Image.open(ROOT / image).convert('RGBA')
    first = 1 + sum(a[2] for a in atlases)
    cols, rows = im.width // 16, im.height // 16
    path = 'Assets/' + name + '.tsj'
    data = dict(columns=cols, image='../'+image, imagewidth=im.width, imageheight=im.height,
                margin=0, spacing=0, name=name, tilecount=cols*rows, tilewidth=16,
                tileheight=16, type='tileset', version='1.10', tiledversion='1.11.2')
    if name != 'fantasyTiles':
        (ROOT/path).write_text(json.dumps(data, indent=2)+'\n')
    sets.append(dict(firstgid=first, source=path))
    atlases.append((first, im, cols*rows, cols))
    return first, cols

F = tileset('fantasyTiles', 'Assets/Tiles.png')
B = tileset('mosswoodDistantTrees', 'Trees/Background.png')
T = tileset('mosswoodGreenTrees', 'Trees/Green-Tree.png')
S = tileset('mosswoodSky', 'Background/Background.png')

def layer(name, mode='none'):
    l = dict(id=len(layers)+1, name=name, type='tilelayer', width=W, height=H,
             x=0, y=0, opacity=1, visible=True, data=[0]*(W*H),
             properties=[dict(name='collision_mode', type='string', value=mode)])
    layers.append(l)
    return l['data']

def put(a,x,y,g):
    if 0 <= x < W and 0 <= y < H: a[y*W+x]=g

def stamp(a, atlas, sx, sy, width, height, x, y):
    first, cols = atlas
    for dy in range(height):
        for dx in range(width): put(a,x+dx,y+dy,first+(sy+dy)*cols+sx+dx)

sky=layer('01 Sky')
for y in range(H):
    for x in range(W): put(sky,x,y,S[0]+min(y,16)*S[1]+x%30)
far=layer('02 Misty forest')
for x,y in [(-2,8),(20,5),(42,9)]: stamp(far,B,22,0,22,16,x,y)
near=layer('03 Deep forest')
for x,y in [(-9,15),(11,17),(33,18),(51,14)]: stamp(near,B,0,0,22,16,x,y)
trees=layer('04 Ancient pines')
for x,y in [(-3,-3),(10,5),(24,-1),(40,8),(51,-4)]: stamp(trees,T,0,0,7,23,x,y)
# Additional overlapping boughs and trunks use intact source atlas motifs.
for x,y in [(3,15),(19,18),(34,14),(47,18)]: stamp(trees,T,0,45,6,14,x,y)
terrain=layer('05 Moss and stone','solid')
def island(x,y,w,depth):
    for dx in range(w):
        put(terrain,x+dx,y,26 if dx==0 else 30 if dx==w-1 else 27+dx%3)
        for dy in range(1,depth):
            put(terrain,x+dx,y+dy,51 if dx==0 else 55 if dx==w-1 else [52,53,54,77,78,79][(dx+dy*3)%6])
        if y+depth<H:
            put(terrain,x+dx,y+depth,101 if dx==0 else 105 if dx==w-1 else 102+dx%3)
# Sheltered entrance, low bank, pond and climbing eastern bank.
for args in [(0,11,9,7),(0,28,12,6),(12,27,8,7),(20,28,7,6),
             (27,30,7,4),(34,32,7,2),(41,29,6,5),(47,26,6,8),(53,23,7,11),
             (12,20,6,2),(22,16,6,2),(34,12,7,2),(46,7,14,3),(0,0,3,9)]: island(*args)
bridge=layer('06 Wooden crossings','one_way')
for x,y,w in [(8,15,7),(18,23,6),(28,19,6),(39,16,7),(32,29,10)]:
    for dx in range(w): put(bridge,x+dx,y,131 if dx==0 else 133 if dx==w-1 else 132)
props=layer('07 Forest flowers and mushrooms')
for x,y,w in [(8,15,7),(18,23,6),(28,19,6),(39,16,7),(32,29,10)]:
    for dx in range(w): put(props,x+dx,y+1,156 if dx==0 else 158 if dx==w-1 else 157)
for x,y in [(2,11),(6,28),(15,27),(23,28),(43,29),(49,26),(55,23),(48,7),(57,7),(24,16)]:
    stamp(props,F,18,15,2,2,x,y-2)
for x,y in [(5,11),(2,28),(10,28),(18,27),(26,28),(45,29),(52,26),(58,23),(36,12),(54,7),(14,20)]:
    stamp(props,F,17,17,1,2,x,y-2)
for x,y in [(8,28),(19,27),(25,28),(42,29),(51,26),(56,23),(38,12)]: put(props,x,y-1,441)
for x,y in [(29,30),(32,30),(41,29)]: stamp(props,F,15,18,1,2,x,y-2)
stamp(props,F,12,6,2,3,30,6)
water=layer('08 Pond water')
for x in range(34,41):
    put(water,x,31,457+(x%4))
    for y in range(32,H): put(water,x,y,482+(x%4))
objects=dict(id=len(layers)+1,name='09 Player and destination',type='objectgroup',draworder='topdown',
             x=0,y=0,opacity=1,visible=True,objects=[
                 dict(id=1,name='Entrance',type='spawn',point=True,x=72,y=160,width=0,height=0,rotation=0,visible=True),
                 dict(id=2,name='Canopy overlook',type='destination',point=True,x=904,y=96,width=0,height=0,rotation=0,visible=True)])
layers.append(objects)
data=dict(compressionlevel=-1,width=W,height=H,infinite=False,nextlayerid=len(layers)+1,nextobjectid=3,
          orientation='orthogonal',renderorder='right-down',tilewidth=16,tileheight=16,type='map',version='1.10',
          tiledversion='1.11.2',tilesets=sets,layers=layers,
          properties=[dict(name='title',type='string',value='Mosswood Hollow')])
(ROOT/'mosswood_hollow.tmj').write_text(json.dumps(data,indent=2)+'\n')
preview=Image.new('RGBA',(W*16,H*16),(198,237,237,255))
for l in layers:
    for i,gid in enumerate(l.get('data',[])):
        if not gid: continue
        first,im,count,cols=next(a for a in reversed(atlases) if gid>=a[0])
        n=gid-first
        assert 0<=n<count
        tile=im.crop((n%cols*16,n//cols*16,n%cols*16+16,n//cols*16+16))
        preview.alpha_composite(tile,(i%W*16,i//W*16))
preview.convert('RGB').resize((W*32,H*32),Image.Resampling.NEAREST).save(ROOT/'mosswood_hollow_preview.png')
print('Created Mosswood Hollow: 60 × 34 tiles; validated every tile ID and layer size.')
