import subprocess

revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")
)

if revision == "":
  revision = "-------"

print(" -DFW_GIT_REV='%s'" % revision[:7], end='')
