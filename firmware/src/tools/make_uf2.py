Import("env")
import os
import os.path

def after_hex_build(source, target, env):
   firmware_path = str(source[0])
   firmware_name = os.path.splitext(firmware_path)[0]
   command = "python ./tools/uf2conv.py " + firmware_path +  " -o " + firmware_name + ".uf2 -f 0xADA52840 -c -b 0x27000"
   print("Converting .bin to .uf2 file")
   os.system(command)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", after_hex_build)