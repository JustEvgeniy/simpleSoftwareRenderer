#include <limits>
#include "TGAImage.h"
#include "Model.h"

const TGAColor white = TGAColor(255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0);
const TGAColor green = TGAColor(0, 255, 0);
const TGAColor blue = TGAColor(0, 0, 255);

static const int width = 850;
static const int height = 850;
static const int depth = 255;

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

void triangle(Vec3i t[], Vec2i uv[], Model *model, TGAImage &image, float intensity, int zBuffer[]) {
    if (t[0].y == t[1].y && t[0].y == t[2].y)
        return;

    if (t[0].y > t[1].y) {
        std::swap(t[0], t[1]);
        std::swap(uv[0], uv[1]);
    }
    if (t[0].y > t[2].y) {
        std::swap(t[0], t[2]);
        std::swap(uv[0], uv[2]);
    }
    if (t[1].y > t[2].y) {
        std::swap(t[1], t[2]);
        std::swap(uv[1], uv[2]);
    }

    int total_height = t[2].y - t[0].y;
    for (int i = 0; i < total_height; i++) {
        bool isSecondHalf = i > t[1].y - t[0].y || t[1].y == t[0].y;
        int segment_height = isSecondHalf ? t[2].y - t[1].y : t[1].y - t[0].y;

        float alpha = float(i) / total_height;
        float beta = float(i - (isSecondHalf ? t[1].y - t[0].y : 0)) / segment_height;

        Vec3i A = t[0] + Vec3f(t[2] - t[0]) * alpha;
        Vec3i B = isSecondHalf ? t[1] + Vec3f(t[2] - t[1]) * beta : t[0] + Vec3f(t[1] - t[0]) * beta;

        Vec2i uvA = uv[0] + (uv[2] - uv[0]) * alpha;
        Vec2i uvB = isSecondHalf ? uv[1] + (uv[2] - uv[1]) * beta : uv[0] + (uv[1] - uv[0]) * beta;

        if (A.x > B.x) {
            std::swap(A, B);
            std::swap(uvA, uvB);
        }

        for (int x = A.x; x <= B.x; x++) {
            float phi = A.x == B.x ? 1.f : float(x - A.x) / (B.x - A.x);

            Vec3i P = Vec3f(A) + Vec3f(B - A) * phi;
            Vec2i uvP = uvA + (uvB - uvA) * phi;

            int idx = P.x + P.y * width;
            if (zBuffer[idx] < P.z) {
                zBuffer[idx] = P.z;
                TGAColor color = model->get_diffuse(uvP) * intensity;
                image.set(P.x, P.y, color);
            }
        }
    }
}

Matrix getViewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;

    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}

int main() {
    auto *model = new Model("../head.obj");

    Vec3f lightDirection(0, 0, -1);
    lightDirection.normalize();

    auto *zBuffer = new int[width * height];
    for (int i = 0; i < width * height; ++i) {
        zBuffer[i] = std::numeric_limits<int>::min();
    }

    Vec3f cameraPosition(0, 0, 3);

    //Image
    TGAImage image(width, height, TGAImage::RGB);

    Matrix projection = Matrix::identity(4);
    projection[3][2] = -1.f / cameraPosition.z;
    Matrix viewport = getViewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    Matrix transformMatrix = viewport * projection;

    for (int iFace = 0; iFace < model->nFaces(); ++iFace) {
        std::vector<int> face = model->get_face(iFace);
        Vec3f world_c[3];
        Vec3i screen_c[3];

        for (int j = 0; j < 3; ++j) {
            Vec3f vertex = model->get_vertex(face[j]);
            screen_c[j] = Vec3f(transformMatrix * Matrix(vertex));
            world_c[j] = vertex;
        }

        Vec3f n = (world_c[2] - world_c[0]) ^(world_c[1] - world_c[0]);
        n.normalize();

        float light_intensity = n * lightDirection;

        if (light_intensity > 0) {
            Vec2i uv[3];
            for (int iVertex = 0; iVertex < 3; ++iVertex) {
                uv[iVertex] = model->get_uv(iFace, iVertex);
            }

            triangle(screen_c, uv, model, image, light_intensity, zBuffer);
        }
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");

    //Z-Buffer image
    TGAImage zBufImage(width, height, TGAImage::GRAYSCALE);

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            zBufImage.set(i, j, TGAColor(zBuffer[i + j * width], 1));
        }
    }

    zBufImage.flip_vertically();
    zBufImage.write_tga_file("zBuffer.tga");

    delete model;
    delete[] zBuffer;

    return 0;
}
