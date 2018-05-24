#! /usr/bin/env python

import math

from lab.parser import Parser


def is_cert(content, props):
  if('unsolv_is_certificate' not in props):
    props['unsolv_is_certificate'] = 'unknown'
  elif(props['unsolv_is_certificate'] == 'valid'):
    props['unsolv_is_certificate'] = 'yes'
  elif(props['unsolv_is_certificate'] == 'not valid'):
    props['unsolv_is_certificate'] = 'no'

parser = Parser()
parser.add_pattern('unsolv_actions', 'Amount of Actions: (.+)', type=int, required=False)
parser.add_pattern('unsolv_total_time', 'Verify total time: (.+)', type=float, required=False)
parser.add_pattern('unsolv_is_certificate', 'Exiting: certificate is (.+)', type=str, required=False)
parser.add_pattern('unsolv_memory', 'Verify memory: (.+)KB', type=float, required=False)
parser.add_pattern('unsolv_abort_memory', 'abort memory (.+)KB', type=float, required=False)
parser.add_pattern('unsolv_abort_time', 'abort time (.+)s', type=float, required=False)
parser.add_pattern('unsolv_exit_message', 'Exiting: (.+)', type=str, required=True)
parser.add_pattern('write_cert_time', 'Time for writing unsolvability certificate: (.+)', type=float, required=False)
parser.add_pattern('certificate_size_kb', 'Certificate size: (.+)', type=int, required=False)
parser.add_function(is_cert)
parser.parse()
