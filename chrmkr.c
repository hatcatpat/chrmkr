/* TODO:
 * - commandline options?
 * 		- palette options (chr -> *)
 * */

#include <png.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed short int int16_t;
typedef unsigned short int uint16_t;
typedef signed int int32_t;
typedef unsigned int uint32_t;
typedef signed long int int64_t;
typedef unsigned long int uint64_t;
typedef unsigned int uint_t;

typedef uint8_t bool;

enum filetype
{
    FILETYPE_CHR,
    FILETYPE_TGA,
    FILETYPE_PNG,
    FILETYPE_BMP,
    NUM_FILETYPES,
    FILETYPE_UNKNOWN
};
char *filetypes[NUM_FILETYPES] = { "chr", "tga", "png", "bmp" };
enum filetype parse_filetype (const char *file);

uint8_t palette[4][3] = {
    { 0, 0, 0 },
    { 0xff, 0, 0 },
    { 0, 0xff, 0 },
    { 0, 0, 0xff },
};

uint8_t chr[4096];
uint_t chr_index (int x, int y);
uint8_t chr_get (int x, int y);
void chr_set (int x, int y, uint8_t v);
void chr_read (FILE *f);
void chr_write (FILE *f);

struct tga
{
    uint8_t id_len;
    uint8_t cm_type;
    uint8_t img_type;

    uint16_t cm_first;
    uint16_t cm_len;
    uint8_t cm_bits_per_entry;

    uint16_t x_origin;
    uint16_t y_origin;
    uint16_t width;
    uint16_t height;
    uint8_t bits_per_pixel;
    uint8_t img_desc;
};

struct bmp_header
{
    uint8_t signature[2];
    uint32_t file_size;
    uint32_t reserved;
    uint32_t offset;
};
#define BMP_HEADER_SIZE 14

struct bmp_dib
{
    uint32_t dib_size;
    uint16_t width;
    uint16_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
};
#define BMP_DIB_CORE_SIZE 12

int chr2tga (const char *chr_in, const char *tga_out);
int tga2chr (const char *tga_in, const char *chr_out);

int chr2png (const char *chr_in, const char *png_out);
int png2chr (const char *png_in, const char *chr_out);

int chr2bmp (const char *chr_in, const char *bmp_out);
int bmp2chr (const char *bmp_in, const char *chr_out);

uint_t
chr_index (int x, int y)
{
    x %= 128, y %= 128;
    return (x / 8) * 16 + (y / 8) * 16 * 16 + (y % 8);
}

uint8_t
chr_get (int x, int y)
{
    uint_t i = chr_index (x, y);
    x = 7 - (x % 8);
    return ((chr[i] >> x) & 1) + ((chr[i + 8] >> x) & 1) * 2;
}

void
chr_set (int x, int y, uint8_t v)
{
    uint_t i = chr_index (x, y);
    x = (1 << (7 - (x % 8)));
    switch (v)
        {
        case 0:
            chr[i] &= ~x;
            chr[i + 8] &= ~x;
            break;
        case 1:
            chr[i] |= x;
            chr[i + 8] &= ~x;
            break;
        case 2:
            chr[i] &= ~x;
            chr[i + 8] |= x;
            break;
        case 3:
            chr[i] |= x;
            chr[i + 8] |= x;
            break;
        }
}

void
chr_read (FILE *f)
{
    fread (chr, 1, 4096, f);
}

void
chr_write (FILE *f)
{
    fwrite (chr, 1, 4096, f);
}

