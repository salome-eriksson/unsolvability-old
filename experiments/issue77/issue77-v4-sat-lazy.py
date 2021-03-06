#! /usr/bin/env python
# -*- coding: utf-8 -*-

import downward.suites

import common_setup
import configs

CONFIGS = configs.default_configs_satisficing(ipc=False, extended=False)

DISABLED = [
]
for key, value in list(CONFIGS.items()):
    if not key.startswith("lazy"):
        del CONFIGS[key]
print(sorted(CONFIGS.keys()))
print(len(CONFIGS))


SUITE = downward.suites.suite_satisficing_with_ipc11()

exp = common_setup.IssueExperiment(
    search_revisions=["issue77-base", "issue77-v4"],
    configs=CONFIGS,
    suite=SUITE
    )
exp.add_absolute_report_step()
exp.add_comparison_table_step()
# exp.add_scatter_plot_step()

exp()
