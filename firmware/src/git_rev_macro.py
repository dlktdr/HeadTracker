import subprocess

revision = ""
try:
  revision = (
      subprocess.check_output(["git", "rev-parse", "HEAD"])
      .strip()
      .decode("utf-8")
  )
except:
  revision = "NO_GIT"

versiontag = ""
try:
  versiontag = (
    subprocess.check_output(["git","describe","--tags","--abbrev=0"])
    .strip()
    .decode("utf-8")
  )
except:
  versiontag = "v0.00"

print(" -DFW_GIT_REV='%s'" % revision[:7], end='')
print(" -DFW_VER_TAG='%s'" % versiontag[1:], end='')
