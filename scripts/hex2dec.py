import sys
import typing

sys.set_int_max_str_digits(0)

# too lazy to write a double-dabble in C


def hex2dec(fp: typing.TextIO, src: str, *, ndigits: int):
    try:
        x = int(src, base=16)
    except ValueError:
        index, char = next(
            filter(lambda p: p[1] not in "0123456789abcdef", enumerate(src.lower()))
        )
        print(f"Invalid character {char!r} at index {index+1}.", file=sys.stderr)
        exit(1)

    dec = str(x)
    if ndigits == -1 or len(dec) <= ndigits:
        fp.write(dec)
    else:
        lead, *rest = dec[: ndigits + 1]
        fp.write(lead)
        fp.write(".")
        fp.write("".join(rest))
        fp.write(f"e{len(dec)-1}")


if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-i",
        "--input",
        metavar="FILE",
        help="Input source. If not provided, input is read from stdin.",
    )
    parser.add_argument(
        "-o",
        "--output",
        metavar="FILE",
        help="Output. If not provided, output is written to stdout.",
    )
    parser.add_argument(
        "-n",
        "--ndigits",
        default=32,
        help="Number of digits to emit, or 0 to emit all digits.",
    )

    args = parser.parse_args()

    source_file = open(args.input) if args.input is not None else sys.stdin
    output_file = open(args.output, "w") if args.output is not None else sys.stdout

    source = "".join(map(str.strip, source_file))
    if args.input is not None:
        source_file.close()

    hex2dec(output_file, source, ndigits=args.ndigits)

    if args.output is not None:
        output_file.close()
    else:
        output_file.write("\n")
