Import("env")

import os
import subprocess

from SCons.Script import Action


def upload_filesystem(source, target, env):
    if env["PIOENV"] != "waveshare_esp32s3_touch_amoled_143c":
        return

    nested_env = os.environ.copy()
    nested_env["RSVP_SKIP_AUTO_UPLOADFS"] = "1"
    subprocess.run(
        ["pio", "run", "-e", env["PIOENV"], "-t", "uploadfs"],
        cwd=env["PROJECT_DIR"],
        check=True,
        env=nested_env,
    )


if os.environ.get("RSVP_SKIP_AUTO_UPLOADFS") != "1":
    env.AddPreAction("upload", Action(upload_filesystem))
