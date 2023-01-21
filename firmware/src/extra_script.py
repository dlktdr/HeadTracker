Import("env")

# Dump build environment (for debug)
# print(env.Dump())

build_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in build_flags.get("CPPDEFINES")}

fileprefix = defines.get("FNAME")
versiontag = defines.get("FW_VER_TAG")
versiontag = versiontag.replace(".", "_")
gitrev = defines.get("FW_GIT_REV")

# The changing filename causes issues when debugging with a J-Link and
# uploading. If built in debug mode, always keep the filename the same

if env['BUILD_TYPE'] == "debug":
  env.Replace(PROGNAME="DEBUG")
else:
  env.Replace(PROGNAME="%s_v%s_%s" % (fileprefix, versiontag, gitrev))
