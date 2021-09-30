import argparse
import pluggy
import logging

hook_implementation = pluggy.HookimplMarker("ecl")
hook_specification = pluggy.HookspecMarker("ecl")

import ecl.hook_specifications


class EclPluginManager(pluggy.PluginManager):
    def __init__(self):
        super().__init__("ecl")
        self.add_hookspecs(ecl.hook_specifications)
        self.load_setuptools_entrypoints("ecl")

    def add_logging_handle_to_root(self, logger):
        handles = self.hook.add_log_handle_to_root()
        for handle in handles:
            logger.addHandler(handle)


def main():
    parser = argparse.ArgumentParser(description="Do python logging on file.")
    parser.add_argument("input_file", type=str, help="File that will be logged")

    args = parser.parse_args()
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    pm = EclPluginManager()
    pm.add_logging_handle_to_root(logger)
    with open(args.input_file, "r") as fin:
        logger.error(fin.read())
