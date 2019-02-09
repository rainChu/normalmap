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
#include <string.h>

#ifdef _MSC_VER
#define INITIALIZE_STRUCT_FIELD(a, b) .a=b
#define STRCMPI _strcmpi
#else
#define INITIALIZE_STRUCT_FIELD(a, b) a:b
#define STRCMPI strcmpi
#endif

#define DEFAULT_SCALE 2.0

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
		"          none (the default)\n"
		"          sobel3x3    sobel5x5\n"
		"          prewitt3x3  prewitt5x5\n"
		"          3x3   5x5   7x7   9x9\n\n"

		"  -s    Set the scale of the generated map. Default is 2.0\n\n"

		// FIXME
		// Can we get someone who knows something about colorspace to provide more user-friendly
		// descriptions of these options?
		"  -h    Set the height input. Valid options are:\n"
		"          rgb      - Key RGB (The default)\n"
		"          r  g  b  - red, green, or blue single channel\n"
		"          biased, min, max  - Biased RGB, RGB Min, RGB Max\n"
		"          colorspace, normalize,     - Colorspace, Normalize Only\n"
		"          dudv, height - Du/Dv to normal, Heightmap"

		, argv[0]);
	return -1;
}

int main(int argc, char * const * argv) {

	// Get command line options
	const char *filtervalue = NULL, *heightsourcevalue = NULL;
	double scalevalue = DEFAULT_SCALE;
	opterr = 0;
	int c;

	while ((c = getopt(argc, argv, "f:s:h:")) != -1)
	{
		switch (c)
		{
		case 'f':
			filtervalue = optarg;
			break;

		case 's':
			scalevalue = atof(optarg);
			if (scalevalue <= 0.0) {
				fprintf( stderr, "Scale \"%s\" must be a positive real number. Defaulting to %f\n", optarg, DEFAULT_SCALE);
				scalevalue = DEFAULT_SCALE;
			}
			break;

		case 'h':
			heightsourcevalue = optarg;
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


    // TODO: expose more options via command line switches.
	NormalmapVals config = {
		INITIALIZE_STRUCT_FIELD(wrap, false),
		INITIALIZE_STRUCT_FIELD(scale, scalevalue),
		INITIALIZE_STRUCT_FIELD(dudv, DUDV_8BIT_UNSIGNED)
    };

	// Filter
	if (filtervalue == NULL || !STRCMPI(filtervalue, "none")) {
		config.filter = FILTER_NONE;
	}
	else if (!STRCMPI(filtervalue, "sobel3x3")) {
		config.filter = FILTER_SOBEL_3x3;
	}
	else if (!STRCMPI(filtervalue, "sobel5x5")) {
		config.filter = FILTER_SOBEL_5x5;
	}
	else if (!STRCMPI(filtervalue, "prewitt3x3")) {
		config.filter = FILTER_PREWITT_3x3;
	}
	else if (!STRCMPI(filtervalue, "prewitt5x5")) {
		config.filter = FILTER_PREWITT_5x5;
	}
	else if (!STRCMPI(filtervalue, "3x3")) {
		config.filter = FILTER_3x3;
	}
	else if (!STRCMPI(filtervalue, "5x5")) {
		config.filter = FILTER_5x5;
	}
	else if (!STRCMPI(filtervalue, "7x7")) {
		config.filter = FILTER_7x7;
	}
	else if (!STRCMPI(filtervalue, "9x9")) {
		config.filter = FILTER_9x9;
	}
	else {
		fprintf(stderr, "Warning: Unknown filter type \"%s\". defaulting to \"none\".", filtervalue);
	}

	// Height Source
	if (heightsourcevalue == NULL || !STRCMPI(heightsourcevalue, "rgb")) {
		config.filter = CONVERT_KEY_RGB;
	}
	else if (!STRCMPI(heightsourcevalue, "r")) {
		config.filter = CONVERT_RED;
	}
	else if (!STRCMPI(heightsourcevalue, "g")) {
		config.filter = CONVERT_GREEN;
	}
	else if (!STRCMPI(heightsourcevalue, "b")) {
		config.filter = CONVERT_BLUE;
	}
	else if (!STRCMPI(heightsourcevalue, "biased")) {
		config.filter = CONVERT_BIASED_RGB;
	}
	else if (!STRCMPI(heightsourcevalue, "min")) {
		config.filter = CONVERT_MIN_RGB;
	}
	else if (!STRCMPI(heightsourcevalue, "max")) {
		config.filter = CONVERT_MAX_RGB;
	}
	else if (!STRCMPI(heightsourcevalue, "colorspace")) {
		config.filter = CONVERT_COLORSPACE;
	}
	else if (!STRCMPI(heightsourcevalue, "normalize")) {
		config.filter = CONVERT_NORMALIZE_ONLY;
	}
	else if (!STRCMPI(heightsourcevalue, "dudv")) {
		config.filter = CONVERT_DUDV_TO_NORMAL;
	}
	else if (!STRCMPI(heightsourcevalue, "height")) {
		config.filter = CONVERT_HEIGHTMAP;
	}

    int32_t result;
    if ((result = normalmap(image_in, image_out, x, y, config)) != 0) {
        fprintf(stderr, "Failed building normal map, code %d", result);
    }

    const char *ext = get_filename_ext(outfile);
    if (STRCMPI("png", ext) == 0) {
        if (!stbi_write_png(outfile, x, y, 4, image_out, 0)) {
            fprintf(stderr, "Unable to write PNG %s", outfile);
        }
    } else if (STRCMPI("bmp", ext) == 0) {
        if (!stbi_write_bmp(outfile, x, y, 4, image_out)) {
            fprintf(stderr, "Unable to write BMP %s", outfile);
        }
    } else if (STRCMPI("tga", ext) == 0) {
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