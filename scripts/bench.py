"""
Really bad benchmarking script.
"""

import math
import os
import re
import subprocess
import sys
import time
import typing
from dataclasses import dataclass

from fibonappy.fast_double import fibonacci


def gen_indices(bases: typing.Iterable[int]):
    bases = list(bases)
    yield 0
    n = 1
    while True:
        for base in bases:
            yield n
            n *= base


@dataclass
class Bench:
    runtime: float
    result: int | None

    @classmethod
    def collect(cls, hexcmd: str, index: int, *, timeout: float = None) -> typing.Self:
        try:
            proc = subprocess.run(
                [hexcmd, str(index)], timeout=timeout, capture_output=True, text=True
            )
        except subprocess.TimeoutExpired:
            return cls(math.inf, -1)

        rtre = re.compile(r"#\s*Runtime:\s*(?P<runtime>[\d.]+)s")
        report = rtre.search(proc.stderr)
        try:
            runtime = float(report.group("runtime"))
        except:
            return cls(math.nan, -1)

        try:
            result = int(proc.stdout, base=16)
        except:
            result = None
        return cls(runtime, result)

    @classmethod
    def golden(cls, index: int) -> typing.Self:
        start = time.time()
        result = fibonacci(index)
        end = time.time()
        return cls(end - start, result)


def mean(fn: typing.Callable[[], Bench], count: int):
    runtime_tot = 0.0
    for _ in range(count):
        bench = fn()
        if (
            math.isnan(bench.runtime)
            or math.isinf(bench.runtime)
            or bench.result is None
        ):
            return bench
        runtime_tot += bench.runtime
    return Bench(runtime_tot / count, bench.result)


def fancy(style: str, string: str) -> str:
    if sys.stdout.isatty():
        return f"{style}{string}\x1b[m"
    return string


def bench(
    hexcmds: typing.Iterable[str],
    *,
    timeout: float = None,
    bases: typing.Iterable[int],
    count: int,
    reference: typing.Optional[str],
):

    if reference is not None:
        hexcmds = filter(lambda cmd: not os.path.samefile(reference, cmd), hexcmds)

    hexcmds = sorted(hexcmds, key=lambda p: os.path.split(p)[::-1])
    refname = reference or "fibonappy"
    headlen = max((len(refname), *map(len, hexcmds)))
    timelen = 10

    for index in gen_indices(bases):
        print(fancy("\x1b[35m", f"# index: {index} ({index:b})"), flush=True)

        ok = [False] * len(hexcmds)
        print(fancy("\x1b[1;36m", f"{refname: >{headlen}}"), end=" ", flush=True)
        gold = (
            Bench.golden(index)
            if reference is None
            else Bench.collect(reference, index)
        )
        print(fancy("\x1b[1;36m", f"{gold.runtime: {timelen}.5f}"), flush=True)
        for i in range(len(hexcmds)):
            cmd = hexcmds[i]
            name = f"{cmd: >{headlen}}"
            print(name, end=" ", flush=True)

            bench = mean(
                lambda: Bench.collect(cmd, index, timeout=timeout), count=count
            )

            if math.isnan(bench.runtime):
                print(fancy("\x1b[1;33m", f"{'ERROR': >{timelen}}"), flush=True)
                continue
            if math.isinf(bench.runtime):
                print(fancy("\x1b[31m", f"{'TIMEOUT': >{timelen}}"), flush=True)
                continue
            if bench.result is None:
                print(fancy("\x1b[1;33m", f"{'INVALID': >{timelen}}"), flush=True)
                continue
            if bench.result != gold.result:
                print(
                    fancy("\x1b[1;37;41m", f"{'INCORRECT': >{timelen}}"),
                    flush=True,
                )
                continue

            ok[i] = True
            if gold.runtime > bench.runtime:
                speedup = gold.runtime / bench.runtime
            else:
                speedup = -bench.runtime / gold.runtime
            print(
                f"{bench.runtime: 10.5f}",
                fancy(
                    (
                        "\x1b[32m"
                        if speedup >= 1.5
                        else "\x1b[31m" if speedup <= -1.5 else "\x1b[2m"
                    ),
                    f"{speedup:+8.3f}x",
                ),
                flush=True,
            )

        hexcmds = [hexcmds[i] for i in range(len(hexcmds)) if ok[i]]
        if not hexcmds:
            break


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(
        description="Really bad benchmark script.",
    )

    parser.add_argument("cmds", metavar="HEXECUTABLE", nargs="*")
    parser.add_argument("-t", "--timeout", type=float, default=10.0)
    parser.add_argument("-b", "--base", type=int, action="append")
    parser.add_argument("-m", "--mean-of", type=int, default=3)
    parser.add_argument("-G", "--baseline", metavar="HEXECUTABLE")

    args = parser.parse_args()

    if args.cmds:
        hexcmds = args.cmds
    else:
        import glob

        hexcmds = glob.glob("./bin/*.hex.out")

    try:
        bench(
            hexcmds,
            timeout=args.timeout,
            bases=args.base or [3],
            count=args.mean_of,
            reference=args.baseline,
        )
    except KeyboardInterrupt:
        print("\n", fancy("\x1b[33m", "ABORTED"))
