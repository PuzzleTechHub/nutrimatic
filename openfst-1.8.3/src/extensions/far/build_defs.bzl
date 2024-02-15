# Copyright 2005-2024 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Helper library to define BUILD rules for FAR manipulation."""

def convert_far_types(name, far_in, far_out, far_type = None, fst_type = None, extra_args = None, **kwds):
    """Converts the FAR type and/or FST types in a FAR, writing a new FAR.

    Args:
      name: The BUILD rule name.
      far_in: The input FAR file.
      far_out: The output FAR file with converted FST types.
      far_type: An optional string specifying the desired type of the FAR in far_out.
                If None, the input FAR's type will be used.
      fst_type: An optional string specifying the desired type of the FSTs in far_out.
      extra_args: Optional additional flags to pass to convert invocation.
                If None, each FST will retain its input type.
      **kwds: Attributes common to all BUILD rules, e.g., testonly, visibility.
    """

    farconvert_rule = "@org_openfst//:farconvert"
    farconvert_cmd = "$(location %s) " % farconvert_rule
    if not far_type and not fst_type:
        fail("No-op conversion for FAR %s." % far_in)
    if far_type:
        farconvert_cmd += " --far_type=%s" % far_type
    if fst_type:
        farconvert_cmd += " --fst_type=%s" % fst_type
    if extra_args:
        farconvert_cmd += " %s" % extra_args
    farconvert_cmd += " $(location %s)" % far_in
    farconvert_cmd += " $(location %s)" % far_out

    native.genrule(
        name = name,
        srcs = [far_in],
        tools = [farconvert_rule],
        outs = [far_out],
        cmd = farconvert_cmd,
        message = "Converting FST type in FAR %s ==> %s" % (far_in, far_out),
        **kwds
    )

def extract_fsts_from_far(name, far, fsts, **kwds):
    """Given a FAR (FST archive), we'll extract individual FST files from it.

    Args:
      name: The BUILD rule name.
      far: The source FAR from which we'll find the FSTs.
      fsts: A list of FSTs to extract from the far.  Each argument should be a
            filename with a '.fst' extension.
      **kwds: Attributes common to all BUILD rules, e.g., testonly, visibility.
    """

    farextract_rule = "//nlp/fst/static:far"
    fst_extension = ".fst"

    fst_prefixes = ""
    for fst in fsts:
        if not fst.endswith(fst_extension):
            fail("FST output must end with .fst extension: %s" % fst)
        if fst_prefixes:
            fst_prefixes += ","
        fst_prefixes += fst[:-len(fst_extension)]

    farextract_cmd = "$(location %s) extract" % farextract_rule
    farextract_cmd += " --keys=%s" % fst_prefixes
    farextract_cmd += " --filename_prefix=$(@D)/"
    farextract_cmd += " --filename_suffix=%s" % fst_extension
    farextract_cmd += " $(location %s)" % far

    native.genrule(
        name = name,
        srcs = [far],
        tools = [farextract_rule],
        outs = fsts,
        cmd = farextract_cmd,
        **kwds
    )
