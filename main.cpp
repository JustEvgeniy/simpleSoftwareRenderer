#include <limits>
#include "TGAImage.h"
#include "Model.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
const TGAColor blue = TGAColor(0, 0, 255, 255);

static const int width = 800;
static const int height = 800;

void line(int x0, int y0, int x1, int y1, TGAImage &image, const TGAColor &color) {
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror = std::abs(dy) * 2;
    int error = 0;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        error += derror;

        if (error > dx) {
            y += (y1 > y0 ? 1 : -1);
            error -= dx * 2;
        }
    }
}

void line(const Vec2i &vec1, const Vec2i &vec2, TGAImage &image, const TGAColor &color) {
    line(vec1.x, vec1.y, vec2.x, vec2.y, image, color);
}

void triangle(Vec3i t0, Vec3i t1, Vec3i t2, TGAImage &image, const TGAColor &color, int zBuffer[]) {
    if (t0.y == t1.y && t0.y == t2.y)
        return;
    if (t0.y > t1.y)
        std::swap(t0, t1);
    if (t0.y > t2.y)
        std::swap(t0, t2);
    if (t1.y > t2.y)
        std::swap(t1, t2);
    int total_height = t2.y - t0.y;
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > t1.y - t0.y || t1.y == t0.y;
        int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;

        float alpha = float(i) / total_height;
        float beta = float(i - (second_half ? t1.y - t0.y : 0)) / segment_height;
        Vec3i A = t0 + Vec3i(t2 - t0) * alpha;
        Vec3i B = second_half ? t1 + Vec3i(t2 - t1) * beta : t0 + Vec3i(t1 - t0) * beta;

        if (A.x > B.x)
            std::swap(A, B);

        for (int j = A.x; j <= B.x; j++) {
            float phi = B.x == A.x ? 1.f : float(j - A.x) / (B.x - A.x);
            Vec3i P = Vec3i(A) + Vec3i(B - A) * phi;
            P.x = j;
            P.y = t0.y + i;
            int idx = P.x + P.y * width;

            if (zBuffer[idx] < P.z) {
                zBuffer[idx] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}

int main(int argc, char **argv) {
    TGAImage image(width, height, TGAImage::RGB);

//    auto *model = new Model("head.obj");
//
//    for (int k = 0; k < runCount; ++k) {
//        for (int i = 0; i < model->nFaces(); ++i) {
//            std::vector<int> face = model->get_face(i);
//            for (int j = 0; j < 3; ++j) {
//                Vec3f v0 = model->get_vertex(get_face[j]);
//                Vec3f v1 = model->get_vertex(get_face[(j + 1) % 3]);
//                int x0 = (v0.x + 1) * width / 2;
//                int y0 = (v0.y + 1) * height / 2;
//                int x1 = (v1.x + 1) * width / 2;
//                int y1 = (v1.y + 1) * height / 2;
//                line(x0, y0, x1, y1, image, white);
//            }
//        }
//    }

    auto *model = new Model("head.obj");

    Vec3f light_dir = Vec3f(0, 0, -1);
    light_dir.normalize();

    auto *zBuffer = new int[width * height];
    for (int i = 0; i < width * height; ++i) {
        zBuffer[i] = std::numeric_limits<int>::min();
    }

    for (int i = 0; i < model->nFaces(); ++i) {
        std::vector<int> face = model->get_face(i);
        Vec3f world_c[3];
        Vec3i screen_c[3];

        for (int j = 0; j < 3; ++j) {
            Vec3f vertex = model->get_vertex(face[j]);
            screen_c[j] = Vec3i((vertex.x + 1) * width / 2.f, (vertex.y + 1) * height / 2.f, 0);
            world_c[j] = vertex;
        }

        Vec3f n = (world_c[2] - world_c[0]) ^(world_c[1] - world_c[0]);
        n.normalize();

        float light_intensity = n * light_dir;

        TGAColor color(255 * light_intensity, 255 * light_intensity, 255 * light_intensity, 255);
        if (light_intensity > 0)
            triangle(screen_c[0], screen_c[1], screen_c[2], image, color, zBuffer);
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");

    return 0;
}
