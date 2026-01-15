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
        "commit": "f13686a7f3ea237475af97b106c894652b2c0ef2",
        'deps_file': 'dependencies/DEPS',
        "ignore_in_git": True,
    },
    "./markdown/third_party/googletest": {
        'type': 'git',
        'url': 'https://github.com/google/googletest',
        'commit': '4a00a24fff3cf82254de382437bf840cab1d3993',
        'ignore_in_git': True,
    },
    "./markdown/third_party/lynx-textra": {
        'type': 'git',
        'url': 'https://github.com/lynx-family/lynx-textra.git',
        'commit': '9d26418825ad2c7d6b8064fceef86b0e65f433ca',
        'ignore_in_git': True,
    },
}
