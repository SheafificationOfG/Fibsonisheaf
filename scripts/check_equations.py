def check(expr: str):
    if expr.startswith("#"):
        return True
    exprs = list(map(str.strip, expr.split("=")))
    for i, e in enumerate(exprs):
        try:
            val = eval(e)
            if i == 0:
                first = e
                target = val
            elif val != target:
                print(f'Expression "{first} = {e}" is false!')
                print(f"{first} = 0x{target:x}")
                print(f"{e} = 0x{val:x}")
                return False
        except Exception as exc:
            print(
                f"Failed to parse {e!r} in expression {expr!r} (received {type(exc).__qualname__}: {exc})"
            )
            return False
    return True


if __name__ == "__main__":
    import sys

    all_passed = True
    for line in sys.stdin:
        all_passed &= check(line)

    if all_passed:
        print("All checks passed!")
