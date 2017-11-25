
def skip(files, toskip):
    return [f for f in files if not f.name in toskip]

env_base = Environment(
    CC='/opt/local/bin/arm-none-eabi-gcc',  
    CCFLAGS=
    # '-mfloat-abi=soft -Wno-psabi '
    # '-march=armv7-a -mtune=cortex-a7 '
    '-mcpu=arm1176jz-s '
    '-Wall -ffunction-sections -fdata-sections '
    '-O'.split()

    ,
    LIBS=['m'],

    LINKFLAGS=
        '-mcpu=arm1176jz-s '
        '-T deps/ChibiOS-RPi/os/ports/GCC/ARM/BCM2835/ld/BCM2835.ld -nostartfiles '
        '-Wl,--no-warn-mismatch,--gc-sections  -mno-thumb-interwork '
    ,
    LINKCOM='$CC -o $TARGET $LINKFLAGS $_LIBDIRFLAGS $_LIBFLAGS $SOURCES',
    ASCOM='$CC $ASFLAGS $_CPPINCFLAGS -o $TARGET $SOURCES',
    ASFLAGS="-c -x assembler-with-cpp $CCFLAGS",
    
    ASCOMSTR = "Assembling $TARGET",
    CCCOMSTR = "Compiling $TARGET",
    LINKCOMSTR = "Linking $TARGET",
    
    
    )

######################
#      ChibiOS       #
######################


chibios_path = 'deps/ChibiOS-RPi/'

env_chibios = env_base.Clone()
env_chibios.Append(
    CCFLAGS=
    '-fomit-frame-pointer -Wall -Wextra -Wstrict-prototypes -mno-thumb-interwork -MD -MP'.split(),
    
    CPPPATH=['.'] + [chibios_path + x for x in [
        '',
        'os/ports/GCC/ARM', 'os/ports/GCC/ARM/BCM2835', 'os/kernel/include', 'test',
        'os/hal/include', 'os/hal/platforms/BCM2835', 'os/various',
        'boards/RASPBERRYPI_MODB'
    ]]
    )

c = chibios_path

chibios = env_chibios.Object(
    [
     Glob(chibios_path + 'os/ports/GCC/ARM/*.s'),
     Glob(chibios_path + 'os/ports/GCC/ARM/*.c'),
     Glob(chibios_path + 'os/ports/GCC/ARM/BCM2835/*.s'),
     Glob(chibios_path + 'os/kernel/src/*.c'),
     Glob(chibios_path + 'os/hal/src/*.c'),
     Glob(chibios_path + 'test/*.c'),
     Glob(chibios_path + 'os/hal/platforms/BCM2835/*.c'),
     chibios_path + 'os/various/shell.c',
     chibios_path + 'os/various/chprintf.c',
     chibios_path + 'boards/RASPBERRYPI_MODB/board.c',
     'adaptors/os.c',
    ])
    
######################
#      Python        #
######################

env_py=env_base.Clone(
    CPPPATH=['.', 'adaptors', 'deps/cpython/Include'],
    )

env_py.Append(CCFLAGS=['-std=c99', '-DPy_BUILD_CORE'])


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
    # 'main_python.c'
    ]
    )

test = env_chibios.Program('test.elf', [
    chibios,
    'main_test.c',
    'libm.a',
    ]
    )
 

pipyos = env_chibios.Program('pipyos.elf', [
    chibios,
    python,
    'main_pipyos.c',
    'libm.a',
    ]
    )



Command('pipyos.img', 'pipyos.elf', '/opt/local/bin/arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
Command('test.img', 'test.elf', '/opt/local/bin/arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
