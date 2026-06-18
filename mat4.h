#ifndef MAT4_H
#define MAT4_H

#include "vec3.h"
#include <vector>
#include <cmath>

struct Mat4 {
    float m[4][4] = {{0}};

    Mat4() {
        m[0][0] = 1; m[1][1] = 1; m[2][2] = 1; m[3][3] = 1;
    }

    static Mat4 identity() {
        return Mat4();
    }

    static Mat4 createTranslation(const Vec3& v) {
        Mat4 res;
        res.m[3][0] = v.x;
        res.m[3][1] = v.y;
        res.m[3][2] = v.z;
        return res;
    }

    static Mat4 createRotation(float angle, const Vec3& axis) {
        Mat4 res;
        Vec3 normAxis = axis;
        normAxis.normalize();
        float c = cos(angle);
        float s = sin(angle);
        float t = 1.0f - c;
        float x = normAxis.x;
        float y = normAxis.y;
        float z = normAxis.z;

        res.m[0][0] = t*x*x + c;
        res.m[0][1] = t*x*y - s*z;
        res.m[0][2] = t*x*z + s*y;
        res.m[1][0] = t*x*y + s*z;
        res.m[1][1] = t*y*y + c;
        res.m[1][2] = t*y*z - s*x;
        res.m[2][0] = t*x*z - s*y;
        res.m[2][1] = t*y*z + s*x;
        res.m[2][2] = t*z*z + c;

        return res;
    }

    static Mat4 createPerspective(float fov, float aspect, float near, float far) {
        Mat4 res = {};
        float tanHalfFov = tan(fov / 2.0f);
        res.m[0][0] = 1.0f / (aspect * tanHalfFov);
        res.m[1][1] = 1.0f / (tanHalfFov);
        res.m[2][2] = -(far + near) / (far - near);
        res.m[2][3] = -1.0f;
        res.m[3][2] = -(2.0f * far * near) / (far - near);
        res.m[3][3] = 0.0f;
        return res;
    }

    static Mat4 createLookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = center - eye;
        f.normalize();
        Vec3 s = cross(f, up);
        s.normalize();
        Vec3 u = cross(s, f);

        Mat4 res;
        res.m[0][0] = s.x;
        res.m[1][0] = s.y;
        res.m[2][0] = s.z;
        res.m[0][1] = u.x;
        res.m[1][1] = u.y;
        res.m[2][1] = u.z;
        res.m[0][2] = -f.x;
        res.m[1][2] = -f.y;
        res.m[2][2] = -f.z;
        res.m[3][0] = -dot(s, eye);
        res.m[3][1] = -dot(u, eye);
        res.m[3][2] = dot(f, eye);
        return res;
    }

    Mat4 operator*(const Mat4& other) const {
        Mat4 result = {};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i][j] = m[i][0] * other.m[0][j] +
                                 m[i][1] * other.m[1][j] +
                                 m[i][2] * other.m[2][j] +
                                 m[i][3] * other.m[3][j];
            }
        }
        return result;
    }

     const float* constData() const {
        return &m[0][0];
    }
};

#endif // MAT4_H
