import sys
sys.path.append('tools')
import mkinitfs
import mkzloader

def skip(files, toskip):
    return [f for f in files if not f.name in toskip]

env_base = Environment(
    CC='arm-none-eabi-gcc',  
    CCFLAGS=
    # '-mfloat-abi=soft -Wno-psabi '
    # '-march=armv7-a -mtune=cortex-a7 '
    '-mcpu=arm1176jz-s -mfloat-abi=soft -mno-thumb-interwork '
    '-Wall -ffunction-sections -fdata-sections -g '
    '-D_XOPEN_SOURCE=600 '
    '-O2'.split()

    ,
    LIBS=['m'],

    LINKFLAGS=
        '-mcpu=arm1176jz-s '
        '-nostartfiles '
        '-Wl,--no-warn-mismatch,--gc-sections -mno-thumb-interwork -Wl,-Map,${TARGET}.map '.split()
    ,
    CPPPATH=['adaptors', 'src', 'src/pi']
    ,
    LINKCOM='$CC -o $TARGET $LINKFLAGS $_LIBDIRFLAGS $_LIBFLAGS $SOURCES',
    ASCOM='$CC $ASFLAGS $_CPPINCFLAGS -o $TARGET $SOURCES',
    ASFLAGS="-c -x assembler-with-cpp $CCFLAGS",
    
    ASCOMSTR = "Assembling $TARGET",
    #CCCOMSTR = "Compiling $TARGET",
    LINKCOMSTR = "Linking $TARGET",
    
    
    )

######################
#      ChibiOS       #
######################


chibios_path = 'deps/ChibiOS-RPi/'

# Add include path to the base environment
env_base.Append(
    CPPPATH=['deps/ff13a'] + [chibios_path + x for x in [
        '',
        'os/ports/GCC/ARM', 'os/ports/GCC/ARM/BCM2835', 'os/kernel/include', 'test',
        'os/hal/include', 'os/hal/platforms/BCM2835', 'os/various',
        'boards/RASPBERRYPI_MODB'
    ]]
    )

# Add custom compile flags to a dedicated ChibiOS environment

env_chibios = env_base.Clone()
env_chibios.Append(
    CCFLAGS=
    '-fomit-frame-pointer -Wall -Wextra -Wstrict-prototypes  -Wno-unused-parameter'.split(),
    
    LINKFLAGS = '-TBCM2835.ld'
    )

chibios = env_chibios.Object(
    [
     Glob(chibios_path + 'os/ports/GCC/ARM/*.s'),
     Glob(chibios_path + 'os/ports/GCC/ARM/*.c'),
     Glob(chibios_path + 'os/ports/GCC/ARM/BCM2835/*.s'),
     Glob(chibios_path + 'os/kernel/src/*.c'),
     Glob(chibios_path + 'os/hal/src/*.c'),
     Glob(chibios_path + 'test/*.c'),
     skip(Glob(chibios_path + 'os/hal/platforms/BCM2835/*.c'),['hal_lld.c', 'serial_lld.c', 'sdc_lld.c', 'spi_lld.c']),
     chibios_path + 'os/various/shell.c',
     chibios_path + 'os/various/chprintf.c',
     chibios_path + 'boards/RASPBERRYPI_MODB/board.c',
     Glob('src/os/*.c'),
     Glob('src/pi/*.c'),
     skip(Glob('deps/ff13a/*.c'), ['diskio.c']),
    ])
    
######################
#      Python        #
######################

env_py=env_base.Clone()

env_py.Append(
    CPPPATH=['.', 'adaptors', 'src', 'deps/cpython/Include', 'deps/cpython/Modules/zlib'],
    CCFLAGS=['-std=gnu99', '-DPy_BUILD_CORE', '-Wno-unused-function', '-Wno-unused-variable', '-Wno-unused-parameter']
    )

# This allows us to use the Python environment also for building zloader
# This simplifies things since they share some zlib source files
env_py.Append(LINKFLAGS = ['-Tsrc/zloader/zloader.ld']) 

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
     'deps/cpython/Modules/getbuildinfo.c', 'deps/cpython/Modules/_weakref.c',
     'deps/cpython/Modules/posixmodule.c', 'deps/cpython/Modules/zipimport.c',
     'deps/cpython/Modules/_codecsmodule.c', 'deps/cpython/Modules/errnomodule.c',
     'deps/cpython/Modules/_struct.c', 'deps/cpython/Modules/mathmodule.c',
     'deps/cpython/Modules/_math.c', 'deps/cpython/Modules/timemodule.c',
     'deps/cpython/Modules/itertoolsmodule.c', 'deps/cpython/Modules/_functoolsmodule.c',
     'deps/cpython/Modules/atexitmodule.c', 'deps/cpython/Modules/arraymodule.c',
     'deps/cpython/Modules/zlibmodule.c',
     ]
    ,
    skip(Glob('deps/cpython/Modules/zlib/*.c'), ['example.c', 'minigzip.c'])
    ,
    Glob('deps/cpython/Modules/_io/*.c')
    ,
    Glob('src/py/*.c'),
    Glob('app/*.c'),
    ]
    )

######################
#     User app       #
######################

app = env_chibios.Object(skip(Glob('app/*.c'), ['appmodule.c']), Glob('app/*.S'))



######################
# Filesystem & rest  #
######################

mkinitfs = Command('initfs.bin', '', mkinitfs.main)
AlwaysBuild(mkinitfs)

initfs = env_chibios.Object('initfs.S')
env_chibios.Depends(initfs, 'initfs.bin') # The include dependency for assembly is not found by scons, add it manually

pipyos = env_chibios.Program('pipyos.elf', [
    chibios,
    python,
    app,
    'main_pipyos.c',
    'libm.a',
    initfs,
    ]
    )

z = 'deps/cpython/Modules/zlib/'

zloader = env_py.Program('zloader.elf', [
     z+ 'inflate.c', z + 'adler32.c', z+ 'crc32.c', z + 'inffast.c', z + 'inftrees.c',
    'src/zloader/zloader.c', 'src/zloader/zloader_asm.s'
    ]
    )



Command('pipyos.img', 'pipyos.elf', 'arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
Command('zloader.img', 'zloader.elf', 'arm-none-eabi-objcopy -O binary $SOURCE $TARGET')
Command('pipyosz.img', ['zloader.img', 'pipyos.img'], mkzloader.main)
