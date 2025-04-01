#include <vector>
#include <cmath>
#include <algorithm>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

using namespace std;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
const TGAColor green = TGAColor(255, 0,   255,   0);

Model *model = nullptr;
const int width  = 800;
const int height = 800;

TGAImage diffuseTex(1024,1024,TGAImage::RGB);

//line algorithm
void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
    bool steep = false; //陡峭情况
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    if (dx < dy) {
        swap(x0, y0);
        swap(x1, y1);
        swap(dx, dy);
        steep = true;
    }
    //左右情况
    if (x0 > x1) {
        swap(x0, x1);
        swap(y0, y1);
    }
    int derror2 = abs(dy * 2); //derror2=derror * dx * 2
    int error2 = 0;
    int y = y0;
    for (int x = x0; x < x1; x++) {
        //循环结束，error2的总累计数应为error2 = derror2 * x1 => dy/dx * dx * x1 * 2     error*2*dx=0.5*2*dx
        if (error2 > dx) {
            y0 < y1 ? y++ : y--; //上下情况
            error2 -= 2 * dx;
        }
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
        error2 += derror2;
    }
}

void triangle2(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {
    //舍弃重复语句，代码更简洁
	if (t0.y > t1.y) std::swap(t0, t1);
	if (t0.y > t2.y) std::swap(t0, t2);
	if (t1.y > t2.y) std::swap(t1, t2);
	int total_height = t2.y - t0.y;
    for (int i = 0; i < total_height; i++)
    {
        bool second_half = (i > t1.y - t0.y) || t0.y == t1.y;
		int segment_height = second_half ? t2.y - t1.y : t1.y - t0.y;
        float alpha = (float)i / total_height;
        float beta = (float)(i - (second_half ? (t1.y - t0.y) : 0)) / segment_height;
        Vec2i A = t0 + (t2 - t0) * alpha;
        Vec2i B = second_half ? t1 + (t2 - t1) * beta : t0 + (t1 - t0) * beta;
        if (A.x > B.x) swap(A, B);
        for (int j = A.x; j <= B.x; j++) {
            image.set(j, i, color);
        }
    }
}

//find barycentric
Vec3f barycentric(Vec3f *pts, Vec2i P) {
    Vec3f u = Vec3f(pts[2].x-pts[0].x, pts[1].x-pts[0].x, pts[0].x-P.x)^Vec3f(pts[2].y-pts[0].y, pts[1].y-pts[0].y, pts[0].y-P.y);  //重心坐标计算,(ABx,ACx,PAx)*(ABy,ACy,PAy)
    if (std::abs(u.z)<1) return {Vec3f(-1,1,1)};
    return {Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z)};
}

//bounding_box algorithm
void triangle(Vec3f *pts, float *zbuffer, Vec2f *uvs, TGAImage &image, float intensity) {
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++) {
        bboxmin.x = max(0.f, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = max(0.f, std::min(bboxmin.y, pts[i].y));
        bboxmax.x = min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }
    Vec2i P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++) {
            Vec3f bc_screen = barycentric(pts, P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
            float z = pts[0].z * bc_screen.x + pts[1].z * bc_screen.y + pts[2].z * bc_screen.z;
            if (zbuffer[int(P.x + P.y * width)] < z) {
                zbuffer[int(P.x + P.y * width)] = z;
                float u = uvs[0].u * bc_screen.x + uvs[1].u * bc_screen.y + uvs[2].u * bc_screen.z;
                float v = uvs[0].v * bc_screen.x + uvs[1].v * bc_screen.y + uvs[2].v * bc_screen.z;
                TGAColor textColor = diffuseTex.get(u * 1024, v * 1024);
                textColor=TGAColor(textColor.bgra[2]*intensity,textColor.bgra[1]*intensity,textColor.bgra[0]*intensity,255);
                image.set(P.x, P.y, textColor);
            }
        }
    }
}

int main(int argc, char** argv) {
    model = new Model("obj/african_head.obj"); //读取模型

    diffuseTex.read_tga_file("tex/african_head_diffuse.tga"); //读取贴图

    TGAImage image(width, height, TGAImage::RGB);
    //线框渲染模型
    Vec3f light_dir(0, 0, -1);
    float *zbuffer = new float[width * height];
    for (int i=0;i<width;i++) {
        for (int j=0;j<height;j++) {
            zbuffer[i+j*width]=-numeric_limits<float>::max();
        }
    }
    for (int i = 0; i < model->nfaces(); i++) {
        vector<int> face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec3f((v.x + 1) * width / 2, (v.y + 1) * height / 2, v.z);
            world_coords[j] = v;
        }

        vector<int> textIndex = model->text(i);
        Vec2f uv[3];
        for (int j = 0; j < 3; j++) {
            uv[j] = model->uv(textIndex[j]);
        }

        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            triangle(screen_coords, zbuffer, uv, image,intensity);
        }
    }

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    delete zbuffer;
    return 0;
}





