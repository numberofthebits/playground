#include "math.h"

#include "allocators.h"
#include "log.h"
#include "util.h"

#include <math.h>
#include <memory.h>
#include <stdint.h>

float vec2f_dot(Vec2f *a, Vec2f *b) { return a->x * b->x + a->y * b->y; }

float vec3f_dot(Vec3f *a, Vec3f *b) {
  return a->x * b->x + a->y * b->y + a->z * b->z;
}

Vec3f cross(Vec3f *a, Vec3f *b) {
  Vec3f result;
  result.x = a->y * b->z - a->z * b->y;
  result.y = a->z * b->x - a->x * b->z;
  result.z = a->x * b->y - a->y * b->x;

  return result;
}

Vec3f scale(Vec3f *v, float scalar) {
  Vec3f result;
  result.x = v->x * scalar;
  result.y = v->y * scalar;
  result.z = v->z * scalar;
  return result;
}

Vec2u32 mul_vec2u32(Vec2u32 *v, uint32_t factor) {
  Vec2u32 ret = {.x = v->x * factor, .y = v->y * factor};
  return ret;
}

float magnitude_squared_vec2f(Vec2f *v) { return v->x * v->x + v->y * v->y; }

float magnitude_squared_vec3f(Vec3f *v) {
  return v->x * v->x + v->y * v->y + v->z * v->z;
}

float length_vec2f(Vec2f *v) { return (float)sqrt(magnitude_squared_vec2f(v)); }

float length_vec3f(Vec3f *v) { return (float)sqrt(magnitude_squared_vec3f(v)); }

Vec2f normalize_vec2f(Vec2f *v) {
  float len = length_vec2f(v);
  Vec2f result;
  result.x = v->x / len;
  result.y = v->y / len;

  return result;
}

Vec2f normalize_with_len_vec2f(Vec2f *v, float len) {
  Vec2f result;
  result.x = v->x / len;
  result.y = v->y / len;

  return result;
}

Vec3f normalize_vec3f(Vec3f *v) {
  float len = length_vec3f(v);
  Vec3f result;
  result.x = v->x / len;
  result.y = v->y / len;
  result.z = v->z / len;

  return result;
}

Vec3f normalize_with_len_vec3f(Vec3f *v, float len) {
  Vec3f result;
  result.x = v->x / len;
  result.y = v->y / len;
  result.z = v->z / len;

  return result;
}

Vec3f vec3f_sub(Vec3f *a, Vec3f *b) {
  Vec3f result;

  result.x = a->x - b->x;
  result.y = a->y - b->y;
  result.z = a->z - b->z;

  return result;
}

Mat4x4 look_at(Vec3f *pos, Vec3f *target, Vec3f *up) {
  Vec3f dir = vec3f_sub(pos, target);
  Vec3f d = normalize_vec3f(&dir);
  Vec3f u = normalize_vec3f(up);
  Vec3f r = cross(&u, &d);
  Vec3f t = {vec3f_dot(pos, &r), vec3f_dot(pos, &u), vec3f_dot(pos, &d)};

  Mat4x4 m = {{r.x, u.x, d.x, 0.0, r.y, u.y, d.y, 0.0, r.z, u.z, d.z, 0.0, -t.x,
               -t.y, -t.z, 1.0}};
  return m;
}

Mat4x4 zero(void) {
  Mat4x4 m;
  memset(&m, 0x0, sizeof(Mat4x4));
  return m;
}

void mat4_identity(Mat4x4 *m) {
  static float Mat4x4_Identity[] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 0.0f, 1.0f};
  memcpy(&m->data[0], Mat4x4_Identity, sizeof(Mat4x4));
}

Mat4x4 perspective(float near, float far, float fov, float aspect) {
  float scale = 1.0f / (aspect * (float)tan(fov * 0.5f * PI / 180.0f));
  float f_min_n = far - near;

  Mat4x4 m;
  mat4_identity(&m);

  m.data[0] = scale;
  m.data[5] = scale;
  m.data[10] = -((far + near) / f_min_n);
  m.data[11] = -1.0f;
  m.data[14] = -(2.0f * far * near) / (far - near);

  return m;
}

