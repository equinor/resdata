import argparse
import logging


def main():
    parser = argparse.ArgumentParser(description='Do python logging on file.')
    parser.add_argument('input_file', type=str,
                        help='File that will be logged')

    args = parser.parse_args()
    print("logging stuff to azure")
    with open(args.input_file, "r") as fin:
        logger = logging.getLogger()
        logger.setLevel(logging.INFO)
        logger.addHandler(logging.StreamHandle())
        txt = fin.read()
        print(txt)
        logger.error(txt)
