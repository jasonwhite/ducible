#!/usr/bin/env python3
# Copyright (c) 2016 Jason White
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

"""
Generates a header file with version information about the program. The major,
minor, and patch versions are taken from the VERSION file at the root of the
repository. The current Git commit hash is also used in the version string.
"""

import re
import sys
import subprocess
import argparse

def git_version():
    """
    Returns a short Git commit hash for the current repository.
    """
    return subprocess.check_output(['git', 'describe', '--always', '--dirty']) \
            .decode('utf-8')\
            .strip()

def git_commit_short():
    """
    Returns a short Git commit hash for the current repository.
    """
    return subprocess.check_output(['git', 'describe', '--always']) \
            .decode('utf-8')\
            .strip()

def git_commit_long():
    """
    Returns a full length Git commit hash for the current repository.
    """
    return subprocess.check_output(['git', 'rev-parse', 'HEAD']) \
            .decode('utf-8')\
            .strip()

def version_info(version_file):
    """
    Returns a tuple of version information using the given version string.
    """

    info = {}

    version_string = next(version_file).strip()

    version = tuple([x for x in version_string.split('.')][0:3])

    info['MAJOR'] = version[0]
    info['MINOR'] = version[1]
    info['PATCH'] = version[2]
    info['GIT_VERSION'] = git_version()
    info['GIT_COMMIT_SHORT'] = git_commit_short()
    info['GIT_COMMIT_LONG'] = git_commit_long()

    return info

def substitute_variables(input_file, output_file, variables):
    """
    Replaces the variables in the given input file, writing the transformed text
    into the output file.
    """

    content = input_file.read()

    re_macro = re.compile(r'\${([^}]*)}')

    try:
        output_file.write(re_macro.sub(lambda m: variables[m.group(1)], content))
    except KeyError as e:
        print('Variable %s is not defined' % e, file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Generates version.h')
    parser.add_argument(
            'input',
            nargs='?',
            type=argparse.FileType('r'),
            help='Path to the version.h.in file.',
            default=sys.stdin
            )
    parser.add_argument(
            'output',
            nargs='?',
            type=argparse.FileType('w'),
            default=sys.stdout
            )
    parser.add_argument(
            '--version-file',
            default='VERSION',
            type=argparse.FileType('r'),
            help='Path to the VERSION file.'
            )
    args = parser.parse_args()

    version = version_info(args.version_file)

    substitute_variables(args.input, args.output, version)
