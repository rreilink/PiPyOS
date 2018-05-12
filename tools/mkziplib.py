import zipfile
import os

# TODO: check Python version
os.chdir('../deps/cpython')
path = 'Lib'
with zipfile.PyZipFile('../python36.zip', 'w') as zf:
    for item in os.listdir(path):
        if item == '__pycache__' or item == 'test':
            continue
    
        zf.writepy(os.path.join(path, item))

with zipfile.PyZipFile('../python36z.zip', 'w', zipfile.ZIP_DEFLATED) as zf:
    for item in os.listdir(path):
        if item == '__pycache__' or item == 'test':
            continue
    
        zf.writepy(os.path.join(path, item))
        