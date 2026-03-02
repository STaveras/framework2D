import json, os
from pathlib import Path

def main():
    cwd = Path.cwd()
    src_json = cwd / 'Character/Animations.json'
    dst_dir = cwd / '2D-Pixel-Art-Character-Template'
    dst_json = dst_dir / 'Animations.json'

    if not src_json.exists():
        print(f"Source file missing: {src_json}")
        return

    data = json.loads(src_json.read_text())
    for anim in data['AnimationFile']['Animation']:
        name = anim['Name']
        tmpl_path = f"./fantasySideScroller/2D-Pixel-Art-Character-Template/{name}/{name}-Sheet.png"
        for frame in anim.get('Frame', []):
            frame['Filename'] = tmpl_path
    dst_json.parent.mkdir(parents=True, exist_ok=True)
    dst_json.write_text(json.dumps(data, indent=2))
    os.system(f"git add {dst_json}")

if __name__=='__main__':
    main()
