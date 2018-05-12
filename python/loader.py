import posix
import __main__

def load():
    a = b'\n'
    while True:
        b = posix.read(2,1)
        if b == b'\x1b':
            exec(a, __main__.__dict__)
            break
        a+=b
