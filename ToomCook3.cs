//
//  60 million per sec, C#, 200 lines, out of the box libraries...
//
//  This algorithm uses the doubling algorithm with ToomCook-3 multiplication, precomputation, caching, threading and recursion
//

#define THREAD

using System.Numerics;

namespace Fibonacci
{
    partial class Program
    {
        const int TOOMCOOK_CUTOFF = 1 << 19;

        static BigInteger[] small = [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946, 17711, 28657, 46368, 75025, 121393, 196418, 317811, 514229, 832040, 1346269, 2178309, 3524578, 5702887, 9227465, 14930352, 24157817, 39088169, 63245986, 102334155, 165580141, 267914296, 433494437, 701408733, 1134903170, 1836311903, 2971215073, 4807526976, 7778742049, 12586269025, 20365011074, 32951280099, 53316291173, 86267571272, 139583862445, 225851433717, 365435296162, 591286729879, 956722026041, 1548008755920, 2504730781961, 4052739537881, 6557470319842, 10610209857723, 17167680177565, 27777890035288, 44945570212853, 72723460248141, 117669030460994, 190392490709135, 308061521170129, 498454011879264, 806515533049393, 1304969544928657, 2111485077978050, 3416454622906707, 5527939700884757, 8944394323791464, 14472334024676221, 23416728348467685, 37889062373143906, 61305790721611591, 99194853094755497, 160500643816367088, 259695496911122585, 420196140727489673, 679891637638612258, 1100087778366101931, 1779979416004714189, 2880067194370816120, 4660046610375530309, 7540113804746346429, 12200160415121876738];

        static Dictionary<ulong, BigInteger> cache = new();

        static BigInteger Fibonacci(ulong n)
        {
            if (n < (ulong)small.Length)
            {
                return small[n];
            }
            else
            {
                if (cache.ContainsKey(n)) return cache[n];

                ulong k = n / 2;
                BigInteger Fk = Fibonacci(k), Fkp1 = Fibonacci(k + 1), result;

                if (n % 2 == 0)
                {
                    result = ToomCook3(Fk, (Fkp1 << 1) - Fk);
                }
                else
                {
                    result = ToomCook3(Fkp1) + ToomCook3(Fk);
                }

                cache[n] = result;

                return result;
            }
        }

        static BigInteger[] Extract(int n, BigInteger mask, BigInteger _p)
        {
            var m = new BigInteger[3];

            if (_p > 0)
            {
                for (int i = 0; i < m.Length; i++)
                {
                    m[i] = _p & mask;
                    _p >>= n;
                }
            }
            else
            {
                for (int i = 0; i < m.Length; i++)
                {
                    m[i] = -1 * (_p & mask);
                    _p >>= n;
                }
            }

            var p = new BigInteger[5];
            var p0 = m[0] + m[2];

            p[0] = m[0];
            p[1] = p0 + m[1];
            p[2] = p0 - m[1];
            p[3] = (p[2] + m[2] << 1) - m[0];
            p[4] = m[2];

            return p;
        }

        static BigInteger ToomCook3(BigInteger _p, BigInteger _q)
        {
            int n = (int)Math.Max(_p.GetBitLength(), _q.GetBitLength());

            if (n <= TOOMCOOK_CUTOFF) return _p * _q;

            n = (n + 2) / 3;

            BigInteger _base = 1; _base <<= n;

            BigInteger[] p = Extract(n, _base - 1, _p), q = Extract(n, _base - 1, _q);
            BigInteger[] __r = new BigInteger[5];

#if THREAD
            Task[] task =
            [
                Task.Run(() => { __r[0] = ToomCook3(p[0], q[0]); }),
                Task.Run(() => { __r[1] = ToomCook3(p[1], q[1]); }),
                Task.Run(() => { __r[2] = ToomCook3(p[2], q[2]); }),
                Task.Run(() => { __r[3] = ToomCook3(p[3], q[3]); }),
                Task.Run(() => { __r[4] = ToomCook3(p[4], q[4]); }),
            ];
            Task.WaitAll(task);
#else
            __r[0] = ToomCook3(p[0], q[0]);
            __r[1] = ToomCook3(p[1], q[1]);
            __r[2] = ToomCook3(p[2], q[2]);
            __r[3] = ToomCook3(p[3], q[3]);
            __r[4] = ToomCook3(p[4], q[4]);
#endif

            BigInteger[] _r = new BigInteger[5];
            _r[0] = __r[0];
            _r[4] = __r[4];
            _r[3] = (__r[3] - __r[1]) / 3;
            _r[1] = (__r[1] - __r[2]) >> 1;
            _r[2] = __r[2] - __r[0];
            _r[3] = ((_r[2] - _r[3]) >> 1) + (__r[4] << 1);
            _r[2] = _r[2] + _r[1] - _r[4];
            _r[1] = _r[1] - _r[3];

            BigInteger r = 0;
            for (int i = _r.Length - 1; i >= 0; i--)
            {
                r <<= n;
                r += _r[i];
            }
            return r;
        }
        static BigInteger ToomCook3(BigInteger _p)
        {
            int n = (int)_p.GetBitLength();

            if (n <= TOOMCOOK_CUTOFF) return _p * _p;

            n = (n + 2) / 3;

            BigInteger _base = 1; _base <<= n;

            BigInteger[] p = Extract(n, _base - 1, _p);
            BigInteger[] __r = new BigInteger[5];

#if THREAD
            Task[] task =
            [
                Task.Run(() => { __r[0] = ToomCook3(p[0]); }),
                Task.Run(() => { __r[1] = ToomCook3(p[1]); }),
                Task.Run(() => { __r[2] = ToomCook3(p[2]); }),
                Task.Run(() => { __r[3] = ToomCook3(p[3]); }),
                Task.Run(() => { __r[4] = ToomCook3(p[4]); }),
            ];
            Task.WaitAll(task);
#else
            __r[0] = ToomCook3(p[0]);
            __r[1] = ToomCook3(p[1]);
            __r[2] = ToomCook3(p[2]);
            __r[3] = ToomCook3(p[3]);
            __r[4] = ToomCook3(p[4]);
#endif

            BigInteger[] _r = new BigInteger[5];
            _r[0] = __r[0];
            _r[4] = __r[4];
            _r[3] = (__r[3] - __r[1]) / 3;
            _r[1] = (__r[1] - __r[2]) >> 1;
            _r[2] = __r[2] - __r[0];
            _r[3] = ((_r[2] - _r[3]) >> 1) + (__r[4] << 1);
            _r[2] = _r[2] + _r[1] - _r[4];
            _r[1] = _r[1] - _r[3];

            BigInteger r = 0;
            for (int i = _r.Length - 1; i >= 0; i--)
            {
                r <<= n;
                r += _r[i];
            }
            return r;
        }
    }
}
