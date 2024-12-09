# Fibsonisheaf

Sheafification of [Fibsonicci](https://github.com/SheafificationOfG/Fibsonicci), a response to the responses.

The goal of all of these algorithms is to (hopefully efficiently) compute $`F_n`$, defined by

```math
F_n = \begin{cases}
    F_{n-1} + F_{n-2}, & n \geq2 \\
    1, & n = 1 \\
    0, & n = 0
\end{cases}
```

## Usage

After cloning the repo, make sure to run

```bash
make init
```

to initialise the output folders.
This step is necessary before trying to build anything else.

### Computing one Fibonacci number

To compute a particular Fibonacci number (using one of the [supported implementations](#algorithms)), use

```bash
# Generate binary
make bin/$(algo).hex.out

# Usage:
./bin/$(algo).hex.out $(fibonacci_index) $(output_file)
# If output_file is not provided, output is directed to stdout
```

Due to laziness, I have not implemented any intelligent conversion from hexadecimal to decimal.
If you don't jive with hex, you can convert the hex output with `scripts/hex2dec.py`.

```bash
./bin/$(algo).hex.out $(fibonacci_index) | python3 scripts/hex2dec.py $(output_file)
# If output_file is not provided, output is directed to stdout½F½F
```

By default, `hex2dec` prints out at most 32 significant digits.
To print *all* digits, pass `-n0` or `--ndigits=0` as an argument.

### Plotting performance

> [!WARNING]
> There is nothing scientific about how these algorithms are benchmarked.
> I promise no consistency between what you get, and what I present in videos.

To construct data for plotting, run

```bash
# To generate data for a specific algorithm
make data/$(algo).dat
# Or, to generate all data
make all-data
```

To plot the data, a prerequisite is a configuration JSON file, having the following form.

```json
{
    "Name of algorithm (for legend)": {
        "path": "data/$(algo).dat",
        "colour": "$(some_matplotlib_colour)",
        // there are also some optional params
    },
    // ...
}
```

> [!NOTE]
> Paths to `*.dat` files are relative to the config JSON you define.

> [!TIP]
> You can also plot against data files generated from the [OG Fibsonicci project](https://github.com/SheafificationOfG/Fibsonicci).
> The `anim` script is able to parse either format.

You should then be able to run

```bash
python3 scripts/anim.py path/to/your.json
```

If you provide `--output=foobar`, then the plot animation will be saved to `foobar.mp4`, and the final plot to `foobar.png`.

> [!IMPORTANT]
> The `anim` script requires `matplotlib`, and `ffmpeg` (for saving `.mp4`s).

# Algorithms

The project includes the following implementations.

| Algorithm | Source (`impl/`) | Runtime |
|:---------:|:----------------:|:-------:|
| [Naive](#naive) | `naive.c` | $`\Omega(\exp(n))`$ |
| ["Linear"](#linear) | `linear.c` | $`O(n^2)`$ |

## Naive

Always need to have the nonrecursive "by definition" implementation present, for completeness:

```py
def naive(n):
    if n <= 1:
        return n
    return fib(n-1) + fib(n-2)
```

> [!WARNING]
> I feel required to tell you that the algorithm will only correctly compute $`F_n`$ for $`n\leq93`$.
> Fibonacci terms that do not fit in 64 bits are expressly not supported.

## "Linear"

The bottom-up iterative implementation, which involves $`O(n)`$ iterations to compute $`F_n`$.

```py
def linear(n):
    a, b = 1, 0
    for _ in range(n):
        a, b = a+b, a
    return b
```

> [!TIP]
> Although the algorithm is called "linear" (referring to the number of iterations), the algorithm is $\Theta(n^2)$ overall.

