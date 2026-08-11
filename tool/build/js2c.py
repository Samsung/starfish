#!/usr/bin/env python3

#
# Copyright (c) 2022-present Samsung Electronics Co., Ltd
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
#  USA
#

# Use Black to format this file.

from __future__ import print_function
import os
import errno
import optparse
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from repo_paths import REPO_ROOT  # noqa: E402


def mkdir_p(path):
    # https://stackoverflow.com/a/600612/119527
    try:
        os.makedirs(path)
    except OSError as exc:  # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else:
            raise


def snake_case(s):
    return re.sub("[.-]", "_", s).lower()


def print_gen(source_path, output_path):
    print(
        "%s -> %s"
        % (
            os.path.relpath(source_path, ROOT_DIR),
            os.path.relpath(output_path, ROOT_DIR),
        )
    )


def read_lincese_file(filename):
    result = []
    result.append("/*\n")
    with open(filename, "r") as file:
        lines = file.readlines()
        for line in lines:
            if len(line) > 0:
                result.append(" * " + line)
    result.append("*/\n")
    return "".join(result)


# constants

ROOT_DIR = REPO_ROOT
OUTPUT_FILE_PREFIX = "Js2c_"

LICENSE = """/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */
"""


TEMPLATE = """{License}
{SourceLicense}
// NOTE: This file was generated from .js source file.

#if defined(STARFISH_ENABLE_SERVICE_WORKER)
#include <string>

namespace Starfish {{
  // {SourceName}
  static const std::string s_js2c_{ValueName} = R"({Source})";
}} // namespace Starfish
#endif /* STARFISH_ENABLE_SERVICE_WORKER */
"""


def main(opts):
    # prepare meta
    source_path = opts.source
    output_path = opts.output
    license_path = opts.license
    source_basename = os.path.basename(opts.source)
    source_license = (
        read_lincese_file(license_path) if license_path != "" else license_path
    )

    # start generating output
    with open(source_path, "r") as source_file:
        rendered = TEMPLATE.format(
            Source=source_file.read(),
            SourceName=source_basename,
            ValueName=snake_case(source_basename),
            License=LICENSE,
            SourceLicense=source_license,
        )
        with open(output_path, "w") as output_file:
            output_file.write(rendered)

    print_gen(source_path, output_path)


def setupCLIOptions(parser):
    optgroup = optparse.OptionGroup(parser, "js2c.py")

    optgroup.add_option(
        "-s",
        "--source-path",
        type="string",
        dest="source",
        default="",
        help="set source file path",
    )

    optgroup.add_option(
        "-o",
        "--output",
        type="string",
        dest="output",
        default="",
        help="set output file path",
    )

    optgroup.add_option(
        "-l",
        "--license-path",
        type="string",
        dest="license",
        default="",
        help="set LICENSE file path",
    )

    parser.add_option_group(optgroup)
    return parser


USAGE = """%prog [options]

Example:
  %prog \\
    -s src/core/modules/serviceworker/cache/deps/cache-storage/dist/cache.min.js \\
    -l src/core/modules/serviceworker/cache/deps/cache-storage/LICENSE \\
    -o out/debug/starfish_generated/CacheStorage.h"""

if __name__ == "__main__":
    parser = setupCLIOptions(optparse.OptionParser(USAGE))
    (options, args) = parser.parse_args()
    main(options)