int
chr2tga (const char *chr_in, const char *tga_out)
{
    FILE *in, *out;
    struct tga tga;
    int i;

    in = fopen (chr_in, "rb");
    if (in == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", chr_in);
            return 1;
        }
    chr_read (in);
    fclose (in);

    out = fopen (tga_out, "wb");
    if (out == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", tga_out);
            return 1;
        }

    tga.id_len = 0;
    tga.cm_type = 1;
    tga.img_type = 1;

    tga.cm_first = 0;
    tga.cm_len = 4;
    tga.cm_bits_per_entry = 24;

    tga.x_origin = 0;
    tga.y_origin = 0;
    tga.width = 128;
    tga.height = 128;
    tga.bits_per_pixel = 8;
    tga.img_desc = 0;

    fwrite (&tga.id_len, 1, 1, out);
    fwrite (&tga.cm_type, 1, 1, out);
    fwrite (&tga.img_type, 1, 1, out);

    fwrite (&tga.cm_first, 2, 1, out);
    fwrite (&tga.cm_len, 2, 1, out);
    fwrite (&tga.cm_bits_per_entry, 1, 1, out);

    fwrite (&tga.x_origin, 2, 1, out);
    fwrite (&tga.y_origin, 2, 1, out);
    fwrite (&tga.width, 2, 1, out);
    fwrite (&tga.height, 2, 1, out);
    fwrite (&tga.bits_per_pixel, 1, 1, out);
    fwrite (&tga.img_desc, 1, 1, out);

    for (i = 0; i < 4 * 3; ++i)
        fputc (palette[i / 3][2 - i % 3], out);

    for (i = 0; i < 128 * 128; ++i)
        fputc (chr_get (i % 128, 127 - i / 128), out);

    fclose (out);
    return 0;
}

int
tga2chr (const char *tga_in, const char *chr_out)
{
    FILE *in, *out;
    struct tga tga;
    int i;

    in = fopen (tga_in, "rb");
    if (in == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", tga_in);
            return 1;
        }

    out = fopen (chr_out, "wb");
    if (out == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", chr_out);
            fclose (in);
            return 1;
        }

    fread (&tga.id_len, 1, 1, in);
    fread (&tga.cm_type, 1, 1, in);
    fread (&tga.img_type, 1, 1, in);

    fread (&tga.cm_first, 2, 1, in);
    fread (&tga.cm_len, 2, 1, in);
    fread (&tga.cm_bits_per_entry, 1, 1, in);

    fread (&tga.x_origin, 2, 1, in);
    fread (&tga.y_origin, 2, 1, in);
    fread (&tga.width, 2, 1, in);
    fread (&tga.height, 2, 1, in);
    fread (&tga.bits_per_pixel, 1, 1, in);
    fread (&tga.img_desc, 1, 1, in);

#ifdef DEBUG
    printf ("%s tga info:\n", tga_in);
    printf ("id_len %i\n", tga.id_len);
    printf ("cm_type %i\n", tga.cm_type);
    printf ("img_type %i\n", tga.img_type);

    printf ("cm_first %i\n", tga.cm_first);
    printf ("cm_len %i\n", tga.cm_len);
    printf ("cm_bits_per_entry %i\n", tga.cm_bits_per_entry);

    printf ("x_origin %i\n", tga.x_origin);
    printf ("y_origin %i\n", tga.y_origin);
    printf ("width %i\n", tga.width);
    printf ("height %i\n", tga.height);
    printf ("bits_per_pixel %i\n", tga.bits_per_pixel);
    printf ("img_desc %i\n", tga.img_desc);
#endif

    if (tga.id_len != 0)
        fseek (in, tga.id_len, SEEK_CUR);

    for (i = 0; i < MIN (tga.cm_len, 4) * tga.cm_bits_per_entry / 8; ++i)
        fgetc (in);

    if (tga.cm_len > 4)
        fseek (in, (tga.cm_len - 4) * tga.cm_bits_per_entry / 8, SEEK_CUR);

    for (i = 0; i < tga.width * tga.height * tga.bits_per_pixel / 8; ++i)
        chr_set (i % tga.width, tga.height - (i / tga.width) - 1, fgetc (in));

    chr_write (out);

    fclose (in), fclose (out);
    return 0;
}

