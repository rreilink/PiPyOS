


env_base = Environment(
    CC='arm-none-eabi-gcc', 
    CCFLAGS=
    '-mfloat-abi=hard -Wall -Wno-psabi '
    '-O'.split()

    ,
    

    LINKFLAGS='-T deps/uspi/env/uspienv.ld -L//usr/lib/arm-none-eabi/newlib/ ',
    LINKCOM='arm-none-eabi-ld -o $TARGET $LINKFLAGS $_LIBDIRFLAGS $_LIBFLAGS $SOURCES',
    )


env_uspi=env_base.Clone(
    CPPPATH=['deps/uspi/include', 'deps/uspi/env/include'],
    )
env_uspi.Append(CCFLAGS = '-march=armv7-a -mtune=cortex-a7 -DRASPPI=2 -std=gnu99 -fsigned-char -fno-builtin -undef -nostdinc -nostdlib'.split())



# env_py=Environment(
#     CC='arm-none-eabi-gcc', 
#     CCFLAGS='-std=c99 -DPy_BUILD_CORE -DFILE=void -Werror '
#     '-march=armv7-a -mtune=cortex-a7 -mfloat-abi=softfp -O'.split()
#     
#     , 
#     CPPPATH=['.', 'adaptors', 'deps/cpython/Include'],
#     LIBS=['m'],
#     LINKFLAGS='-nostartfiles', # TODO: in a separate environment
#     )

env_py=env_base.Clone(
    CPPPATH=['.', 'adaptors', 'deps/cpython/Include'],
    #LIBS=['m'],
    )

env_py.Append(CCFLAGS=['-std=c99', '-DPy_BUILD_CORE'])


def skip(files, toskip):
    return [f for f in files if not f.name in toskip]


env_link = env_uspi.Clone(LIBS=['c', 'm'])

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


test = env_link.Program('python', [
    'deps/uspi/env/lib/startup.S',
    'test.c'
    ]
    )



# uspi = env_uspi.Object(
#     [
#      Glob('deps/uspi/lib/*.c'), 
#      skip(Glob('deps/uspi/env/lib/*.c'), ['string.c']),
#      skip(Glob('deps/uspi/env/lib/*.S'), 'startup.S') 
#     ])




Command('config.c', 'deps/cpython/Modules/config.c.in',
    [Copy("$TARGET", "$SOURCE")])


# python = env_py.Object(
#     [
#     skip(Glob('deps/cpython/Python/*.c'), 
#         ['dynload_aix.c', 'dynload_dl.c', 'dynload_hpux.c', 'dynload_next.c',
#          'dynload_shlib.c', 'dynload_win.c', 'thread.c'])
#     ,
#     skip(Glob('deps/cpython/Parser/*.c'), 
#         ['parsetok_pgen.c', 'pgen.c', 'pgenmain.c', 'tokenizer_pgen.c'])
#     ,
#     Glob('deps/cpython/Objects/*.c')
#     ,
#     ['deps/cpython/Modules/gcmodule.c', 'deps/cpython/Modules/hashtable.c', 
#      'deps/cpython/Modules/main.c', 'deps/cpython/Modules/getpath.c', 
#      'deps/cpython/Modules/_tracemalloc.c', 'deps/cpython/Modules/faulthandler.c',
#      'deps/cpython/Modules/getbuildinfo.c',
#      'config.c',
#      ]
#     ,
#     'adaptors/adaptor.c',
#     ]
#     )
# 
# 
# env_link.Program('kernel.elf', (
#     'deps/uspi/env/lib/startup.S',
#     uspi,
# 
#     python,
#     'adaptors/main.c'
#     ))

Command('kernel7.img', 'python', 'arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
