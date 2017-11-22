


env_base = Environment(
    CC='arm-none-eabi-gcc', 
    CCFLAGS=
    '-mfloat-abi=soft -Wall -Wno-psabi '
    '-march=armv7-a -mtune=cortex-a7 '
    '-O'.split()

    ,
    

    LINKFLAGS='-T deps/uspi/env/uspienv.ld -nostartfiles',
    LINKCOM='arm-none-eabi-gcc -o $TARGET $LINKFLAGS $_LIBDIRFLAGS $_LIBFLAGS $SOURCES',
    )


env_uspi=env_base.Clone(
    CPPPATH=['deps/uspi/include', 'deps/uspi/env/include'],
    )
env_uspi.Append(CCFLAGS = '-DRASPPI=2 -std=gnu99 -fsigned-char -fno-builtin -undef -nostdinc -nostdlib'.split())

env_py=env_base.Clone(
    CPPPATH=['.', 'adaptors', 'deps/cpython/Include'],
    #LIBS=['m'],
    )

env_py.Append(CCFLAGS=['-std=c99', '-DPy_BUILD_CORE'])

def skip(files, toskip):
    return [f for f in files if not f.name in toskip]


env_link = env_base.Clone(LIBS=['m'])

# env_link.Append(CPPPATH=['deps/uspi/include', 'deps/uspi/env/include'])


# testkeybd = env_link.Program('python', [
#     'deps/uspi/env/lib/startup.S',
#     Glob('deps/uspi/lib/*.c'), 
#     skip(Glob('deps/uspi/env/lib/*.c'), ['string.c']),
#     skip(Glob('deps/uspi/env/lib/*.S'), ['startup.S']),
#     'deps/uspi/sample/keyboard/main.c',
#     #'test.c'
#     ]
#     )



env = env_uspi.Object(
    [
     'deps/uspi/env/lib/startup.S',
     skip(Glob('deps/uspi/env/lib/*.S'), 'startup.S'),
     skip(Glob('deps/uspi/env/lib/*.c'), []),
     'adaptors/os.c'

    ])



test = env_link.Program('test.elf', [
    env,
    'test.c',
    ]
    )



Command('config.c', 'deps/cpython/Modules/config.c.in',
    [Copy("$TARGET", "$SOURCE")])


python = env_py.Object(
    [
    skip(Glob('deps/cpython/Python/*.c'), 
        ['dynload_aix.c', 'dynload_dl.c', 'dynload_hpux.c', 'dynload_next.c',
         'dynload_shlib.c', 'dynload_win.c', 'thread.c'])
    ,
    skip(Glob('deps/cpython/Parser/*.c'), 
        ['parsetok_pgen.c', 'pgen.c', 'pgenmain.c', 'tokenizer_pgen.c'])
    ,
    Glob('deps/cpython/Objects/*.c')
    ,
    ['deps/cpython/Modules/gcmodule.c', 'deps/cpython/Modules/hashtable.c', 
     'deps/cpython/Modules/main.c', 'deps/cpython/Modules/getpath.c', 
     'deps/cpython/Modules/_tracemalloc.c', 'deps/cpython/Modules/faulthandler.c',
     'deps/cpython/Modules/getbuildinfo.c',
     'config.c',
     ]
    ,
    'adaptors/adaptor.c',
    ]
    )

main_python = env_uspi.Object(['main_python.c'])

env_link.Program('python.elf', (
    env,
    python,
    main_python,
    #Glob('libm/*.o')
    'libm.a'
    ))

Command('python.img', 'python.elf', 'arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
Command('test.img', 'python.elf', 'arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