int
chr2png (const char *chr_in, const char *png_out)
{
    static png_byte row[128 / 4];
    FILE *in, *out;
    int i, j;
    png_infop info;
    png_structp png;
    png_color png_palette[4] = {
        { palette[0][0], palette[0][1], palette[0][2] },
        { palette[1][0], palette[1][1], palette[1][2] },
        { palette[2][0], palette[2][1], palette[2][2] },
        { palette[3][0], palette[3][1], palette[3][2] },
    };

    in = fopen (chr_in, "rb");
    if (in == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", chr_in);
            return 1;
        }
    chr_read (in);
    fclose (in);

    out = fopen (png_out, "wb");
    if (out == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", png_out);
            return 1;
        }

    png = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL)
        {
            fclose (out);
            return 1;
        }

    info = png_create_info_struct (png);
    if (info == NULL)
        {
            png_destroy_write_struct (&png, NULL);
            fclose (out);
            return 1;
        }

    if (setjmp (png_jmpbuf (png)))
        {
            png_destroy_write_struct (&png, &info);
            fclose (out);
            return 1;
        }

    png_init_io (png, out);
    png_set_IHDR (png, info, 128, 128, 2, PNG_COLOR_TYPE_PALETTE,
                  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
                  PNG_FILTER_TYPE_DEFAULT);
    png_set_PLTE (png, info, png_palette, 4);
    png_write_info (png, info);

    for (i = 0; i < 128; ++i)
        {
            for (j = 0; j < 128 / 4; ++j)
                {
                    row[j] = 0;
                    row[j] |= chr_get (j * 4, i) << 6;
                    row[j] |= chr_get (j * 4 + 1, i) << 4;
                    row[j] |= chr_get (j * 4 + 2, i) << 2;
                    row[j] |= chr_get (j * 4 + 3, i);
                }
            png_write_row (png, row);
        }

    png_write_end (png, info);
    png_destroy_write_struct (&png, &info);

    fclose (out);
    return 0;
}

int
png2chr (const char *png_in, const char *chr_out)
{
    FILE *in, *out;
    int i, j, width, height;
    png_byte color_type, bit_depth;
    png_infop info;
    png_structp png;
    png_bytep *rows;

    in = fopen (png_in, "rb");
    if (in == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", png_in);
            return 1;
        }

    out = fopen (chr_out, "wb");
    if (out == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", chr_out);
            fclose (in);
            return 1;
        }

    png = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png == NULL)
        {
            fclose (in), fclose (out);
            return 1;
        }

    info = png_create_info_struct (png);
    if (info == NULL)
        {
            png_destroy_read_struct (&png, NULL, NULL);
            fclose (in), fclose (out);
            return 1;
        }

    if (setjmp (png_jmpbuf (png)))
        {
            png_destroy_read_struct (&png, &info, NULL);
            fclose (in), fclose (out);
            return 1;
        }

    png_init_io (png, in);
    png_read_info (png, info);

    width = png_get_image_width (png, info);
    height = png_get_image_height (png, info);
    color_type = png_get_color_type (png, info);
    bit_depth = png_get_bit_depth (png, info);

#ifdef DEBUG
    printf ("%s png info:\n", png_in);
    printf ("width: %i\n", width);
    printf ("height: %i\n", height);
    printf ("color_type: %i\n", color_type);
    printf ("bit_depth: %i\n", bit_depth);
