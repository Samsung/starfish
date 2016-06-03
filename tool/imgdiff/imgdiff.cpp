#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include <vector>

#define PNG_DEBUG 3
#include <png.h>

struct Color {
    unsigned char r, g, b, a;
    bool hasAlpha;
    inline bool operator ==(const Color& b)
    {
        int diff = 0;
        const int threshold = 64;
        int d;
        if (hasAlpha && b.hasAlpha) {
            d = abs((int)a - (int)b.a);
            diff += d;
            if (d > threshold) {
                return false;
            }
        }

        d = abs((int)r - (int)b.r);
        diff += d;
        if (d > threshold) {
            return false;
        }

        d = abs((int)g - (int)b.g);
        diff += d;
        if (d > threshold) {
            return false;
        }

        d = abs((int)this->b - (int)b.b);
        diff += d;
        if (d > threshold) {
            return false;
        }
        // return memcmp(this, &b, 4) == 0;
        return diff < (threshold);
    }

    inline bool operator !=(const Color& b)
    {
        return !this->operator ==(b);
    }
};

class Image {
public:
    virtual bool open(const char* fileName) = 0;
    virtual ~Image()
    {

    }
    virtual int width() = 0;
    virtual int height() = 0;
    virtual Color colorAt(int x, int y) = 0;
protected:

};

class ImagePng : public Image {
public:
    virtual bool open(const char* fileName)
    {
        unsigned char header[8];    // 8 is the maximum size that can be checked

        /* open file and test for it being a png */
        FILE *fp = fopen(fileName, "rb");
        if (!fp)
            abort("[read_png_file] File %s could not be opened for reading", fileName);
        fread(header, 1, 8, fp);
        if (png_sig_cmp(header, 0, 8))
            abort("[read_png_file] File %s is not recognized as a PNG file", fileName);

        /* initialize stuff */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

        if (!png_ptr)
            abort("[read_png_file] png_create_read_struct failed");

        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr)
            abort("[read_png_file] png_create_info_struct failed");

        if (setjmp(png_jmpbuf(png_ptr)))
            abort("[read_png_file] Error during init_io");

        png_init_io(png_ptr, fp);
        png_set_sig_bytes(png_ptr, 8);

        png_read_info(png_ptr, info_ptr);

        m_width = png_get_image_width(png_ptr, info_ptr);
        m_height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        number_of_passes = png_set_interlace_handling(png_ptr);
        png_read_update_info(png_ptr, info_ptr);

        if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGB) {
            num_palette = 3;
//            printf("RGB:%ld\n", png_get_rowbytes(png_ptr, info_ptr));
//            printf("%d %d - %d\n", m_width, m_height, num_palette );
        } else if (png_get_color_type(png_ptr, info_ptr) == PNG_COLOR_TYPE_RGBA) {
            num_palette = 4;
//            printf("RGBA:%ld\n", png_get_rowbytes(png_ptr, info_ptr));
//            printf("%d %d - %d\n", m_width, m_height, num_palette );
        } else {
            abort("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) or PNG_COLOR_TYPE_RGB (%d) (is %d)", PNG_COLOR_TYPE_RGBA, PNG_COLOR_TYPE_RGB, png_get_color_type(png_ptr, info_ptr));
        }

        /*
        if (png_get_color_type(png_ptr, info_ptr) != PNG_COLOR_TYPE_RGBA)
            abort("[process_file] color_type of input file must be PNG_COLOR_TYPE_RGBA (%d) (is %d)", PNG_COLOR_TYPE_RGBA, png_get_color_type(png_ptr, info_ptr));
        */

        /* read file */
        if (setjmp(png_jmpbuf(png_ptr)))
            abort("[read_png_file] Error during read_image");

        row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * m_height);
        for (int y = 0; y < m_height; y++)
            row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr, info_ptr));

        png_read_image(png_ptr, row_pointers);

        fclose(fp);
        return true;
    }

    void abort(const char * s, ...)
    {
        va_list args;
        va_start(args, s);
        vfprintf(stderr, s, args);
        fprintf(stderr, "\n");
        va_end(args);
        ::abort();
    }

    virtual int width()
    {
        return m_width;
    }

    virtual int height()
    {
        return m_height;
    }
    virtual Color colorAt(int x, int y)
    {
        png_byte* row = row_pointers[y];
        png_byte* ptr = &(row[x*num_palette]);
        // printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
        // x, y, ptr[0], ptr[1], ptr[2], ptr[3]);
        Color c;
        c.r = ptr[0];
        c.g = ptr[1];
        c.b = ptr[2];
        if (num_palette >= 4) {
            c.a = ptr[3];
            c.hasAlpha = true;
        } else {
            c.hasAlpha = false;
        }
        return c;
    }

    int m_width, m_height;
    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;
    int num_palette;
    png_bytep * row_pointers;
};



int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc != 3) {
        puts("oops! need 2 png file");
        return -1;
    }

    Image* i1 = new ImagePng();
    Image* i2 = new ImagePng();

    i1->open(argv[1]);
    i2->open(argv[2]);

    if ((i1->width() != i2->width()) || (i1->height() != i2->height())) {
        puts("diff: 100.0% failed (image size diffrent)");
        return 0;
    }

    int height = i1->height();
    int width = i1->width();
    int diffCount = 0;

    // 0 same
    // > 0 not same

    std::vector<std::vector<bool>> checkVector;
    checkVector.resize(height);

    for (int y = 0; y < height; y ++) {
        checkVector[y].resize(width, 0);
        for (int x = 0; x < width; x ++) {
            Color a = i1->colorAt(x, y);
            Color b = i2->colorAt(x, y);
            if (a != b) {
                diffCount++;
                checkVector[y][x] = 1;
            }
        }
    }

    if (diffCount) {
        int maxCnt = 1;
        for (int y = 0; y < height; y ++) {
            for (int x = 0; x < width; x ++) {

                if (checkVector[y][x]) {
                    int cnt = 1;
                    // n
                    if ((y > 0) && (checkVector[y-1][x])) {
                        cnt++;
                    }
                    // e
                    if ((x < (width-1)) && (checkVector[y][x+1])) {
                        cnt++;
                    }

                    // w
                    if ((x > 0) && (checkVector[y][x-1])) {
                        cnt++;
                    }

                    // s
                    if ((y < (height-1)) && (checkVector[y+1][x])) {
                        cnt++;
                    }

                    maxCnt = std::max(maxCnt, cnt);
                }

            }
        }

        if (maxCnt < 4) {
            printf("diff: %01.2f%% passed (not exactly same)(debug %d)\n", 100.f*(diffCount/((float)(width*height))), maxCnt);
        } else {
            printf("diff: %01.2f%% failed\n", 100.f*(diffCount/((float)(width*height))));
        }


    } else {
        printf("diff: 0.00%% passed\n");
    }


    return 0;
}
