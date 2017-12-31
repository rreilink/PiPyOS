import zipfile
# TODO: check Python version

zf = zipfile.PyZipFile('../python36.zip', 'w')
zf.writepy('../deps/cpython/Lib')