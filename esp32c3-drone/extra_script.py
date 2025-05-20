# extra_script.py
import subprocess
import sys
from SCons.Script import AlwaysBuild, Default, DefaultEnvironment, Exit
print("====================================")
print("EXTRA SCRIPT IS RUNNING!")
print("====================================")

Import("env")


def build_and_upload_fs(source, target, env):
    print("Building filesystem...")
    env.Execute(
        f'$PYTHONEXE -m platformio run --target buildfs --environment {env["PIOENV"]}'
    )
    print("Uploading filesystem...")
    env.Execute(
        f'$PYTHONEXE -m platformio run --target uploadfs --environment {env["PIOENV"]}'
    )


env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", build_and_upload_fs)
