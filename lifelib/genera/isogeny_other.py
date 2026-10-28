'''
Isotropic generations
'''

from .generations import number_of_states

import subprocess

from .eightbit import *
from .eightbit import create_rule as make_header
from .rulefiles import rule2files
from ._caviewer import download


def create_rule(rulestring):
    download()  # Download CAViewer

    nstates = number_of_states(rulestring)

    if (nstates < 3):
        raise ValueError("Number of states must be at least 3")

    preface = os.path.dirname(os.path.abspath(__file__)) + '/bin/CAViewer'
    if rulestring[-2:] in ["v2", "fe", "fc"] or rulestring[-1:] in ["h"]:
        rulestring = rulestring[:-2] + rulestring[-2:].upper()

    # Canonising rulestring
    p = subprocess.Popen(
        preface + " info -r " + rulestring,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out = p.communicate()

    if out[1].decode("utf-8"):
        raise Exception(out[1].decode("utf-8"))

    # Change from B.../S.../N... to b...s......
    tokens = out[0].decode("utf-8").split("\n")[0].replace("Rulestring: ", "").split("/")
    if len(tokens) == 4:
        rulestring = "g" + tokens[2] + "b" + tokens[1] + "s" + tokens[0] + tokens[3].upper()
    else:
        rulestring = "g" + tokens[2] + "b" + tokens[1] + "s" + tokens[0]

    # Generating rulefile
    p = subprocess.Popen(
        preface + " apgtable -r " + rulestring + " -o " + rulestring.lower() + ".rule",
        stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out = p.communicate()

    if out[1].decode("utf-8"):
        raise Exception(out[1].decode("utf-8"))

    # Make rulestring lowercase again
    rulestring = rulestring.lower()

    # Convert .rule file into C/C++ code:
    rule2files(rulestring + '.rule')

    # Link code into lifelib:
    make_header(rulestring)
