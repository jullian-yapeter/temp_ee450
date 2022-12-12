all: serverM.c serverC.c serverEE.c serverCS.c client.c
	gcc serverM.c -o serverM
	gcc serverC.c -o serverC
	gcc serverEE.c -o serverEE
	gcc serverCS.c -o serverCS
	gcc client.c -o client
