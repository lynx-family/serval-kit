import os
import platform
system = platform.system().lower()
machine = platform.machine().lower()
machine = "x86_64" if machine == "amd64" else machine
python_path = "python3"
if system == "windows":
    python_path = "python"
deps = {
    "./tools_shared": {
        "type": "solution",
        "url": "https://github.com/lynx-family/tools-shared.git",
        "commit": "a1f5807a5e89b52d4c015a946371bd29730a31c8",
        'deps_file': 'dependencies/DEPS',
        "ignore_in_git": True,
    },
    "./markdown/third_party/lynx-textra": {
        'type': 'git',
        'url': 'https://github.com/lynx-family/lynx-textra.git',
        'commit': '27d334449d6521f7809fc9e80713932213a8c264',
        'ignore_in_git': True,
    },
}
