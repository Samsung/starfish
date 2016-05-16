#include <string>
#include <Evas.h>
#include <Elementary.h>

#include <vector>

using std::string;

class Color {
public:
    Color(uint8_t a, uint8_t r, uint8_t g, uint8_t b)
        : m_a(a), m_r(r), m_g(g), m_b(b) {
    }

    bool operator ==(const Color& c)
    {
        if (m_a == c.a()
            && m_r == c.r()
            && m_g == c.g()
            && m_b == c.b()) {
            return true;
        } else {
            return false;
        }
    }

    bool similar(const Color& b)
    {
        int diff = 0;
        const int threshold = 3;
        int d;
        d = abs((int)a() - (int)b.a());
        diff += d;
        if (d > threshold) {
            return false;
        }

        d = abs((int)r() - (int)b.r());
        diff += d;
        if (d > threshold) {
            return false;
        }

        d = abs((int)g() - (int)b.g());
        diff += d;
        if (d > threshold) {
            return false;
        }

        d = abs((int)this->b() - (int)b.b());
        diff += d;
        if (d > threshold) {
            return false;
        }
        // return memcmp(this, &b, 4) == 0;
        return diff < (threshold * 4);
    }

    uint8_t a() const { return m_a; }
    uint8_t r() const { return m_r; }
    uint8_t g() const { return m_g; }
    uint8_t b() const { return m_b; }

private:
    uint8_t m_a, m_r, m_g, m_b;
};

class EvasImage {
public:
    EvasImage(Evas* win, string file)
    {
        m_image = evas_object_image_add(win);
        evas_object_image_file_set(m_image, file.c_str(), NULL);
        evas_object_image_size_get(m_image, &width, &height);
        evas_object_resize(m_image, width, height);
    }

    Color pixel(int x, int y)
    {
        Evas_Colorspace colorspace = evas_object_image_colorspace_get(m_image);
        uint32_t* src_data = (uint32_t*)evas_object_image_data_get(m_image, 0);
        int stride = evas_object_image_stride_get(m_image);

        if (colorspace == EVAS_COLORSPACE_ARGB8888) {
            uint32_t pixel = *(src_data + (y * stride/4 + x));
            uint8_t a = (pixel >> 24) & 0xFF;
            uint8_t r = (pixel >> 16) & 0xFF;
            uint8_t g = (pixel >> 8) & 0xFF;
            uint8_t b = pixel & 0xFF;
            return Color(a, r, g, b);
        }

        return Color(0,0,0,0);
    }

    Evas_Object* m_image;
    int width;
    int height;
};

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s file1.png file2.png\n", argv[0]);
        return 0;
    }

    elm_init(argc, argv);

    string file1 = string(argv[1]);
    string file2 = string(argv[2]);

    Evas_Object* win = elm_win_add(NULL, "main", ELM_WIN_BASIC);
    Evas* e = evas_object_evas_get(win);

    EvasImage img1 = EvasImage(e, file1);
    EvasImage img2 = EvasImage(e, file2);

    if (!((img1.width == img2.width) && (img1.height == img2.height))) {
        printf("Fail\n");
        return 0;
    }
    evas_object_resize(win, img1.width, img1.height);

    int diffCount = 0;
    int width = img1.width;
    int height = img1.height;

    std::vector<std::vector<bool>> checkVector;
    checkVector.resize(height);

    for (int y=0; y < img1.height; y++) {
        checkVector[y].resize(width, 0);
        for (int x=0; x < img1.width; x++) {
            Color c1 = img1.pixel(x, y);
            Color c2 = img2.pixel(x, y);

            if (!(c1.similar(c2))) {
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

        if (maxCnt < 10) {
            printf("diff: %01.2f%% passed (not exactly same)(debug %d)\n", 100.f*(diffCount/((float)(width*height))), maxCnt);
        } else {
            printf("diff: %01.2f%% failed\n", 100.f*(diffCount/((float)(width*height))));
        }

    } else {
        printf("Pass\n");
    }

    return 0;
}
