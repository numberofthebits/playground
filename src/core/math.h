#ifndef MATH_H
#define MATH_H

#include <math.h>
#include <memory.h>
#include <stdint.h>

#define PI 3.1415927f
#define PI_MUL_2 2.f * PI
#define PI_DIV_2 PI / 2.f
#define PI_3_DIV_4 3.f / 4.f * PI
#define PI_DIV_4 PI / 4.f
#define PI_7_DIV_4 7.f / 4.f * PI

typedef struct {
  float x;
  float y;
} Vec2f;

typedef struct {
  float x;
  float y;
  float z;
} Vec3f;

typedef struct {
  uint8_t x;
  uint8_t y;
} Vec2u8;

typedef struct {
  uint32_t x;
  uint32_t y;
} Vec2u32;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Vec3u8;

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint8_t a;
} Vec4u8;

typedef struct {
  uint32_t r;
  uint32_t g;
  uint32_t b;
  uint32_t a;
} Vec4u32;

typedef struct {
  float data[16];
} Mat4x4;

typedef struct {
  Vec2f pos; // lower left
  float width;
  float height;
} Rectf;

float dot_vec2f(Vec2f *a, Vec2f *b);
float dot_vec3f(Vec3f *a, Vec3f *b);

Vec3f cross(Vec3f *a, Vec3f *b);
Vec3f scale(Vec3f *v, float scalar);

Vec2u32 mul_vec2u32(Vec2u32 *v, uint32_t factor);

float magnitude_squared_vec2f(Vec2f *v);
float magnitude_squared_vec3f(Vec3f *v);

float length_vec2f(Vec2f *v);
float length_vec3f(Vec3f *v);

Vec2f normalize_vec2f(Vec2f *v);
Vec3f normalize_vec3f(Vec3f *v);

Vec2f normalize_with_len_vec2f(Vec2f *v, float len);
Vec3f normalize_with_len_vec3f(Vec3f *v, float len);

// Subtract "b" from "a"
Vec3f sub(Vec3f *a, Vec3f *b);
Mat4x4 look_at(Vec3f *pos, Vec3f *target, Vec3f *up);
Mat4x4 zero(void);

void mat4_identity(Mat4x4 *m);

Mat4x4 perspective(float near, float far, float fov, float aspect);
Mat4x4 ortho(float near, float far, float right, float left, float top,
             float bottom);

void mat4_translate(Mat4x4 *mat, Vec3f *v);
void mat4_scale(Mat4x4 *mat, Vec3f *v);
void mat4_transpose(Mat4x4 *mat);
// 'axis' should be normalized
void mat4_rotate(Mat4x4 *mat, Vec3f *axis, float radians);
Mat4x4 mul(Mat4x4 *a, Mat4x4 *b);

int intersect_rectf(Rectf *a, Rectf *b);

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
//         write!(f, "r={}, \u{03C6}={}, \u{03B8}={}", self.radius, self.theta,
//         self.phi)
//     }
// }

//  struct Quaternion {
//     scalar: float,
//     vec: Vec3f,
// }

// impl Quaternion {
//     // Math taken from here:
//     //
//     https://www.sciencedirect.com/topics/computer-science/quaternion-multiplication

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

//         let q = float::sqrt(self.scalar * self.scalar +
//         vec_neg.magnitude_squared()); let inv_q = 1.0 / q; let inv_q_squared
//         = inv_q * inv_q;

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

#endif
