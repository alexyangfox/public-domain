#include <stdio.h>

#define	MAX 20000

main() {
	int	i, j, p[MAX], k;

	for (k=0, i=3; k<MAX; i+=2) {
		for (j=0; j<k; j++)
			if (i % p[j] == 0)
				break;
		if (j < k) continue;
		p[k++] = i;
	}
	printf("%d\n", p[k-1]);
}