Mat4x4 ortho(float near, float far, float right, float left, float top,
             float bottom) {
  Mat4x4 m;
  mat4_identity(&m);

  m.data[0] = 2.f / (right - left); // diagonal
  m.data[5] = 2.f / (top - bottom); // diagonal
  m.data[10] = -2.f / (far - near); // diagonal
  m.data[3] = -(right + left) / (right - left);
  m.data[7] = -(top + bottom) / (top - bottom);
  m.data[11] = -(far + near) / (far - near);
  m.data[15] = 1.f; // diagonal

  return m;
}

void mat4_translate(Mat4x4 *mat, Vec3f *v) {
  mat->data[12] += v->x;
  mat->data[13] += v->y;
  mat->data[14] += v->z;
}

void mat4_scale(Mat4x4 *mat, Vec3f *v) {
  mat->data[0] *= v->x;
  mat->data[5] *= v->y;
  mat->data[10] *= v->z;
}

void mat4_rotate(Mat4x4 *mat, Vec3f *axis, float radians) {
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

void mat4_transpose(Mat4x4 *mat) {
  float values[16] = {mat->data[0], mat->data[4], mat->data[8],  mat->data[12],
                      mat->data[1], mat->data[5], mat->data[9],  mat->data[13],
                      mat->data[2], mat->data[6], mat->data[10], mat->data[14],
                      mat->data[3], mat->data[7], mat->data[11], mat->data[15]};

  for (int i = 0; i < 16; ++i) {
    mat->data[i] = values[i];
  }
}

float mat3_determinant(Mat3x3 *m) {
  /*
    TODO: This is row major, isn't it...
    | 0 1 2 |
    | 3 4 5 |
    | 6 7 8 |
   */
  return m->data[0] * m->data[4] * m->data[8] +
         m->data[1] * m->data[5] * m->data[6] +
         m->data[2] * m->data[3] * m->data[7] -
         m->data[2] * m->data[4] * m->data[6] -
         m->data[1] * m->data[3] * m->data[8] -
         m->data[0] * m->data[5] * m->data[7];
}

Mat4x4 mul(Mat4x4 *a, Mat4x4 *b) {

  Mat4x4 m = zero();
  int index = 0;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a->data[i * 4 + k] * b->data[k * 4 + j];
      }
      m.data[index] = sum;
      index += 1;
    }
  }

  return m;
}

int intersect_rectf(Rectf *a, Rectf *b) {
  if (a->pos.x + a->width < b->pos.x || a->pos.x >= b->pos.x + b->width ||
      a->pos.y + a->height < b->pos.y || a->pos.y >= b->pos.y + b->height) {
    return 0;
  }

  return 1;
}

int mesh_intersect_ray(Mesh *mesh, Ray3f *ray,
                       MeshRayIntersection *intersect_out) {
  float epsilon = 0.01f;
  for (size_t i = 0; i < mesh->num_triangles; ++i) {
    Vec3f p0 = mesh->vertices[mesh->triangles[i].index_v0];
    Vec3f p1 = mesh->vertices[mesh->triangles[i].index_v1];
    Vec3f p2 = mesh->vertices[mesh->triangles[i].index_v2];

    Vec3f o = ray->origin;
    Vec3f d = ray->direction;
    Vec3f e1 = vec3f_sub(&p1, &p0);
    Vec3f e2 = vec3f_sub(&p2, &p0);
    Vec3f s = vec3f_sub(&o, &p0);

    // Optimization. Consolidate computation
    Vec3f q = cross(&d, &e2);
    Vec3f r = cross(&s, &e1);

    float a = vec3f_dot(&q, &e1);

    if (a > -epsilon && a < epsilon) {
      continue;
    }

    float factor = 1.f / vec3f_dot(&q, &e1);

    float u = factor * vec3f_dot(&q, &s);
    if (u < 0.f) {
      continue;
    }

    float v = factor * vec3f_dot(&r, &d);
    if (v < 0.f || u + v > 1.f) {
      continue;
    }

    float t = factor * vec3f_dot(&r, &e2);

    intersect_out->triangle_index = i;
    intersect_out->tuv.x = t;
    intersect_out->tuv.y = u;
    intersect_out->tuv.z = v;

    return 1;
  }

  return 0;
}

