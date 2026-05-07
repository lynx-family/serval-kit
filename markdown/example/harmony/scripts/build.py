#!/usr/bin/env python3
# Copyright 2025 The Lynx Authors. All rights reserved.
# Licensed under the Apache License Version 2.0 that can be found in the
# LICENSE file in the root directory of this source tree.

import argparse
import os
import shutil
import sys
from subprocess import check_call


CUR_DIR = os.path.dirname(os.path.abspath(__file__))
HARMONY_DIR = os.path.normpath(os.path.join(CUR_DIR, '..'))

def resolve_hvigorw():
    hvigorw_from_env = os.environ.get('HVIGORW')
    if hvigorw_from_env:
        return hvigorw_from_env
    hvigorw_from_path = shutil.which('hvigorw')
    if hvigorw_from_path:
        return hvigorw_from_path
    raise Exception('hvigorw not found in PATH or HVIGORW env')

def is_valid_harmony_sdk_path(sdk_path):
    if not sdk_path:
        return False
    return (
        os.path.isdir(sdk_path)
        and os.path.isdir(os.path.join(sdk_path, 'hms'))
        and os.path.isdir(os.path.join(sdk_path, 'openharmony'))
    )

def resolve_harmony_sdk_path():
    candidates = []
    harmony_home = os.environ.get('HARMONY_HOME')
    if harmony_home:
        candidates.append(harmony_home)
        candidates.append(os.path.join(harmony_home, 'default'))

    for candidate in candidates:
        candidate = os.path.normpath(candidate)
        if is_valid_harmony_sdk_path(candidate):
            return candidate
    return None

def resolve_deveco_sdk_home(harmony_sdk_path):
    deveco_sdk_home = os.environ.get('DEVECO_SDK_HOME')
    if deveco_sdk_home and os.path.isdir(os.path.join(deveco_sdk_home, 'default')):
        return os.path.normpath(deveco_sdk_home)
    if harmony_sdk_path and os.path.basename(harmony_sdk_path) == 'default':
        return os.path.dirname(harmony_sdk_path)
    return None

def read_hwsdk_dir(local_properties_path):
    if not os.path.exists(local_properties_path):
        return None
    with open(local_properties_path, 'r') as f:
        for line in f:
            line = line.strip()
            if line.startswith('hwsdk.dir='):
                return line[len('hwsdk.dir='):]
    return None

def ensure_local_properties():
    local_properties_path = os.path.join(HARMONY_DIR, 'local.properties')
    hwsdk_dir = read_hwsdk_dir(local_properties_path)
    if is_valid_harmony_sdk_path(hwsdk_dir):
        return hwsdk_dir

    harmony_sdk_path = resolve_harmony_sdk_path()
    if not harmony_sdk_path:
        print('harmony/local.properties not found or invalid, and no valid Harmony SDK path was found.')
        return None

    if hwsdk_dir:
        print('harmony/local.properties has invalid hwsdk.dir {}, rewrite it with {}'.format(hwsdk_dir, harmony_sdk_path))
    else:
        print('harmony/local.properties not found, write hwsdk.dir with {} to it'.format(harmony_sdk_path))
    with open(local_properties_path, 'w') as f:
        f.write('hwsdk.dir={}'.format(harmony_sdk_path))
    return harmony_sdk_path

def patch_oh_package(module_path, version):
    cmd = 'ohpm version {}'.format(version)
    print(f'run command {cmd} in {module_path}')
    check_call(cmd, shell=True, cwd=module_path)
    
    package_file = os.path.join(module_path, "oh-package.json5")
    # print replaced file
    with open(package_file, "r") as f:
        print(f.read())

def collect_module_config_list(args):
    import json5
    with open(os.path.join(HARMONY_DIR,'build-profile.json5'), 'r') as f:
        build_profile = json5.load(f)

    module_config_list = build_profile['modules']
    if args.verbose:
        print('module_config_list is'+ str(module_config_list))
    return module_config_list

def run_package_har(module_name, module_path, build_mode, verbose):
    if verbose:
        print(f'===== start run package {module_name} =====')

    harmony_sdk_path = ensure_local_properties()

    hvigorw = resolve_hvigorw()
    cmd = [
        hvigorw,
        'assembleHar',
        '--mode',
        'module',
        '-p',
        f'module={module_name}@default',
        '-p',
        'product=default',
        '-p',
        f'buildMode={build_mode}',
        '--no-daemon'
    ]
    if verbose:
        print(f'run command {" ".join(cmd)}')
    env = os.environ.copy()
    deveco_sdk_home = resolve_deveco_sdk_home(harmony_sdk_path)
    if deveco_sdk_home:
        env['DEVECO_SDK_HOME'] = deveco_sdk_home
    check_call(cmd, cwd=HARMONY_DIR, env=env)
    # as even hvigor build failed, it still return value 0, so we need to check har file exist or not
    har_path = os.path.join(HARMONY_DIR, module_path, 'build', 'default', 'outputs', 'default', f'{module_name}.har')
    print(f'har_path is {har_path}')
    if not os.path.isfile(har_path):
        raise Exception('har file not found, please check your build')

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--verbose", action="store_true", default=False, help="verbose print")
    parser.add_argument("--version", type=str, default=None, help="override version")
    parser.add_argument("--publish_har", action="store_true", default=False, help="publish har")
    parser.add_argument("--build_mode", type=str, choices=["debug", "release"], default=None, help="build mode")
    parser.add_argument("--modules", nargs="*", help="list of modules name")

    args = parser.parse_args()

    modules = args.modules if args.modules else []
    if len(modules) == 0:
        print('no module specified to build')
        return 0

    for module in modules:
        module_config_list = collect_module_config_list(args)
        for module_config in module_config_list:
            if module_config['name'] == module:
                module_path = module_config['srcPath']
                break
        else:
            raise Exception(f"module {module} not found in build-profile.json5")
        module_full_path = os.path.join(HARMONY_DIR, module_path)
        if args.publish_har:
            if args.version:
                publish_version = args.version
                patch_oh_package(module_full_path, publish_version)
            else:
                print(f"version is required when publish har")
                return 1
        build_mode = args.build_mode if args.build_mode else ("release" if args.publish_har else "debug")
        run_package_har(module, module_full_path, build_mode, args.verbose)


if __name__ == "__main__":
    sys.exit(main())
