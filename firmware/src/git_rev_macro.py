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

if revision == "":
  revision = "-------"

print(" -DFW_GIT_REV='%s'" % revision[:7], end='')
