#! /usr/bin/env python
# -*- coding: utf-8 -*-

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from downward.experiment import FastDownwardExperiment
from downward.reports.absolute import AbsoluteReport
from downward.reports.scatter import ScatterPlotReport

import os
import os.path
import platform

def fd_finished(run):
    print(run['error'])
    if(run['error'] == 'incomplete-search-found-no-plan'):
        run['fd_finished'] = 1
    else:
        run['fd_finished'] = 0
    return run

def verify_timeout(run):    
    if(run['verify_returncode'] == 7):
        run['verify_timeout'] = 1
    else:
        run['verify_timeout'] = 0
    return run
def verify_oom(run):
    if(run['verify_returncode'] == 6):
        run['verify_oom'] = 1
    else:
        run['verify_oom'] = 0
    return run
def verify_finished(run):
    if(not run['unsolv_is_certificate'] == 'unknown'):
        run['verify_finished'] = 1
    else:
        run['verify_finished'] = 0 
    return run
def invalid_certificate(run):
    if(run['unsolv_is_certificate'] == 'no'):
        run['invalid_certificate'] = 1
    else:
        run['invalid_certificate'] = 0
    return run
def valid_certificate(run):
    if(run['unsolv_is_certificate'] == 'yes'):
        run['valid_certificate'] = 1
    else:
        run['valid_certificate'] = 0
    return run
def search_time_wo_cert(run):
    if('search_time' in run and 'write_cert_time' in run):
        run['search_time_wo_cert'] = run['search_time'] - run['write_cert_time']
    return run


SUITE = ["3unsat", "bag-barman", "bag-gripper", "bag-transport", "bottleneck", "cave-diving", "chessboard-pebbling", "diagnosis", "document-transfer", "mystery", "over-nomystery", "over-rovers", "over-tpp", "pegsol", "pegsol-row5", "sliding-tiles", "tetris", "unsat-nomystery", "unsat-rovers", "unsat-tpp"]
# for test purposes on the grid
#SUITE = ['bottleneck:prob03.pddl']

NODE = platform.node()
if NODE.endswith(".scicore.unibas.ch") or NODE.endswith(".cluster.bc2.ch"):
    ENV = BaselSlurmEnvironment(email="salome.eriksson@unibas.ch")
else:
    SUITE = ['bottleneck:prob03.pddl']
    ENV = LocalEnvironment(processes=2)

# paths to repository, benchmarks and revision cache
REPO = os.path.expanduser('~/downward-unsolvability/')
BENCHMARKS_DIR = os.path.expanduser('~/unsolv-total')
REVISION_CACHE = os.path.expanduser('~/lab/revision-cache')

exp = FastDownwardExperiment(environment=ENV, revision_cache=REVISION_CACHE, verify_experiment=True)

# Add default parsers to the experiment.
exp.add_parser('lab_driver_parser', exp.LAB_DRIVER_PARSER)
exp.add_parser('exitcode_parser', exp.EXITCODE_PARSER)
exp.add_parser('translator_parser', exp.TRANSLATOR_PARSER)
exp.add_parser('single_search_parser', exp.SINGLE_SEARCH_PARSER)


exp.add_suite(BENCHMARKS_DIR, SUITE)

# Algorithms
REVISIONS = ['tip']
GEN_CERT = ['false', 'true']
CONFIGS = []

CONFIGS.append(['mas', 'merge_and_shrink(merge_strategy=merge_precomputed(merge_tree=linear()), shrink_strategy=shrink_bisimulation(), label_reduction=exact(before_shrinking=true,before_merging=false))'])
CONFIGS.append(['hmax', 'hmax()'])


ALGS = []
for rev in REVISIONS:
   for config in CONFIGS:
       for gen in GEN_CERT:
           config_name, config_desc = config
           if(gen == 'true'):
              config_name += '-certifying'
           ALGS.append(config_name)
           # changing verifier time and memory limit is currently done directly in lab
           exp.add_algorithm(config_name, REPO, rev, 
                             ["--search", "astar(%s, generate_certificate=%s, certificate_directory=$TMPDIR)" % (config_desc, gen)], 
                             driver_options=["--translate-time-limit", "30m", "--translate-memory-limit","2G"], 
                             build_options=['--build-verifier'])

      

# Add script to print filesize
exp.add_resource('filesize_script', 'print_filesize')
exp.add_command('print-filesize', ['./{filesize_script}'])

# Add custom parser only after filesize is printed, as it parses for this
exp.add_parser('verify_parser', 'myparser.py')

# Example report over all configs and with most important attributes
report = os.path.join(exp.eval_dir, 'test.html')
exp.add_report(AbsoluteReport(filter_algorithm=ALGS, filter=[fd_finished, verify_timeout, verify_oom, verify_finished, invalid_certificate, valid_certificate, search_time_wo_cert], attributes=['fd_finished', 'verify_timeout','verify_oom', 'verify_finished', 'invalid_certificate', 'valid_certificate', 'search_time_wo_cert', 'run_dir', 'error', 'raw_memory', 'evaluated', 'dead_ends', 'expansions', 'search_time', 'total_time', 'unsolv_total_time', 'unsolv_actions', 'unsolv_is_certificate', 'unsolv_memory', 'unsolv_abort_memory', 'unsolv_abort_time', 'unsolv_exit_message', 'verify_returncode', 'write_cert_time', 'certificate_size_kb']), outfile=report)


#Example for scatterplot
#exp.add_report(ScatterPlotReport(format='tex', attributes=['total_time'], filter_algorithm=['mas', 'mas-certifying']), outfile=os.path.join(exp.eval_dir, 'test.tex'))


exp.run_steps()

