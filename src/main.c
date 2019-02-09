#include <stdint.h>
#include <stdlib.h>
#include <stb_image.h>
#include <assert.h>

#ifdef _MSC_VER
#include <memory.h>
#else
#include <mem.h>
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


int main(int argc, const char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <heightmap_filename> <bumpmap_filename>", argv[0]);
        exit(-1);
    }

    const char *infile = argv[1];
    const char *outfile = argv[2];
    int x, y, n;
    uint8_t *image_in = stbi_load(infile, &x, &y, &n, 4);
    if (!image_in) {
        fprintf(stderr, "File %s was not loaded: %s", infile, stbi_failure_reason());
        exit(-2);
    }
    uint8_t *image_out = malloc(x * y * 4);

    // TODO: expose this via command line switches.
    NormalmapVals config = {
		INITIALIZE_STRUCT_FIELD(filter, FILTER_NONE),
		INITIALIZE_STRUCT_FIELD(wrap, false),
		INITIALIZE_STRUCT_FIELD(conversion, CONVERT_RED),
		INITIALIZE_STRUCT_FIELD(scale, 2.0f),
		INITIALIZE_STRUCT_FIELD(dudv, DUDV_8BIT_UNSIGNED)
    };

	//config.filter = FILTER_NONE;

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