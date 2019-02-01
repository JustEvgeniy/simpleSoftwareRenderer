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

void triangle(Vec3i t[], Vec2i uv[], float ity[], Model *model, TGAImage &image, int zBuffer[]) {
    if (t[0].y == t[1].y && t[0].y == t[2].y)
        return;

    if (t[0].y > t[1].y) {
        std::swap(t[0], t[1]);
        std::swap(uv[0], uv[1]);
        std::swap(ity[0], ity[1]);
    }
    if (t[0].y > t[2].y) {
        std::swap(t[0], t[2]);
        std::swap(uv[0], uv[2]);
        std::swap(ity[0], ity[2]);
    }
    if (t[1].y > t[2].y) {
        std::swap(t[1], t[2]);
        std::swap(uv[1], uv[2]);
        std::swap(ity[1], ity[2]);
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

        float ityA = ity[0] + (ity[2] - ity[0]) * alpha;
        float ityB = isSecondHalf ? ity[1] + (ity[2] - ity[1]) * beta : ity[0] + (ity[1] - ity[0]) * beta;

        if (A.x > B.x) {
            std::swap(A, B);
            std::swap(uvA, uvB);
            std::swap(ityA, ityB);
        }

        for (int x = A.x; x <= B.x; x++) {
            float phi = A.x == B.x ? 1.f : float(x - A.x) / (B.x - A.x);

            Vec3i P = Vec3f(A) + Vec3f(B - A) * phi;
            Vec2i uvP = uvA + (uvB - uvA) * phi;
            float ityP = ityA + (ityB - ityA) * phi;

            int idx = P.x + P.y * width;
            if (zBuffer[idx] < P.z) {
                zBuffer[idx] = P.z;
                TGAColor color = model->get_diffuse(uvP) * ityP;
//                TGAColor color = TGAColor(255, 255, 255) * ityP;
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

Matrix lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye - center).normalize();
    Vec3f x = (up ^ z).normalize();
    Vec3f y = (z ^ x).normalize();
    Matrix res = Matrix::identity();
    for (int i = 0; i < 3; ++i) {
        res[0][i] = x[i];
        res[1][i] = y[i];
        res[2][i] = z[i];
        res[i][3] = -center[i];
    }
    return res;
}

int main() {
    auto *model = new Model("../head.obj");

    auto *zBuffer = new int[width * height];
    for (int i = 0; i < width * height; ++i) {
        zBuffer[i] = std::numeric_limits<int>::min();
    }

    Vec3f lightDirection = Vec3f(1, 0, 3).normalize();
    Vec3f eyePosition(1, 0, 3);
    Vec3f center(0, 0, 0);

    //Image
    TGAImage image(width, height, TGAImage::RGB);

    Matrix modelView = lookat(eyePosition, center, Vec3f(0, 1, 0));
    Matrix projection = Matrix::identity();
    projection[3][2] = -1.f / (eyePosition - center).z;
    Matrix viewport = getViewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    Matrix transformMatrix = viewport * projection * modelView;

    std::cerr << modelView << std::endl;
    std::cerr << projection << std::endl;
    std::cerr << viewport << std::endl;
    std::cerr << transformMatrix << std::endl;

    for (int iFace = 0; iFace < model->nFaces(); ++iFace) {
        std::vector<int> face = model->get_face(iFace);
        Vec3f world_c[3];
        Vec3i screen_c[3];
        Vec2i uv[3];
        float intensity[3];

        for (int jVertex = 0; jVertex < 3; ++jVertex) {
            Vec3f vertex = model->get_vertex(face[jVertex]);
            screen_c[jVertex] = Vec3f(transformMatrix * Matrix(vertex));
            world_c[jVertex] = vertex;

            intensity[jVertex] = model->get_norm(iFace, jVertex) * lightDirection;
            uv[jVertex] = model->get_uv(iFace, jVertex);
        }

        triangle(screen_c, uv, intensity, model, image, zBuffer);
    }

    image.flip_vertically();
    image.write_tga_file("output.tga");

    //Z-Buffer image
    TGAImage zBufImage(width, height, TGAImage::GRAYSCALE);

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            zBufImage.set(i, j, TGAColor(static_cast<uint8_t>(zBuffer[i + j * width])));
        }
    }

    zBufImage.flip_vertically();
    zBufImage.write_tga_file("zBuffer.tga");

    delete model;
    delete[] zBuffer;

    return 0;
}