void mesh_init(Mesh *mesh, uint32_t size_vertices, uint32_t size_edges,
               uint32_t size_triangles) {
  mesh->vertices =
      ArenaAlloc(&global_static_allocator, size_vertices, Vertex3f);
  mesh->edges = ArenaAlloc(&global_static_allocator, size_edges, Edge);
  mesh->triangles =
      ArenaAlloc(&global_static_allocator, size_triangles, Triangle);

  mesh->size_vertices = size_vertices;
  mesh->size_edges = size_edges;
  mesh->size_triangles = size_triangles;

  mesh->num_vertices = 0;
  mesh->num_edges = 0;
  mesh->num_triangles = 0;
}

void mesh_add_triangle(Mesh *mesh, Vertex3f *a, Vertex3f *b, Vertex3f *c) {
  if (mesh->size_vertices - mesh->num_vertices < 3) {
    LOG_WARN("No space to add triangle (%d / %d)", mesh->num_vertices,
             mesh->size_vertices);

    return;
  }

  if (mesh->size_edges - mesh->num_edges < 3) {
    LOG_WARN("No space to add edges (%d / %d)", mesh->num_edges,
             mesh->size_edges);
    return;
  }

  if (mesh->num_triangles == mesh->size_triangles) {
    LOG_WARN("No space to add triangles (%d / %d)", mesh->num_triangles,
             mesh->size_triangles);
  }

  mesh->vertices[mesh->num_vertices++] = *a;
  mesh->vertices[mesh->num_vertices++] = *b;
  mesh->vertices[mesh->num_vertices++] = *c;

  Edge *ea = &mesh->edges[mesh->num_edges++];
  ea->a = mesh->num_vertices - 3;
  ea->b = mesh->num_vertices - 2;

  Edge *eb = &mesh->edges[mesh->num_edges++];
  eb->a = mesh->num_vertices - 2;
  eb->b = mesh->num_vertices - 1;

  Edge *ec = &mesh->edges[mesh->num_edges++];
  ec->a = mesh->num_vertices - 1;
  ec->b = mesh->num_vertices - 3;

  Triangle *t = &mesh->triangles[mesh->num_triangles++];
  t->index_edge_ab = mesh->num_edges - 3;
  t->index_edge_bc = mesh->num_edges - 2;
  t->index_edge_ca = mesh->num_edges - 1;
  t->index_v0 = mesh->num_vertices - 3;
  t->index_v1 = mesh->num_vertices - 2;
  t->index_v2 = mesh->num_vertices - 1;
}

int vec3f_cmp_eq(Vec3f *a, Vec3f *b) {
  return a->x == b->x && a->y == b->y && a->z == b->z;
}

inline uint32_t mesh_find_vertex_index(Mesh *mesh, Vertex3f *v) {
  for (uint32_t i = 0; i < mesh->num_vertices; ++i) {
    if (vec3f_cmp_eq(&mesh->vertices[i], v)) {
      return i;
    }
  }

  return MESH_INVALID_VERTEX_INDEX;
}

void mesh_extrude_edge_v(Mesh *mesh, Vertex3f *a, Vertex3f *b,
                         Vertex3f *new_vertex) {
  uint32_t index_va = mesh_find_vertex_index(mesh, a);
  uint32_t index_vb = mesh_find_vertex_index(mesh, b);

  if (index_va == MESH_INVALID_VERTEX_INDEX ||
      index_vb == MESH_INVALID_VERTEX_INDEX) {
    LOG_WARN("Failed to find mesh index");
    return;
  }

  uint32_t index_new_vertex = mesh->num_vertices;
  mesh->vertices[mesh->num_vertices++] = *new_vertex;

  Edge *ea = &mesh->edges[mesh->num_edges++];
  ea->a = index_va;
  ea->b = index_new_vertex;

  Edge *eb = &mesh->edges[mesh->num_edges++];
  eb->a = index_vb;
  eb->b = index_new_vertex;

  Triangle *t = &mesh->triangles[mesh->num_triangles++];
  t->index_edge_ab = mesh->num_edges - 3;
  t->index_edge_bc = mesh->num_edges - 2;
  t->index_edge_ca = mesh->num_edges - 1;
  t->index_v0 = index_va;
  t->index_v1 = index_vb;
  t->index_v2 = index_new_vertex;
}

