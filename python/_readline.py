'''
Python implementation of readline function:
- editing of current line using left/right arrow keys, backspace
- command history

'''

import sys

history = []

def readline(prompt):
    sys.stdout.write(prompt + '\x1b[K')

    buffer = ''
    pos = 0
    history.append('')
    histpos = len(history) - 1

    while True:
        sys.stdout.flush()

        cmd = ''
        while '\x1b['.startswith(cmd):
            cmd += sys.stdin.read(1)
        
        if cmd=='\n':
            history[-1] = buffer
            
            # Prevent duplicate entries at the end
            if len(history) > 1 and history[-2] == history[-1]:
                history.pop()
                
            sys.stdout.write('\n')
              
            return buffer + '\n'
        if cmd=='\b' or cmd == '\x7f':
            if pos>0:
                buffer = buffer[:pos-1] + buffer[pos:]
                nback = len(buffer) - pos + 1
                backcmd = '\x1b[' + str(nback) + 'D' if nback>0 else ''
                sys.stdout.write('\x1b[D'  + buffer[pos-1:] + '\x1b[K' + backcmd)
                pos = pos -1
                history[-1] = buffer
                histpos = len(history) - 1
                
        elif len(cmd)==1 and ord(cmd) >= 32: # Normal char
            buffer = buffer[:pos] + cmd + buffer[pos:]
            nback = len(buffer) - pos - 1
            backcmd = '\x1b[' + str(nback) + 'D' if nback>0 else ''
            sys.stdout.write(buffer[pos:] + backcmd)
            pos += 1

            history[-1] = buffer
            histpos = len(history) - 1
                
                           
        elif cmd == '\x1b[D': # left
            if pos > 0:
                pos-=1
                sys.stdout.write(cmd)
        elif cmd == '\x1b[C': # right
            if pos < len(buffer):
                pos+=1
                sys.stdout.write(cmd)
                
        elif cmd == '\x1b[A': # up
            if histpos > 0:
                histpos -= 1
                backcmd = '\x1b[' + str(pos) + 'D' if pos>0 else ''
                buffer = history[histpos]
                pos = len(buffer)
                sys.stdout.write(backcmd + '\x1b[K' + buffer)
        elif cmd == '\x1b[B': # down

            if histpos < len(history) - 1:
                histpos += 1
                backcmd = '\x1b[' + str(pos) + 'D' if pos>0 else ''
                buffer = history[histpos]
                pos = len(buffer)
                sys.stdout.write(backcmd + '\x1b[K' + buffer)