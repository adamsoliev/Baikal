#!/bin/bash

assert() {
    expected="$(( ($1 % 256 + 256) % 256 ))"
    input="$2"

    ./build/ganymede "$input" > ./build/tmp.s || exit
    # ./build/ganymede "-s" "$input" -o ./build/tmp.s || exit

    # riscv64-linux-gnu-gcc -static -o ./build/tmp ./build/tmp.s
    riscv64-linux-gnu-gcc -static -o ./build/tmp ./build/tmp.s
    qemu-riscv64-static ./build/tmp

    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual" | paste -s -d ' '
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 3 "int main() { int a = 23; if (a > 10) { return 3; } return 0; }";
# assert 0 "int main() { return 0; }"
# assert 2 "int main() { return 2; }"
# assert -2 "int main() { return -2; }"
# assert 5 "int main() { return 2 + 3; }"
# assert 2 "int main() { return 4 - 2; }"
# assert 9 "int main() { return 3 * 3; }"
# assert 3 "int main() { return 9 / 3; }"

# assert 16 "int main() { return 2 + 3 + 3 + 3 + 3 + 2; }"
# assert 6 "int main() { return 20 - 3 - 3 - 3 - 3 - 2; }"
# assert 81 "int main() { return 3 * 3 * 3 * 3; }"
# assert 3 "int main() { return 81 / 3 / 3 / 3; }"

# assert 21 "int main() { return 4 + 3 * 3 * 3 - 10; }"
# assert 26 "int main() { return 4 + 3 * 3 * 3 - 10 / 2; }"

# assert 23 "int main() { int a = 23; return a; }"

# 2
# echo -e "\nTESTSUITE #2 - ensure generated .ll and .s files match expected files"
# python3 tests.py
# exit_code=$?
# if [[ $exit_code -ne 0 ]]; then
#     echo "Script failed"
#     exit 1
# fi


echo -e "\nOK"