#ifdef BUILD_TESTS
static void math_test_mesh_gen() {
  Mat3x3 mat3 = {1.0f, 2.f, 3.f, 3.f, 2.f, 1.f, 2.f, 1.f, 3.f};

  float det = mat3_determinant(&mat3);
  Assert(det == -12.f);

  Mesh mesh = {0};
  mesh_init(&mesh, 6, 9, 3);

  float s = 1.f;
  Vertex3f v[6] = {{.x = -s, .y = -s, .z = 0.f}, {.x = s, .y = -s, .z = 0.f},
                   {.x = 0.f, .y = s, .z = 0.f}, {.x = -s, .y = s, .z = 0.f},
                   {.x = s, .y = s, .z = 0.f},   {.x = 0.f, .y = -s, .z = 0.f}};

  mesh_add_triangle(&mesh, &v[0], &v[1], &v[2]);
  mesh_extrude_edge_v(&mesh, &v[0], &v[1], &v[3]);
  mesh_extrude_edge_v(&mesh, &v[0], &v[1], &v[4]);
  mesh_extrude_edge_v(&mesh, &v[0], &v[1], &v[5]);

  Assert(mesh.num_triangles == 4);
  Assert(mesh.num_edges == 9);
  Assert(mesh.num_vertices == 6);
}

static void math_test_mesh_ray_intersect() {
  Mesh mesh = {0};
  mesh_init(&mesh, 9, 9, 3);

  float s = 1.f;
  Vertex3f v[] = {
      {.x = -s, .y = -s, .z = 0.f},   {.x = s, .y = -s, .z = 0.f},
      {.x = 0.f, .y = s, .z = 0.f},   {.x = 0.f, .y = -s, .z = -1.f},
      {.x = 0.f, .y = -s, .z = 1.f},  {.x = 0.f, .y = s, .z = 0.f},
      {.x = -1.f, .y = s, .z = -1.f}, {.x = 1.f, .y = s, .z = -1.f},
      {.x = 0.f, .y = s, .z = 1.f}};

  for (unsigned int i = 0; i < (sizeof(v) / sizeof(Vertex3f)); i += 3) {
    mesh_add_triangle(&mesh, &v[i + 0], &v[i + 1], &v[i + 2]);
  }

  Assert(mesh.num_triangles == 3);
  Assert(mesh.num_edges == 9);
  Assert(mesh.num_vertices == 9);

  // Left/right side and front back of triangle
  Ray3f ray0 = {.origin = {0.f, 0.f, 100.f}, .direction = {0.f, 0.f, -1.f}};
  Ray3f ray1 = {.origin = {0.f, 0.f, -100.f}, .direction = {0.f, 0.f, 1.f}};
  Ray3f ray2 = {.origin = {1.f, -1.f, -1.f}, .direction = {0.f, 0.f, 1.f}};
  Ray3f ray3 = {.origin = {1.f, -1.f, 1.f}, .direction = {0.f, 0.f, -1.f}};

  MeshRayIntersection out = {0};
  Assert(mesh_intersect_ray(&mesh, &ray0, &out));
  Assert(mesh_intersect_ray(&mesh, &ray1, &out));
  Assert(mesh_intersect_ray(&mesh, &ray2, &out));
  Assert(mesh_intersect_ray(&mesh, &ray3, &out));

  // Miss on the left side
  Ray3f ray4 = {.origin = {-1.f, -1.f, 1.f}, .direction = {-1.f, 0.f, -1.f}};
  Assert(!mesh_intersect_ray(&mesh, &ray4, &out));

  // Top down
  Ray3f ray5 = {.origin = {0.f, 10.f, 0.f}, .direction = {0.f, -1.f, 0.f}};
  Assert(mesh_intersect_ray(&mesh, &ray5, &out));

  // Left to right
  Ray3f ray6 = {.origin = {-10.f, 0.f, 0.f}, .direction = {1.f, 0.f, 0.f}};
  Assert(mesh_intersect_ray(&mesh, &ray6, &out));
}

static void math_test_mat3_determinant() {
  Mat3x3 mat3 = {1.0f, 2.f, 3.f, 3.f, 2.f, 1.f, 2.f, 1.f, 3.f};

  float det = mat3_determinant(&mat3);
  Assert(det == -12.f);
}

void math_test() {
  math_test_mat3_determinant();
  math_test_mesh_gen();
  math_test_mesh_ray_intersect();
}
#endif

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
//         write!(f, "r={}, \u{03C6}={}, \u{03B8}={}",
//         self.radius, self.theta, self.phi)
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
//         vec_neg.magnitude_squared()); let inv_q = 1.0 / q; let
//         inv_q_squared = inv_q * inv_q;

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
