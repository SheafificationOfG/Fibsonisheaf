import argparse
import glob
import os
import sys

from . import HeaderBuilder, Macro

IMPL_DIR = "./impl"


def __imports():
    path = os.path.dirname(__file__)
    imports = dict[str, type[HeaderBuilder]]()
    for file in glob.glob(os.path.join(IMPL_DIR, "*.c")):
        fname = os.path.split(file)[1]
        name = os.path.splitext(fname)[0]
        pymodule = os.path.join(path, f"{name}.py")
        if os.path.exists(pymodule):
            exec(f"from .{name} import Header as {name}", globals(), imports)
        else:
            imports[name] = HeaderBuilder

    return imports


if __name__ == "__main__":

    imports = __imports()

    parser = argparse.ArgumentParser("Auto-header generator.")
    parser.add_argument(
        "-D",
        "--define",
        type=Macro,
        dest="macros",
        action="append",
        default=[],
        help="C macros (-DFOO or -DFOO=bar)",
    )
    parser.add_argument("--verbose", action="store_true", help="Print verbose output")
    parser.add_argument(
        "object",
        choices=imports.keys(),
        help="the name of the object for which a header is being generated",
    )
    parser.add_argument("--folder", help="Folder for generated headers.", required=True)

    args = parser.parse_args()

    if args.verbose:
        logger = lambda *args: print(*args, file=sys.stderr)
    else:
        logger = lambda *_: None

    if not os.path.exists(args.folder):
        print("[#]", "mkdir", args.folder)
        os.mkdir(args.folder)

    header = os.path.join(args.folder, f"{args.object}.h")
    header_tmp = os.path.join(args.folder, f".{args.object}.h.tmp")
    try:
        with open(header_tmp, "w") as file:
            imports[args.object](file, args.macros, logger).main()
    except Exception as exc:
        logger(
            "[x]",
            f"{type(exc).__qualname__}: {exc}",
            f"Partial file written to {header_tmp}.",
        )
        exit(-1)

    os.replace(header_tmp, header)

    logger("[#]", f"{header} generated successfully")
