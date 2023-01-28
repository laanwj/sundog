#!/usr/bin/env python3
# Copyright (c) 2022 Wladimir J. van der Laan
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
'''
Simple resource compiler. Produces a C header file with all files embedded,
sorted by name.
'''
import argparse
import os
import sys
import gzip

# We could have bothered with ld -b, which would be faster at build time,
# but much more of a platform compatibility hassle. So we don't.

def emit_byteobj(f, varname, data, per_line=32):
    f.write('static const uint8_t %s[%d] = {\n' % (varname, len(data)))
    num_lines = (len(data) + per_line - 1) // per_line
    ofs = 0
    for _ in range(num_lines):
        f.write(','.join(f'0x{b:02x}' for b in data[ofs:ofs+per_line]))
        ofs += per_line
        if ofs < len(data):
            f.write(',\n')
        else:
            f.write('\n')
    f.write('};\n')

def c_escape_string(s):
    '''Escape a string for emitting in C code.'''
    # XXX this is far from complete.
    return '"' + s.replace('\\', '\\\\') + '"'

def emit_directory(f, order):
    f.write('''static const struct {
    const char *name;
    uint32_t flags;
    const uint8_t *data;
    size_t size;
} resource_directory[%d] = {
''' % len(order))
    for i, (varname, file_id, flags, data) in enumerate(order):
        f.write('{%s, 0x%08x, %s, %d}' % (c_escape_string(file_id), flags, varname, len(data)))
        if i != len(order) - 1:
            f.write(',\n')
        else:
            f.write('\n')
    f.write('};\n')
    f.write('static const size_t resource_directory_entries = %d;\n' % len(order))



def parse_args():
    parser = argparse.ArgumentParser(description='Simple embedded resource compiler')
    parser.add_argument('inputs', help='Input files', nargs='*')
    parser.add_argument('-o', '--output', required=True, help='Output file (C header)')
    parser.add_argument('-b', '--base-path', help='Root directory of resource (defaults to current directory)')
    # XXX compression?
    return parser.parse_args()

def main():
    args = parse_args()

    if args.base_path is None:
        args.base_path = os.getcwd()

    # Read resources into memory.
    resources = {}
    for filename in args.inputs:
        fullpath = os.path.realpath(filename)
        file_id = os.path.relpath(fullpath, args.base_path).replace(os.path.sep, '/')

        if file_id.endswith('.gz'): # strip .gz suffix and decompress (for now)
            file_id = file_id[:-3]
            with gzip.open(filename, 'rb') as f:
                data = f.read()
        else:
            with open(filename, 'rb') as f:
                data = f.read()

        resources[file_id] = (0, data)

    sorted_ids = sorted(resources.keys())

    # Write C header
    with open(args.output, 'w') as f:
        f.write('// clang-format off\n')
        f.write('/** This is an automatically generated file, do not edit. */\n')

        order = []
        for i, file_id in enumerate(sorted_ids):
            flags, data = resources[file_id]
            varname = f'resource_data{i}'
            emit_byteobj(f, varname, data)
            order.append((varname, file_id, flags, data))

        emit_directory(f, order)

        f.write('// clang-format on\n')

if __name__ ==  '__main__':
    main()
