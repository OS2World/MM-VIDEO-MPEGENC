			----------------
			| FILE FORMATS |
			----------------

PPM FORMAT
----------

To give you an idea of what a PPM file is, the following code will write
one out:

    int **red, **green, **blue;

    void	WritePPM(char *fileName, int width, int height, int maxVal)
    {
	register int x, y;
	unsigned char   r, g, b;

	fprintf(stdout, "P6\n");
	fprintf(stdout, "%d %d\n", width, height);
	fprintf(stdout, "%d\n", maxVal);

	for ( y = 0; y < height; y++ )
	    for ( x = 0; x < width; x++ )
	    {
		r = red[x][y];  g = green[x][y];    b = blue[x][y];

		fwrite(&r, 1, 1, stdout);
		fwrite(&g, 1, 1, stdout);
		fwrite(&b, 1, 1, stdout);
	    }
    }

maxVal is the maximum color value.  It must be between 0 and 255 inclusive.
Generally speaking, it should be 255 always.


YUV FORMAT
----------

You should be aware that the YUV format used in the MPEG encoder is DIFFERENT
than the Abekas YUV format.  The reason for this is that in MPEG, the U and
V components are subsampled 4:1.

To give you an idea of what format the YUV file must be in, the following
code will read in a YUV file:

    unsigned char **y_data, **cr_data, **cb_data;

    void	ReadYUV(char *fileName, int width, int height)
    {
	FILE *fpointer;
	register int y;

	/* should allocate memory for y_data, cr_data, cb_data here */

	fpointer = fopen(fileName, "r");

	for (y = 0; y < height; y++)			/* Y */
	    fread(y_data[y], 1, width, fpointer);

	for (y = 0; y < height / 2; y++)			/* U */
	    fread(cb_data[y], 1, width / 2, fpointer);

	for (y = 0; y < height / 2; y++)			/* V */
	    fread(cr_data[y], 1, width / 2, fpointer);

	fclose(fpointer);
    }

There are two reasons why you'd want to use YUV files rather than PPM files:
	1)  The YUV files are 50% the size of the corresponding PPM files
	2)  The ENCODER will run slightly faster, since it doesn't have to
	    do the RGB to YUV conversion itself.
