import re
from pathlib import Path


def define_env(env):
    version_h = Path(__file__).parent.parent / "dtmc_base_library/include/dtmc_base/version.h"
    text = version_h.read_text()
    major = re.search(r"DTMC_BASE_VERSION_MAJOR\s+(\d+)", text).group(1)
    minor = re.search(r"DTMC_BASE_VERSION_MINOR\s+(\d+)", text).group(1)
    patch = re.search(r"DTMC_BASE_VERSION_PATCH\s+(\d+)", text).group(1)
    env.variables["dtmc_base_version"] = f"{major}.{minor}.{patch}"
