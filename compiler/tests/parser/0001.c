int a = 23;
int b = 23 + 32 + 23 - 43 - 32;
int c = 82 + 23 * 32 - 600 / 20;
int d = 54 << 2;
int e = 24 >> 2;
int f = 94 < 2;
int g = 74 > 2;
int h = 13 <= 2;
int i = 94 >= 2;
int j = 51 == 2;
int k = 18 != 2;
int l = 43 & 2;
int m = 93 ^ 2;
int n = 34 | 2;
int o = 32 && 92;
int p = 32 || 12;
int v = sizeof a;

int sum0() { return 0; }

int sum1(int a) { return a; }

int sum2(int a, int b) { return a + b; }

int sum10(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
        return a + b + c + d + e + f + g + h + i;
}

int a[10];
float b[10][10];

int main() {
        int a = b == 23 ? 32 : 43;
        a = b != 23 ? 32 : 43;
        int aa = ++b;
        --c;
        +d;
        -e;
        !f;
        ~g;
        g++;
        g--;
        g.b;
        g->a;
        g[2];
        g();
        g(a);
        g(a, b);
        g(a, b, c, d, e, f);
        if (a == 23) {
                a = 32;
        }
        if (a == 43) b = 23;

        if (c == 23) {
                g = 32;
        } else {
                g = 0;
        }
        if (a == 43)
                b = 23;
        else
                b = 32;

        if (f == 103) {
                j = 423;
        } else if (k == 32) {
                m = 392;
        } else {
                n = 0;
        }

        for (i = 0; i < 10; i++) {
                a += b;
                if (a == 23) {
                        c += d;
                }
        }
        for (int i = 0; i < 10; i++) {
                a += b;
                int c = 23 * b;
        }

        while (a > 10) {
                j >> 2;
                a--;
        }
        while (aaa < 10) bbb--;
        while (a > 10) {
                for (int i = 0; i < 10; i++) {
                        a += b;
                        if (a == 23) {
                                c += d;
                        }
                        int c = 23 * b;
                }
                j >> 2;
                a--;
        }

        do {
                a++;
                if (a == 43)
                        b = 23;
                else
                        b = 32;
        } while (a < 10);

        do js--;
        while (a < 10);

        switch (a) {
                case 23: {
                        a = 32;
                        break;
                }
                case 43: {
                        b = 23;
                        break;
                }
                default: {
                        c = 0;
                        break;
                }
        }

        for (int i = 0; i < 10; i++) {
                if (i == 3) continue;
                if (i == 5) break;
                if (i == 7) goto end_loop;
        }

end_loop:
        return 23;

        return 0;
}
