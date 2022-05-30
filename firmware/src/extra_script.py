Import("env")

# access to global build environment
print(env)

# access to project build environment (is used source files in "src" folder)

my_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}
print(defines)
env.Replace(PROGNAME="BLE%s v%s_%s%s_%s" % (defines.get("FW_EXTRA"),
                                            defines.get("FW_MAJ"),
                                            defines.get("FW_MIN"),
                                            defines.get("FW_REV"),
                                            defines.get("FW_GIT_REV")))
