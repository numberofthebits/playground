#include "math.h"

#include <math.h>
#include <stdint.h>
#include <memory.h>


float dot_vec2f(Vec2f* a, Vec2f* b) {
    return a->x * b->x + a->y * b->y;
}

float dot_vec3f(Vec3f* a, Vec3f* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

Vec3f cross(Vec3f* a, Vec3f* b) {
    Vec3f result;
    result.x = a->y * b->z - a->z * b->y;
    result.y = a->z * b->x - a->x * b->z;
    result.z = a->x * b->y - a->y * b->x;

    return result;
}

Vec3f scale(Vec3f* v, float scalar) {
    Vec3f result;
    result.x = v->x * scalar;
    result.y = v->y * scalar;
    result.z = v->z * scalar;
    return result;
}

float magnitude_squared_vec2f(Vec2f* v) {
    return v->x * v->x + v->y * v->y;
}

float magnitude_squared_vec3f(Vec3f* v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

float length_vec2f(Vec2f* v) {
    return (float)sqrt(magnitude_squared_vec2f(v));
}

float length_vec3f(Vec3f* v) {
    return (float)sqrt(magnitude_squared_vec3f(v));
}

Vec2f normalize_vec2f(Vec2f* v) {
    float len = length_vec2f(v);
    Vec2f result;
    result.x = v->x / len;
    result.y = v->y / len;

    return result;
}

Vec2f normalize_with_len_vec2f(Vec2f* v, float len) {
    Vec2f result;
    result.x = v->x / len;
    result.y = v->y / len;

    return result;
}

Vec3f normalize_vec3f(Vec3f* v) {
    float len = length_vec3f(v);
    Vec3f result;
    result.x = v->x / len;
    result.y = v->y / len;
    result.z = v->z / len;

    return result;
}

Vec3f normalize_with_len_vec3f(Vec3f* v, float len) {
    Vec3f result;
    result.x = v->x / len;
    result.y = v->y / len;
    result.z = v->z / len;

    return result;
}


// Subtract "b" from "a"
Vec3f sub(Vec3f* a, Vec3f* b) {
    Vec3f result;
    
    result.x = a->x - b->x;
    result.y = a->y - b->y;
    result.z = a->z - b->z;
    
    return result;
}

Mat4x4 look_at(Vec3f* pos, Vec3f* target, Vec3f* up) {
    Vec3f dir = sub(pos, target);
    Vec3f d = normalize_vec3f(&dir);
    Vec3f u = normalize_vec3f(up);
    Vec3f r = cross(&u, &d);
    Vec3f t = {
        dot_vec3f(pos, &r),
        dot_vec3f(pos, &u),
        dot_vec3f(pos, &d)
    };
        
    Mat4x4 m = {
        r.x,  u.x,  d.x, 0.0,
        r.y,  u.y,  d.y, 0.0,
        r.z,  u.z,  d.z, 0.0,
        -t.x, -t.y, -t.z, 1.0
    };
    return m;
}
    
Mat4x4 zero() {
    Mat4x4 m;
    memset(&m, 0x0, sizeof(Mat4x4));
    return m;
}

Mat4x4 identity() {
    Mat4x4 m;

    m.data[0] = 1.0f;
    m.data[1] = 0.0f;
    m.data[2] = 0.0f;
    m.data[3] = 0.0f;

    m.data[4] = 0.0f;
    m.data[5] = 1.0f;
    m.data[6] = 0.0f;
    m.data[7] = 0.0f;

    m.data[8] = 0.0f;
    m.data[9] = 0.0f;
    m.data[10] = 1.0f;
    m.data[11] = 0.0f;

    m.data[12] = 0.0f;
    m.data[13] = 0.0f;
    m.data[14] = 0.0f;
    m.data[15] = 1.0f;

    return m;
}

Mat4x4 perspective(float near, float far, float fov, float aspect) {
    float scale = 1.0f / (aspect *  (float)tan(fov * 0.5f * PI / 180.0f));
    float f_min_n = far - near;

    Mat4x4 m = identity();
       
    m.data[0] = scale;
    m.data[5] = scale;
    m.data[10] = -((far + near) / f_min_n);
    m.data[11] = -1.0f;
    m.data[14] = -(2.0f * far * near) / (far - near);

    return m;
}

Mat4x4 ortho(float near, float far, float right, float left, float top, float bottom) {
    Mat4x4 m = identity();
    // TODO: Transpose assignments instead of calling transpose runtime
    m.data[0] = 2.f / (right - left);
    m.data[5] = 2.f / (top - bottom);
    m.data[10] = -2.f / (far - near);
    m.data[12] = -(right + left) / (right - left);
    m.data[13] = -(top + bottom) / (top - bottom);
    m.data[14] = -(far + near)  / (far - near);
    m.data[15] = 1.f;


    transpose(&m);
    return m;
}

void translate(Mat4x4* mat, Vec3f* v) {
    mat->data[12] += v->x;
    mat->data[13] += v->y;
    mat->data[14] += v->z;
}

void scale_mat4(Mat4x4* mat, Vec3f* v) {
    mat->data[0] *= v->x;
    mat->data[5] *= v->y;
    mat->data[10] *= v->z;
}

void mat4_rotate(Mat4x4* mat, Vec3f* axis, float radians) {
    float cos_r = cosf(radians);
    float sin_r = sinf(radians);
    float one_min_cos_r = 1.0f - cos_r;
    float x2 = axis->x * axis->x;
    float y2 = axis->y * axis->y;
    float z2 = axis->z * axis->z;
    float xy = axis->x * axis->y;
    float xz = axis->x * axis->z;
    float yz = axis->y * axis->z;

    mat->data[0] = x2 * one_min_cos_r + cos_r;
    mat->data[1] = xy * one_min_cos_r + axis->z * sin_r;
    mat->data[2] = xz * one_min_cos_r - axis->y * sin_r;

    mat->data[4] = xy * one_min_cos_r - axis->z * sin_r;
    mat->data[5] = y2 * one_min_cos_r + cos_r;
    mat->data[6] = yz * one_min_cos_r + axis->x * sin_r;

    mat->data[8] = xz * one_min_cos_r + axis->y * sin_r;
    mat->data[9] = yz * one_min_cos_r - axis->x * sin_r;
    mat->data[10] = z2 * one_min_cos_r + cos_r;
}

void transpose(Mat4x4* mat) {
    float values[16] = {
        mat->data[0], mat->data[4], mat->data[8], mat->data[12],
        mat->data[1], mat->data[5], mat->data[9], mat->data[13],
        mat->data[2], mat->data[6], mat->data[10],mat->data[14],
        mat->data[3], mat->data[7], mat->data[11], mat->data[15]
    };

    for (int i = 0; i < 16; ++i) {
        mat->data[i] = values[i];
    }
}
    
Mat4x4 mul(Mat4x4* a, Mat4x4* b) {

    Mat4x4 m = zero();
    int index = 0;
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j <4; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a->data[i*4 + k] * b->data[k*4 + j];        
            }
            m.data[index] = sum;
            index += 1;
        }
    }

    return m;
}

