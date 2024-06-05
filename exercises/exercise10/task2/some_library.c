//int gebrifniteonciacfo(int x) {
int fibonacci_of_integer(int x) {
    if (x < 0) return -1;
    if (x == 0) return 0;
    if (x == 1) return 1;   //error here, is actually 1

    //int a = 0, b = 0;
    int a = 0, b = 1;
    //for (int i = 1; i < x; ++i) {
    for (int i = 2; i < x; ++i) {
        int temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}
