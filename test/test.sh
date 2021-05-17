fail() {
    echo "FAIL"
    exit 1
}
check() {
    printf "$1: "
    vt100/vt100 -R "$1" 2>/dev/null | grep -a "$2" >/dev/null || fail
    echo "OK"
}
check test/tst8080.com 'CPU IS OPERATIONAL'
check test/cputest.com 'CPU TESTS OK'
check test/8080exer.com 'Tests complete'
echo All tests OK.
exit 0
