/* convert u-boot dump from words to raw binary (little endian native Xscale, x86) */


#include <stdio.h>
#include <stdlib.h>


int convert(FILE* FIN, FILE* FOUT)
	
{
	char *line = 0;
	size_t maxline = 128;
	unsigned pa0;
	unsigned pa;
	unsigned short data_words[8];
	unsigned data_ex[8];
	int ll = 0;
	while(getline(&line, &maxline, FIN) >= 0){
		if (sscanf(line, "%x: %x %x %x %x %x %x %x %x",
					&pa, 
					data_ex+0, data_ex+1, data_ex+2, data_ex+3,
					data_ex+4, data_ex+5, data_ex+6, data_ex+7) == 9){
			if (ll++ != 0 && pa != pa0+0x10){
				fprintf(stderr, "ERROR at line % wanted %x got %x\n",
						ll, pa0+0x10, pa);
			}
			pa0 = pa;
			for (int ii = 0; ii < 8; ++ii){
				data_words[ii] = (unsigned short)data_ex[ii];
			}
			fwrite(data_words, sizeof(unsigned short), 8, FOUT);
		}
	}
	return 0;
}
int convert(const char* file_in, const char* file_out)
{
	FILE *FIN = fopen(file_in, "r");
	FILE *FOUT = fopen(file_out, "w");

	if (FIN == 0){
		perror(file_in);
		exit(1);
	} else if (FOUT == 0){
		perror(file_out);
		exit(1);
	} else {
		return convert(FIN, FOUT);
	}
	/* keep compiler happy */
	return 0;
}
int main(int argc, char* argv[])
{
	if (argc == 3){
		return convert(argv[1], argv[2]);
	}else{
		fprintf(stderr, "USAGE: parse-uboot-dump IN OUT\n");
		return 1;
	}
}