int intersect_rectf(Rectf* a, Rectf* b) {
    if (a->pos.x  + a->width  < b->pos.x ||
        a->pos.x >= b->pos.x  + b->width ||
        a->pos.y  + a->height < b->pos.y ||
        a->pos.y >= b->pos.y  + b->height)
    {
        return 0;
    }

    return 1;
}

// #[derive(Default)]
//  struct SphericalCoord {
//      radius: float,
//      theta: float,
//      phi: float,
// }

// impl SphericalCoord {
//       to_vec3f(&self) -> Vec3f {
//         let sin_theta = float::sin(self.theta);
//         Vec3f {
//             x: self.radius * sin_theta * float::cos(self.phi),
//             y: self.radius * sin_theta * float::sin(self.phi),
//             z: self.radius * float::cos(self.theta),
//         }
//     }
// }

// impl Display for SphericalCoord {
//      fmt(&self, f: &mut Formatter<'_>) -> Result {
//         write!(f, "r={}, \u{03C6}={}, \u{03B8}={}", self.radius, self.theta, self.phi)
//     }
// }

//  struct Quaternion {
//     scalar: float,
//     vec: Vec3f,
// }

// impl Quaternion {
//     // Math taken from here:
//     // https://www.sciencedirect.com/topics/computer-science/quaternion-multiplication

//       mul(&self, other: &Self) -> Quaternion {
//         let s1 = self.scalar;
//         let s2 = other.scalar;
//         let v1 = &self.vec;
//         let v2 = &other.vec;

//         Quaternion {
//             scalar: s1 * s2 - v1.dot(&v2),
//             vec: v2.scale(s1) + v1.scale(s2) + v1.cross(&v2),
//         }
//     }

//       inverse(&self) -> Quaternion {
//         let vec_neg = Vec3f {
//             x: -self.vec.x,
//             y: -self.vec.y,
//             z: -self.vec.z,
//         };

//         let q = float::sqrt(self.scalar * self.scalar + vec_neg.magnitude_squared());
//         let inv_q = 1.0 / q;
//         let inv_q_squared = inv_q * inv_q;

//         Quaternion {
//             scalar: inv_q_squared * self.scalar,
//             vec: Vec3f {
//                 x: inv_q_squared * -self.vec.x,
//                 y: inv_q_squared * -self.vec.y,
//                 z: inv_q_squared * -self.vec.z,
//             },
//         }
//     }

//       rotation(axis: &Vec3f, radians: float) -> Quaternion {
//         let axis = axis.normalize();
//         let theta = radians / 2.0;
//         let sin_theta = float::sin(theta);

//         Quaternion {
//             scalar: float::cos(theta),
//             vec: Vec3f {
//                 x: axis.x * sin_theta,
//                 y: axis.y * sin_theta,
//                 z: axis.z * sin_theta,
//             },
//         }
//     }

//       to_vec3f(&self) -> Vec3f {
//         Vec3f {
//             x: self.vec.x,
//             y: self.vec.y,
//             z: self.vec.z,
//         }
//     }

//       to_mat4(&self) -> Mat4x4 {
//         let qr = self.scalar; 
//         let qi = self.vec.x;
//         let qj = self.vec.y;
//         let qk = self.vec.z;

//         let qr_squared = qr * qr;
//         let qi_squared = qi * qi;
//         let qj_squared = qj * qj;
//         let qk_squared = qk * qk;
        
//         let mut m = Mat4x4::identity();

//         m.data[0] = 2.0 * (qr_squared + qi_squared) - 1.0;
//         m.data[1] = 2.0 * (qi * qj + qr * qk);
//         m.data[2] = 2.0 * (qi * qk - qr * qj);
        
//         m.data[4] = 2.0 * (qi * qj - qr * qk);
//         m.data[5] = 2.0 * (qr_squared + qj_squared) - 1.0;
//         m.data[6] = 2.0 * (qj * qk + qr * qi);

//         m.data[8] = 2.0 * (qi * qk + qr * qj);
//         m.data[9] = 2.0 * (qj * qk - qr * qi);
//         m.data[10] = 2.0 * (qr_squared + qk_squared) - 1.0;

//         m
//     }

//       from_vec3f(v: Vec3f) -> Quaternion {
//         Quaternion {
//             scalar: 0.0,
//             vec: v,
//         }
//     }
// }
