import typing
from dataclasses import dataclass


@dataclass
class PrimeFactor:
    factor: int
    multiplicity: int


def extract_prime_factors(n: int) -> typing.Iterator[PrimeFactor]:
    fac = 2
    while fac * fac <= n:
        mult = 0
        while n % fac == 0:
            mult += 1
            n //= fac
        if mult:
            yield PrimeFactor(fac, mult)
        fac += 1
    if n > 1:
        yield PrimeFactor(n, 1)


def is_mod_generator(candidate: int, mod: int, factors: typing.Iterable[PrimeFactor]):
    for pf in factors:
        exp = (mod - 1) // pf.factor
        if pow(candidate, exp, mod) == 1:
            return False
    return True


def get_mod_generator(prime: int):
    factors = list(extract_prime_factors(prime - 1))
    for g in range(1, prime):
        if is_mod_generator(g, prime, factors):
            return g

    raise RuntimeError(f"Could not find generator for units modulo {prime}!")


if __name__ == "__main__":
    import sys

    prime = int(sys.argv[1])
    factors = list(extract_prime_factors(prime - 1))
    print(
        "# Prime factors:",
        " * ".join(f"{pf.factor} ** {pf.multiplicity}" for pf in factors),
    )
    for g in range(1, prime):
        if is_mod_generator(g, prime, factors):
            print(g)