#endif

    if (bit_depth < 8)
        png_set_packing (png);

    if (color_type & PNG_COLOR_MASK_ALPHA)
        {
            png_set_strip_alpha (png);
            color_type &= ~PNG_COLOR_MASK_ALPHA;
        }

    if (color_type == PNG_COLOR_TYPE_RGB
        || color_type == PNG_COLOR_TYPE_PALETTE
        || color_type == PNG_COLOR_TYPE_GRAY)
        {
            if (color_type == PNG_COLOR_TYPE_RGB)
                {
                    png_set_rgb_to_gray (png, 1, -1, -1);
                    color_type = PNG_COLOR_TYPE_GRAY;
                }

            png_read_update_info (png, info);

            rows = malloc (sizeof (png_bytep) * height);
            for (i = 0; i < height; i++)
                rows[i] = malloc (png_get_rowbytes (png, info));

            png_read_image (png, rows);

            for (i = 0; i < MIN (128, width); ++i)
                for (j = 0; j < MIN (128, height); ++j)
                    if (color_type == PNG_COLOR_TYPE_PALETTE)
                        chr_set (i, j, rows[j][i]);
                    else
                        chr_set (i, j, rows[j][i] / (256 / 4));

            for (i = 0; i < height; i++)
                free (rows[i]);
            free (rows);
        }
    else
        {
            fprintf (stderr, "[error] unsupported color_type %i\n",
                     color_type);
            png_destroy_read_struct (&png, &info, NULL);
            fclose (in), fclose (out);
            return 1;
        }

    png_destroy_read_struct (&png, &info, NULL);

    chr_write (out);

    fclose (in), fclose (out);
    return 0;
}

int
chr2bmp (const char *chr_in, const char *bmp_out)
{
    FILE *in, *out;
    struct bmp_header header;
    struct bmp_dib dib;
    int i;

    in = fopen (chr_in, "rb");
    if (in == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", chr_in);
            return 1;
        }
    chr_read (in);
    fclose (in);

    out = fopen (bmp_out, "wb");
    if (out == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", bmp_out);
            return 1;
        }

    header.signature[0] = 'B';
    header.signature[1] = 'M';
    header.file_size = BMP_HEADER_SIZE + BMP_DIB_CORE_SIZE + 4 * 3 + 64 * 128;
    header.reserved = 0;
    header.offset = BMP_HEADER_SIZE + BMP_DIB_CORE_SIZE + 4 * 3;

    fwrite (&header.signature, 1, 2, out);
    fwrite (&header.file_size, 4, 1, out);
    fwrite (&header.reserved, 4, 1, out);
    fwrite (&header.offset, 4, 1, out);

    dib.dib_size = BMP_DIB_CORE_SIZE;
    dib.width = 128;
    dib.height = 128;
    dib.planes = 1;
    dib.bits_per_pixel = 4;

    fwrite (&dib.dib_size, 4, 1, out);
    fwrite (&dib.width, 2, 1, out);
    fwrite (&dib.height, 2, 1, out);
    fwrite (&dib.planes, 2, 1, out);
    fwrite (&dib.bits_per_pixel, 2, 1, out);

    for (i = 0; i < 4 * 3; ++i)
        fputc (palette[i / 3][2 - i % 3], out);

    for (i = 0; i < 64 * 128; ++i)
        {
            uint8_t left = chr_get ((i % 128) * 2, 127 - i / 64) << 4;
            uint8_t right = chr_get ((i % 128) * 2 + 1, 127 - i / 64);
            fputc (left | right, out);
        }

    fclose (out);
    return 0;
}

