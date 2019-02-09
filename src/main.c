#include <stdint.h>
#include <stdlib.h>
#include <stb_image.h>
#include <assert.h>

#ifdef _MSC_VER
#include <memory.h>
#include "getopt_msvs.h" /* getopt at: https://gist.github.com/ashelly/7776712 */
#else
#include <mem.h>
#include <getopt.h>
#endif

#include <stb_image_write.h>
#include <normalmap.h>
#include <stdbool.h>

#ifdef _MSC_VER
#define INITIALIZE_STRUCT_FIELD(a, b) .a=b
#else
#define INITIALIZE_STRUCT_FIELD(a, b) a:b
#endif

const char *get_filename_ext(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

int printusage(char * const *argv)
{
	fprintf(stderr, "Usage: %s [OPTIONS] <heightmap_filename> <bumpmap_filename>\n\n"
		"OPTIONS:\n"
		"  -f    Set filter type. Valid options are:\n"
		"          none\n"
		"          sobel3x3    sobel5x5\n"
		"          prewitt3x3  prewitt5x5\n"
		"          3x3   5x5   7x7   9x9\n"
		, argv[0]);
	return -1;
}

int main(int argc, char * const * argv) {

	// Get command line options
	const char *filtervalue = NULL;
	opterr = 0;
	int c;

	while ((c = getopt(argc, argv, "f:")) != -1)
	{
		switch (c)
		{
		case 'f':
			filtervalue = optarg;
			break;

		case '?':
			if (optopt == 'c') {
				printf("Option -%c requires an argument.\n", optopt);
			}
			exit(-1);
			break;

		default:
			abort();
		}
	}

    if (argc < (optind + 2)) {
		printf("%d %d\n", optind, argc);
		exit(printusage(argv));
    }

    const char *infile = argv[optind];
    const char *outfile = argv[optind + 1];

    int x, y, n;
    uint8_t *image_in = stbi_load(infile, &x, &y, &n, 4);
    if (!image_in) {
        fprintf(stderr, "File %s was not loaded: %s", infile, stbi_failure_reason());
        exit(-2);
    }
    uint8_t *image_out = malloc(x * y * 4);


    // TODO: expose more optiosn via command line switches.
    NormalmapVals config = {
		INITIALIZE_STRUCT_FIELD(filter, FILTER_NONE),
		INITIALIZE_STRUCT_FIELD(wrap, false),
		INITIALIZE_STRUCT_FIELD(conversion, CONVERT_RED),
		INITIALIZE_STRUCT_FIELD(scale, 2.0f),
		INITIALIZE_STRUCT_FIELD(dudv, DUDV_8BIT_UNSIGNED)
    };

	if (filtervalue == NULL || !strcmp(filtervalue, "none")) {
		config.filter = FILTER_NONE;
	}
	else if (!strcmp(filtervalue, "sobel3x3")) {
		config.filter = FILTER_SOBEL_3x3;
	}
	else if (!strcmp(filtervalue, "sobel5x5")) {
		config.filter = FILTER_SOBEL_5x5;
	}
	else if (!strcmp(filtervalue, "prewitt3x3")) {
		config.filter = FILTER_PREWITT_3x3;
	}
	else if (!strcmp(filtervalue, "prewitt5x5")) {
		config.filter = FILTER_PREWITT_5x5;
	}
	else if (!strcmp(filtervalue, "3x3")) {
		config.filter = FILTER_3x3;
	}
	else if (!strcmp(filtervalue, "5x5")) {
		config.filter = FILTER_5x5;
	}
	else if (!strcmp(filtervalue, "7x7")) {
		config.filter = FILTER_7x7;
	}
	else if (!strcmp(filtervalue, "9x9")) {
		config.filter = FILTER_9x9;
	}
	else {
		fprintf(stderr, "Warning: Unknown filter type \"%s\". defaulting to \"none\".", filtervalue);
	}

    int32_t result;
    if ((result = normalmap(image_in, image_out, x, y, config)) != 0) {
        fprintf(stderr, "Failed building normal map, code %d", result);
    }

    const char *ext = get_filename_ext(outfile);
    if (strcmpi("png", ext) == 0) {
        if (!stbi_write_png(outfile, x, y, 4, image_out, 0)) {
            fprintf(stderr, "Unable to write PNG %s", outfile);
        }
    } else if (strcmpi("bmp", ext) == 0) {
        if (!stbi_write_bmp(outfile, x, y, 4, image_out)) {
            fprintf(stderr, "Unable to write BMP %s", outfile);
        }
    } else if (strcmpi("tga", ext) == 0) {
        if (!stbi_write_tga(outfile, x, y, 4, image_out)) {
            fprintf(stderr, "Unable to write TGA %s", outfile);
        }
//    } else if (strcmpi("hdr", ext) == 0) {
//        if (!stbi_write_hdr(outfile, x, y, 4, image_out)) {
//            fprintf(stderr, "Unable to write HDR %s", outfile);
//        }
    } else {
        fprintf(stderr, "Unknown file format for save: %s", ext);
        exit(-1);
    }
    stbi_image_free(image_in);
    free(image_out);
}