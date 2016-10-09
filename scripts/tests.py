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
Runs tests to make sure that builds really are reproducible.

This works by building twice and comparing the checksums of the outputs.

It is assumed that we are in a Visual Studio environment (i.e., vcvarsall.bat
has been run).
"""

import os
import sys
import json
import shutil
import fnmatch
import hashlib
import argparse
import subprocess


_script_dir = os.path.dirname(os.path.realpath(__file__))

class MismatchException(Exception):
    """
    Thrown when a checksum mismatch is detected in a test case.
    """
    pass

class Test:
    """
    Represents a single test.
    """

    def __init__(self, name, workdir, commands, args, clean_files):
        self.name = name
        self.workdir = workdir
        self.commands = commands
        self.args = args
        self.clean_files = clean_files

    def run(self, ducible):
        """
        Runs a single test.

        Throws an exception if the test failed.
        """

        ducible = os.path.abspath(ducible)

        outputs = [os.path.join(self.workdir, o) for o in self.args]

        # Run the commands to do the build
        for command in self.commands:
            subprocess.check_call(command, cwd=self.workdir)

        # Attempt to eliminate nondeterminism
        subprocess.check_call([ducible] + self.args, cwd=self.workdir)

        checksums_1 = [hash_file(o).digest() for o in outputs]

        self.clean()

        # Run the commands to do the build (again)
        for command in self.commands:
            subprocess.check_call(command, cwd=self.workdir)

        # Attempt to eliminate nondeterminism (again)
        subprocess.check_call([ducible] + self.args, cwd=self.workdir)

        checksums_2 = [hash_file(o).digest() for o in outputs]

        self.clean()

        mismatches = [i for i,c in enumerate(zip(checksums_1, checksums_2))
                        if c[0] != c[1]]

        if mismatches:
            print('Error: The following files are not reproducible:')
            for m in mismatches:
                print('  {}'.format(self.args[m]))

            raise MismatchException('Some files are not reproducible')

    def analyze(self, ducible):
        """
        Creates an environment to make analyzing non-determinism easier.

        In particular, it creates a hexdump of all files and puts them
        side-by-side in the test's directory for easy diffing.
        """

        ducible = os.path.abspath(ducible)
        analysis = os.path.join(self.workdir, 'analysis')

        os.makedirs(analysis, exist_ok=True)

        outputs = [os.path.join(self.workdir, o) for o in self.args]

        # Run the commands to do the build
        for command in self.commands:
            subprocess.check_call(command, cwd=self.workdir)

        # Copy *original* outputs to the analysis directory (round 1)
        for o in outputs:
            shutil.copyfile(o, os.path.join(analysis,
                os.path.basename(o)+'.1.orig'))

        # Attempt to eliminate nondeterminism
        subprocess.check_call([ducible] + self.args, cwd=self.workdir)

        # Copy *rewritten* outputs to the analysis directory (round 1)
        for o in outputs:
            shutil.copyfile(o, os.path.join(analysis,
                os.path.basename(o)+'.1.rewritten'))

        self.clean()

        # Run the commands to do the build (again)
        for command in self.commands:
            subprocess.check_call(command, cwd=self.workdir)

        # Copy *original* outputs to the analysis directory (round 2)
        for o in outputs:
            shutil.copyfile(o, os.path.join(analysis,
                os.path.basename(o)+'.2.orig'))

        # Attempt to eliminate nondeterminism (again)
        subprocess.check_call([ducible] + self.args, cwd=self.workdir)

        # Copy *rewritten* outputs to the analysis directory (round 2)
        for o in outputs:
            shutil.copyfile(o, os.path.join(analysis,
                os.path.basename(o)+'.2.rewritten'))

        self.clean()

        # Copy the analyze script there for convenience
        shutil.copy(os.path.join(_script_dir, 'analyze'), analysis)

    def clean(self):
        """
        Deletes the files specified in the 'clean' array.
        """
        root, dirs, files = next(os.walk(self.workdir))

        for f in files:
            for pattern in self.clean_files:
                if fnmatch.fnmatch(f, pattern):
                    print('Deleting \'%s\'' % f)
                    os.remove(os.path.join(root, f))

def hash_file(path, chunk_size=65536):
    hasher = hashlib.md5()

    buf = bytearray(chunk_size)

    with open(path, 'rb') as f:
        while True:
            read = f.readinto(buf)
            if read == 0:
                break
            hasher.update(buf[0:read])

    return hasher


def tests(tests_dir):
    """
    Yields test objects to be executed.
    """
    root, dirs, files = next(os.walk(tests_dir))

    for d in dirs:
        try:
            with open(os.path.join(root, d, 'test.json')) as f:
                obj = json.load(f)
                yield Test(d, os.path.join(root, d),
                        obj['commands'],
                        obj['ducible_args'],
                        obj['clean'])
        except FileNotFoundError:
            # Directory doesn't have a test in it
            pass

def run_all_tests(tests_dir, ducible):
    failed = 0

    for t in tests(tests_dir):
        print(':: Running test "{}"...'.format(t.name))
        try:
            t.run(ducible)
        except MismatchException as e:
            print('Mismatch detected, re-running test for analysis...')
            t.analyze(ducible)

            failed += 1
            print('TEST FAILED:', e)

        except Exception as e:
            failed += 1
            print('TEST FAILED:', e)

    return failed

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Runs tests')
    parser.add_argument('ducible', help='Path to Ducible executable.')
    args = parser.parse_args()

    assert os.environ['VisualStudioVersion'] == '14.0',\
            'You must be in a Visual Studio 2015 command prompt.'

    tests_dir = os.path.relpath(os.path.join(_script_dir, '../tests'))

    failed = run_all_tests(tests_dir, args.ducible)
    if failed > 0:
        print(':: %s test(s) failed' % failed)
        sys.exit(1)
    else:
        print(':: All tests passed')
