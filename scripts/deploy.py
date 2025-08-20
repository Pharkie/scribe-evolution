#!/usr/bin/env python3
# pylint: disable=undefined-variable
# type: ignore

import subprocess

Import("env")  # pylint: disable=undefined-variable  # type: ignore


def deploy_all(source, target, env):  # pylint: disable=unused-argument
    """Deploy complete workflow: build + upload fs + upload firmware + monitor"""
    subprocess.run(["npm", "run", "build"], check=True)
    env.Execute("pio run -e main -t uploadfs")
    env.Execute("pio run -e main -t upload")
    env.Execute("pio run -e main -t monitor")


env.AddCustomTarget(  # pylint: disable=undefined-variable  # type: ignore
    "deploy_all",
    None,
    deploy_all,
    "Deploy All",
    "Build + Upload FS + Upload Firmware + Monitor",
)