int
bmp2chr (const char *bmp_in, const char *chr_out)
{
    FILE *in, *out;
    struct bmp_header header;
    struct bmp_dib dib;
    int i, c = 0;

    in = fopen (bmp_in, "rb");
    if (in == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", bmp_in);
            return 1;
        }

    out = fopen (chr_out, "wb");
    if (out == NULL)
        {
            fprintf (stderr, "[error] unable to open %s\n", chr_out);
            fclose (in);
            return 1;
        }

    fread (&header.signature[0], 1, 1, in);
    fread (&header.signature[1], 1, 1, in);
    fread (&header.file_size, 4, 1, in);
    fread (&header.reserved, 4, 1, in);
    fread (&header.offset, 4, 1, in);

#ifdef DEBUG
    printf ("%s bmp info:\n", bmp_in);
    printf ("header.signature %c%c\n", header.signature[0],
            header.signature[1]);
    printf ("header.file_size %i\n", header.file_size);
    printf ("header.reserved %i\n", header.reserved);
    printf ("header.offset %i\n", header.offset);
#endif

    fread (&dib.dib_size, 4, 1, in);

    if (dib.dib_size == 12)
        {
            fread (&dib.width, 2, 1, in);
            fread (&dib.height, 2, 1, in);
        }
    else
        {
            fread (&dib.width, 4, 1, in);
            fread (&dib.height, 4, 1, in);
        }

    fread (&dib.planes, 2, 1, in);
    fread (&dib.bits_per_pixel, 2, 1, in);

#ifdef DEBUG
    printf ("dib.size %i\n", dib.dib_size);
    printf ("dib.width %i\n", dib.width);
    printf ("dib.height %i\n", dib.height);
    printf ("dib.planes %i\n", dib.planes);
    printf ("dib.bits_per_pixel %i\n", dib.bits_per_pixel);
#endif

    fseek (in, header.offset, SEEK_SET);

    switch (dib.bits_per_pixel)
        {
        case 1:
            for (i = 0; i < dib.width * dib.height; ++i)
                {
                    if (i % 8 == 0)
                        c = fgetc (in);
                    chr_set (i % dib.width, dib.height - (i / dib.width) - 1,
                             (c >> (i % 8)) & 1);
                }
            break;

        case 4:
            for (i = 0; i < dib.width * dib.height; ++i)
                {
                    if (i % 2 == 0)
                        c = fgetc (in);
                    chr_set (i % dib.width, dib.height - (i / dib.width) - 1,
                             (c >> (4 * ((i + 1) % 2))) & 15);
                }
            break;

        case 8:
            for (i = 0; i < dib.width * dib.height; ++i)
                chr_set (i % dib.width, dib.height - (i / dib.width) - 1,
                         fgetc (in));
            break;

        case 16:
        case 24:
        case 32:
            for (i = 0; i < dib.width * dib.height; ++i)
                {
                    chr_set (i % dib.width, dib.height - (i / dib.width) - 1,
                             fgetc (in));
                    fseek (in, (dib.bits_per_pixel / 8) - 1, SEEK_CUR);
                }
            break;

        default:
            fprintf (stderr, "[error] unsupported bits_per_pixel %i\n",
                     dib.bits_per_pixel);
            fclose (in), fclose (out);
            return 1;
        }

    chr_write (out);

    fclose (in), fclose (out);
    return 0;
}

enum filetype
parse_filetype (const char *file)
{
    int i;
    for (i = 0; i < NUM_FILETYPES; ++i)
        if (strstr (file, filetypes[i]) != NULL)
            return i;
    return FILETYPE_UNKNOWN;
}

void
usage ()
{
    printf ("usage:\n"
            "input.chr output.*: converts input chr to output\n"
            "input.tga output.chr: converts input tga to output chr\n"
            "input.png output.chr: converts input png to output chr\n"
            "input.bmp output.chr: converts input bmp to output chr\n");
};

int
main (int argc, char *argv[])
{
    if (argc <= 2)
        usage ();
    else if (argc == 3)
        {
            enum filetype input = parse_filetype (argv[1]),
                          output = parse_filetype (argv[2]);

            if (input == FILETYPE_CHR && output == FILETYPE_TGA)
                chr2tga (argv[1], argv[2]);
            else if (input == FILETYPE_TGA && output == FILETYPE_CHR)
                tga2chr (argv[1], argv[2]);
            else if (input == FILETYPE_CHR && output == FILETYPE_PNG)
                chr2png (argv[1], argv[2]);
            else if (input == FILETYPE_PNG && output == FILETYPE_CHR)
                png2chr (argv[1], argv[2]);
            else if (input == FILETYPE_CHR && output == FILETYPE_BMP)
                chr2bmp (argv[1], argv[2]);
            else if (input == FILETYPE_BMP && output == FILETYPE_CHR)
                bmp2chr (argv[1], argv[2]);
            else
                {
                    fprintf (stderr, "[error] unable to convert %s to %s\n",
                             argv[1], argv[2]);
                    usage ();
                    return 1;
                }
        }

    return 0;
}
