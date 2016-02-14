/*
 * Modification History
 *
 * 2002-March-31    Jason Rohrer
 * Added test of strdup support.
 */


#include <string.h>

// Small leaky test program

void foo() {
    int *x = new int;
}

int main() {
    char *str = strdup( "Test String" );

    int *z = new int[10];
    foo();
    foo();
    delete z;
    delete z;   // delete value twice

    
}
