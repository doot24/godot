/*************************************************************************/
/*  matrix3.cpp                                                          */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2017 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2017 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "matrix3.h"
#include "math_funcs.h"
#include "os/copymem.h"
#include "print_string.h"
#define cofac(row1, col1, row2, col2) \
	(elements[row1][col1] * elements[row2][col2] - elements[row1][col2] * elements[row2][col1])

void Basis::from_z(const Vector3 &p_z) {

	if (Math::abs(p_z.z) > Math_SQRT12) {

		// choose p in y-z plane
		real_t a = p_z[1] * p_z[1] + p_z[2] * p_z[2];
		real_t k = 1.0 / Math::sqrt(a);
		elements[0] = Vector3(0, -p_z[2] * k, p_z[1] * k);
		elements[1] = Vector3(a * k, -p_z[0] * elements[0][2], p_z[0] * elements[0][1]);
	} else {

		// choose p in x-y plane
		real_t a = p_z.x * p_z.x + p_z.y * p_z.y;
		real_t k = 1.0 / Math::sqrt(a);
		elements[0] = Vector3(-p_z.y * k, p_z.x * k, 0);
		elements[1] = Vector3(-p_z.z * elements[0].y, p_z.z * elements[0].x, a * k);
	}
	elements[2] = p_z;
}

void Basis::invert() {

	real_t co[3] = {
		cofac(1, 1, 2, 2), cofac(1, 2, 2, 0), cofac(1, 0, 2, 1)
	};
	real_t det = elements[0][0] * co[0] +
				 elements[0][1] * co[1] +
				 elements[0][2] * co[2];
#ifdef MATH_CHECKS
	ERR_FAIL_COND(det == 0);
#endif
	real_t s = 1.0 / det;

	set(co[0] * s, cofac(0, 2, 2, 1) * s, cofac(0, 1, 1, 2) * s,
			co[1] * s, cofac(0, 0, 2, 2) * s, cofac(0, 2, 1, 0) * s,
			co[2] * s, cofac(0, 1, 2, 0) * s, cofac(0, 0, 1, 1) * s);
}

void Basis::orthonormalize() {
#ifdef MATH_CHECKS
	ERR_FAIL_COND(determinant() == 0);
#endif
	// Gram-Schmidt Process

	Vector3 x = get_axis(0);
	Vector3 y = get_axis(1);
	Vector3 z = get_axis(2);

	x.normalize();
	y = (y - x * (x.dot(y)));
	y.normalize();
	z = (z - x * (x.dot(z)) - y * (y.dot(z)));
	z.normalize();

	set_axis(0, x);
	set_axis(1, y);
	set_axis(2, z);
}

Basis Basis::orthonormalized() const {

	Basis c = *this;
	c.orthonormalize();
	return c;
}

bool Basis::is_orthogonal() const {
	Basis id;
	Basis m = (*this) * transposed();

	return is_equal_approx(id, m);
}

bool Basis::is_rotation() const {
	return Math::is_equal_approx(determinant(), 1) && is_orthogonal();
}

bool Basis::is_symmetric() const {

	if (!Math::is_equal_approx(elements[0][1], elements[1][0]))
		return false;
	if (!Math::is_equal_approx(elements[0][2], elements[2][0]))
		return false;
	if (!Math::is_equal_approx(elements[1][2], elements[2][1]))
		return false;

	return true;
}

Basis Basis::diagonalize() {

//NOTE: only implemented for symmetric matrices
//with the Jacobi iterative method method
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(!is_symmetric(), Basis());
#endif
	const int ite_max = 1024;

	real_t off_matrix_norm_2 = elements[0][1] * elements[0][1] + elements[0][2] * elements[0][2] + elements[1][2] * elements[1][2];

	int ite = 0;
	Basis acc_rot;
	while (off_matrix_norm_2 > CMP_EPSILON2 && ite++ < ite_max) {
		real_t el01_2 = elements[0][1] * elements[0][1];
		real_t el02_2 = elements[0][2] * elements[0][2];
		real_t el12_2 = elements[1][2] * elements[1][2];
		// Find the pivot element
		int i, j;
		if (el01_2 > el02_2) {
			if (el12_2 > el01_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 1;
			}
		} else {
			if (el12_2 > el02_2) {
				i = 1;
				j = 2;
			} else {
				i = 0;
				j = 2;
			}
		}

		// Compute the rotation angle
		real_t angle;
		if (Math::is_equal_approx(elements[j][j], elements[i][i])) {
			angle = Math_PI / 4;
		} else {
			angle = 0.5 * Math::atan(2 * elements[i][j] / (elements[j][j] - elements[i][i]));
		}

		// Compute the rotation matrix
		Basis rot;
		rot.elements[i][i] = rot.elements[j][j] = Math::cos(angle);
		rot.elements[i][j] = -(rot.elements[j][i] = Math::sin(angle));

		// Update the off matrix norm
		off_matrix_norm_2 -= elements[i][j] * elements[i][j];

		// Apply the rotation
		*this = rot * *this * rot.transposed();
		acc_rot = rot * acc_rot;
	}

	return acc_rot;
}

Basis Basis::inverse() const {

	Basis inv = *this;
	inv.invert();
	return inv;
}

void Basis::transpose() {

	SWAP(elements[0][1], elements[1][0]);
	SWAP(elements[0][2], elements[2][0]);
	SWAP(elements[1][2], elements[2][1]);
}

Basis Basis::transposed() const {

	Basis tr = *this;
	tr.transpose();
	return tr;
}

// Multiplies the matrix from left by the scaling matrix: M -> S.M
// See the comment for Basis::rotated for further explanation.
void Basis::scale(const Vector3 &p_scale) {

	elements[0][0] *= p_scale.x;
	elements[0][1] *= p_scale.x;
	elements[0][2] *= p_scale.x;
	elements[1][0] *= p_scale.y;
	elements[1][1] *= p_scale.y;
	elements[1][2] *= p_scale.y;
	elements[2][0] *= p_scale.z;
	elements[2][1] *= p_scale.z;
	elements[2][2] *= p_scale.z;
}

Basis Basis::scaled(const Vector3 &p_scale) const {

	Basis m = *this;
	m.scale(p_scale);
	return m;
}

Vector3 Basis::get_scale() const {
	// FIXME: We are assuming M = R.S (R is rotation and S is scaling), and use polar decomposition to extract R and S.
	// A polar decomposition is M = O.P, where O is an orthogonal matrix (meaning rotation and reflection) and
	// P is a positive semi-definite matrix (meaning it contains absolute values of scaling along its diagonal).
	//
	// Despite being different from what we want to achieve, we can nevertheless make use of polar decomposition
	// here as follows. We can split O into a rotation and a reflection as O = R.Q, and obtain M = R.S where
	// we defined S = Q.P. Now, R is a proper rotation matrix and S is a (signed) scaling matrix,
	// which can involve negative scalings. However, there is a catch: unlike the polar decomposition of M = O.P,
	// the decomposition of O into a rotation and reflection matrix as O = R.Q is not unique.
	// Therefore, we are going to do this decomposition by sticking to a particular convention.
	// This may lead to confusion for some users though.
	//
	// The convention we use here is to absorb the sign flip into the scaling matrix.
	// The same convention is also used in other similar functions such as set_scale,
	// get_rotation_axis_angle, get_rotation, set_rotation_axis_angle, set_rotation_euler, ...
	//
	// A proper way to get rid of this issue would be to store the scaling values (or at least their signs)
	// as a part of Basis. However, if we go that path, we need to disable direct (write) access to the
	// matrix elements.
	real_t det_sign = determinant() > 0 ? 1 : -1;
	return det_sign * Vector3(
							  Vector3(elements[0][0], elements[1][0], elements[2][0]).length(),
							  Vector3(elements[0][1], elements[1][1], elements[2][1]).length(),
							  Vector3(elements[0][2], elements[1][2], elements[2][2]).length());
}

// Sets scaling while preserving rotation.
// This requires some care when working with matrices with negative determinant,
// since we're using a particular convention for "polar" decomposition in get_scale and get_rotation.
// For details, see the explanation in get_scale.
void Basis::set_scale(const Vector3 &p_scale) {
	Vector3 e = get_euler();
	Basis(); // reset to identity
	scale(p_scale);
	rotate(e);
}

// Multiplies the matrix from left by the rotation matrix: M -> R.M
// Note that this does *not* rotate the matrix itself.
//
// The main use of Basis is as Transform.basis, which is used a the transformation matrix
// of 3D object. Rotate here refers to rotation of the object (which is R * (*this)),
// not the matrix itself (which is R * (*this) * R.transposed()).
Basis Basis::rotated(const Vector3 &p_axis, real_t p_phi) const {
	return Basis(p_axis, p_phi) * (*this);
}

void Basis::rotate(const Vector3 &p_axis, real_t p_phi) {
	*this = rotated(p_axis, p_phi);
}

Basis Basis::rotated(const Vector3 &p_euler) const {
	return Basis(p_euler) * (*this);
}

void Basis::rotate(const Vector3 &p_euler) {
	*this = rotated(p_euler);
}

// TODO: rename this to get_rotation_euler
Vector3 Basis::get_rotation() const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	return m.get_euler();
}

void Basis::get_rotation_axis_angle(Vector3 &p_axis, real_t &p_angle) const {
	// Assumes that the matrix can be decomposed into a proper rotation and scaling matrix as M = R.S,
	// and returns the Euler angles corresponding to the rotation part, complementing get_scale().
	// See the comment in get_scale() for further information.
	Basis m = orthonormalized();
	real_t det = m.determinant();
	if (det < 0) {
		// Ensure that the determinant is 1, such that result is a proper rotation matrix which can be represented by Euler angles.
		m.scale(Vector3(-1, -1, -1));
	}

	m.get_axis_angle(p_axis, p_angle);
}

// Sets rotation while preserving scaling.
// This requires some care when working with matrices with negative determinant,
// since we're using a particular convention for "polar" decomposition in get_scale and get_rotation.
// For details, see the explanation in get_scale.
void Basis::set_rotation_euler(const Vector3 &p_euler) {
	Vector3 s = get_scale();
	Basis(); // reset to identity
	scale(s);
	rotate(p_euler);
}

// Sets rotation while preserving scaling.
// This requires some care when working with matrices with negative determinant,
// since we're using a particular convention for "polar" decomposition in get_scale and get_rotation.
// For details, see the explanation in get_scale.
void Basis::set_rotation_axis_angle(const Vector3 &p_axis, real_t p_angle) {
	Vector3 s = get_scale();
	Basis(); // reset to identity
	scale(s);
	rotate(p_axis, p_angle);
}

// get_euler_xyz returns a vector containing the Euler angles in the format
// (a1,a2,a3), where a3 is the angle of the first rotation, and a1 is the last
// (following the convention they are commonly defined in the literature).
//
// The current implementation uses XYZ convention (Z is the first rotation),
// so euler.z is the angle of the (first) rotation around Z axis and so on,
//
// And thus, assuming the matrix is a rotation matrix, this function returns
// the angles in the decomposition R = X(a1).Y(a2).Z(a3) where Z(a) rotates
// around the z-axis by a and so on.
Vector3 Basis::get_euler_xyz() const {

	// Euler angles in XYZ convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cy*cz          -cy*sz           sy
	//        cz*sx*sy+cx*sz  cx*cz-sx*sy*sz -cy*sx
	//       -cx*cz*sy+sx*sz  cz*sx+cx*sy*sz  cx*cy

	Vector3 euler;
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(is_rotation() == false, euler);
#endif
	euler.y = Math::asin(elements[0][2]);
	if (euler.y < Math_PI * 0.5) {
		if (euler.y > -Math_PI * 0.5) {
			//if rotation is Y-only, return a proper -pi,pi range like in x or z for the same case.
			if (elements[1][0] == 0.0 && elements[0][1] == 0.0 && elements[0][0] < 0.0) {
				euler.x = 0;
				euler.z = 0;

				if (euler.y > 0.0)
					euler.y = Math_PI - euler.y;
				else
					euler.y = -(Math_PI + euler.y);
			} else {
				euler.x = Math::atan2(-elements[1][2], elements[2][2]);
				euler.z = Math::atan2(-elements[0][1], elements[0][0]);
			}

		} else {
			real_t r = Math::atan2(elements[1][0], elements[1][1]);
			euler.z = 0.0;
			euler.x = euler.z - r;
		}
	} else {
		real_t r = Math::atan2(elements[0][1], elements[1][1]);
		euler.z = 0;
		euler.x = r - euler.z;
	}

	return euler;
}

// set_euler_xyz expects a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// The current implementation uses XYZ convention (Z is the first rotation).
void Basis::set_euler_xyz(const Vector3 &p_euler) {

	real_t c, s;

	c = Math::cos(p_euler.x);
	s = Math::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = Math::cos(p_euler.y);
	s = Math::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = Math::cos(p_euler.z);
	s = Math::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	//optimizer will optimize away all this anyway
	*this = xmat * (ymat * zmat);
}

// get_euler_yxz returns a vector containing the Euler angles in the YXZ convention,
// as in first-Z, then-X, last-Y. The angles for X, Y, and Z rotations are returned
// as the x, y, and z components of a Vector3 respectively.
Vector3 Basis::get_euler_yxz() const {

	// Euler angles in YXZ convention.
	// See https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix
	//
	// rot =  cy*cz+sy*sx*sz    cz*sy*sx-cy*sz        cx*sy
	//        cx*sz             cx*cz                 -sx
	//        cy*sx*sz-cz*sy    cy*cz*sx+sy*sz        cy*cx

	Vector3 euler;
#ifdef MATH_CHECKS
	ERR_FAIL_COND_V(is_rotation() == false, euler);
#endif
	real_t m12 = elements[1][2];

	if (m12 < 1) {
		if (m12 > -1) {
			if (elements[1][0] == 0 && elements[0][1] == 0 && elements[2][2] < 0) { // use pure x rotation
				real_t x = asin(-m12);
				euler.y = 0;
				euler.z = 0;

				if (x > 0.0)
					euler.x = Math_PI - x;
				else
					euler.x = -(Math_PI + x);
			} else {
				euler.x = asin(-m12);
				euler.y = atan2(elements[0][2], elements[2][2]);
				euler.z = atan2(elements[1][0], elements[1][1]);
			}
		} else { // m12 == -1
			euler.x = Math_PI * 0.5;
			euler.y = -atan2(-elements[0][1], elements[0][0]);
			euler.z = 0;
		}
	} else { // m12 == 1
		euler.x = -Math_PI * 0.5;
		euler.y = -atan2(-elements[0][1], elements[0][0]);
		euler.z = 0;
	}

	return euler;
}

// set_euler_yxz expects a vector containing the Euler angles in the format
// (ax,ay,az), where ax is the angle of rotation around x axis,
// and similar for other axes.
// The current implementation uses YXZ convention (Z is the first rotation).
void Basis::set_euler_yxz(const Vector3 &p_euler) {

	real_t c, s;

	c = Math::cos(p_euler.x);
	s = Math::sin(p_euler.x);
	Basis xmat(1.0, 0.0, 0.0, 0.0, c, -s, 0.0, s, c);

	c = Math::cos(p_euler.y);
	s = Math::sin(p_euler.y);
	Basis ymat(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c);

	c = Math::cos(p_euler.z);
	s = Math::sin(p_euler.z);
	Basis zmat(c, -s, 0.0, s, c, 0.0, 0.0, 0.0, 1.0);

	//optimizer will optimize away all this anyway
	*this = ymat * xmat * zmat;
}

bool Basis::is_equal_approx(const Basis &a, const Basis &b) const {

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (Math::is_equal_approx(a.elements[i][j], b.elements[i][j]) == false)
				return false;
		}
	}

	return true;
}

bool Basis::operator==(const Basis &p_matrix) const {

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			if (elements[i][j] != p_matrix.elements[i][j])
				return false;
		}
	}

	return true;
}

bool Basis::operator!=(const Basis &p_matrix) const {

	return (!(*this == p_matrix));
}

Basis::operator String() const {

	String mtx;
	for (int i = 0; i < 3; i++) {

		for (int j = 0; j < 3; j++) {

			if (i != 0 || j != 0)
				mtx += ", ";

			mtx += rtos(elements[i][j]);
		}
	}

	return mtx;
}

Basis::operator Quat() const {
	//commenting this check because precision issues cause it to fail when it shouldn't
	//#ifdef MATH_CHECKS
	//ERR_FAIL_COND_V(is_rotation() == false, Quat());
	//#endif
	real_t trace = elements[0][0] + elements[1][1] + elements[2][2];
	real_t temp[4];

	if (trace > 0.0) {
		real_t s = Math::sqrt(trace + 1.0);
		temp[3] = (s * 0.5);
		s = 0.5 / s;

		temp[0] = ((elements[2][1] - elements[1][2]) * s);
		temp[1] = ((elements[0][2] - elements[2][0]) * s);
		temp[2] = ((elements[1][0] - elements[0][1]) * s);
	} else {
		int i = elements[0][0] < elements[1][1] ?
						(elements[1][1] < elements[2][2] ? 2 : 1) :
						(elements[0][0] < elements[2][2] ? 2 : 0);
		int j = (i + 1) % 3;
		int k = (i + 2) % 3;

		real_t s = Math::sqrt(elements[i][i] - elements[j][j] - elements[k][k] + 1.0);
		temp[i] = s * 0.5;
		s = 0.5 / s;

		temp[3] = (elements[k][j] - elements[j][k]) * s;
		temp[j] = (elements[j][i] + elements[i][j]) * s;
		temp[k] = (elements[k][i] + elements[i][k]) * s;
	}

	return Quat(temp[0], temp[1], temp[2], temp[3]);
}

static const Basis _ortho_bases[24] = {
	Basis(1, 0, 0, 0, 1, 0, 0, 0, 1),
	Basis(0, -1, 0, 1, 0, 0, 0, 0, 1),
	Basis(-1, 0, 0, 0, -1, 0, 0, 0, 1),
	Basis(0, 1, 0, -1, 0, 0, 0, 0, 1),
	Basis(1, 0, 0, 0, 0, -1, 0, 1, 0),
	Basis(0, 0, 1, 1, 0, 0, 0, 1, 0),
	Basis(-1, 0, 0, 0, 0, 1, 0, 1, 0),
	Basis(0, 0, -1, -1, 0, 0, 0, 1, 0),
	Basis(1, 0, 0, 0, -1, 0, 0, 0, -1),
	Basis(0, 1, 0, 1, 0, 0, 0, 0, -1),
	Basis(-1, 0, 0, 0, 1, 0, 0, 0, -1),
	Basis(0, -1, 0, -1, 0, 0, 0, 0, -1),
	Basis(1, 0, 0, 0, 0, 1, 0, -1, 0),
	Basis(0, 0, -1, 1, 0, 0, 0, -1, 0),
	Basis(-1, 0, 0, 0, 0, -1, 0, -1, 0),
	Basis(0, 0, 1, -1, 0, 0, 0, -1, 0),
	Basis(0, 0, 1, 0, 1, 0, -1, 0, 0),
	Basis(0, -1, 0, 0, 0, 1, -1, 0, 0),
	Basis(0, 0, -1, 0, -1, 0, -1, 0, 0),
	Basis(0, 1, 0, 0, 0, -1, -1, 0, 0),
	Basis(0, 0, 1, 0, -1, 0, 1, 0, 0),
	Basis(0, 1, 0, 0, 0, 1, 1, 0, 0),
	Basis(0, 0, -1, 0, 1, 0, 1, 0, 0),
	Basis(0, -1, 0, 0, 0, -1, 1, 0, 0)
};

int Basis::get_orthogonal_index() const {

	//could be sped up if i come up with a way
	Basis orth = *this;
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {

			real_t v = orth[i][j];
			if (v > 0.5)
				v = 1.0;
			else if (v < -0.5)
				v = -1.0;
			else
				v = 0;

			orth[i][j] = v;
		}
	}

	for (int i = 0; i < 24; i++) {

		if (_ortho_bases[i] == orth)
			return i;
	}

	return 0;
}

void Basis::set_orthogonal_index(int p_index) {

	//there only exist 24 orthogonal bases in r3
	ERR_FAIL_INDEX(p_index, 24);

	*this = _ortho_bases[p_index];
}

void Basis::get_axis_angle(Vector3 &r_axis, real_t &r_angle) const {
#ifdef MATH_CHECKS
	ERR_FAIL_COND(is_rotation() == false);
#endif
	real_t angle, x, y, z; // variables for result
	real_t epsilon = 0.01; // margin to allow for rounding errors
	real_t epsilon2 = 0.1; // margin to distinguish between 0 and 180 degrees

	if ((Math::abs(elements[1][0] - elements[0][1]) < epsilon) && (Math::abs(elements[2][0] - elements[0][2]) < epsilon) && (Math::abs(elements[2][1] - elements[1][2]) < epsilon)) {
		// singularity found
		// first check for identity matrix which must have +1 for all terms
		//  in leading diagonaland zero in other terms
		if ((Math::abs(elements[1][0] + elements[0][1]) < epsilon2) && (Math::abs(elements[2][0] + elements[0][2]) < epsilon2) && (Math::abs(elements[2][1] + elements[1][2]) < epsilon2) && (Math::abs(elements[0][0] + elements[1][1] + elements[2][2] - 3) < epsilon2)) {
			// this singularity is identity matrix so angle = 0
			r_axis = Vector3(0, 1, 0);
			r_angle = 0;
			return;
		}
		// otherwise this singularity is angle = 180
		angle = Math_PI;
		real_t xx = (elements[0][0] + 1) / 2;
		real_t yy = (elements[1][1] + 1) / 2;
		real_t zz = (elements[2][2] + 1) / 2;
		real_t xy = (elements[1][0] + elements[0][1]) / 4;
		real_t xz = (elements[2][0] + elements[0][2]) / 4;
		real_t yz = (elements[2][1] + elements[1][2]) / 4;
		if ((xx > yy) && (xx > zz)) { // elements[0][0] is the largest diagonal term
			if (xx < epsilon) {
				x = 0;
				y = 0.7071;
				z = 0.7071;
			} else {
				x = Math::sqrt(xx);
				y = xy / x;
				z = xz / x;
			}
		} else if (yy > zz) { // elements[1][1] is the largest diagonal term
			if (yy < epsilon) {
				x = 0.7071;
				y = 0;
				z = 0.7071;
			} else {
				y = Math::sqrt(yy);
				x = xy / y;
				z = yz / y;
			}
		} else { // elements[2][2] is the largest diagonal term so base result on this
			if (zz < epsilon) {
				x = 0.7071;
				y = 0.7071;
				z = 0;
			} else {
				z = Math::sqrt(zz);
				x = xz / z;
				y = yz / z;
			}
		}
		r_axis = Vector3(x, y, z);
		r_angle = angle;
		return;
	}
	// as we have reached here there are no singularities so we can handle normally
	real_t s = Math::sqrt((elements[1][2] - elements[2][1]) * (elements[1][2] - elements[2][1]) + (elements[2][0] - elements[0][2]) * (elements[2][0] - elements[0][2]) + (elements[0][1] - elements[1][0]) * (elements[0][1] - elements[1][0])); // s=|axis||sin(angle)|, used to normalise

	angle = Math::acos((elements[0][0] + elements[1][1] + elements[2][2] - 1) / 2);
	if (angle < 0) s = -s;
	x = (elements[2][1] - elements[1][2]) / s;
	y = (elements[0][2] - elements[2][0]) / s;
	z = (elements[1][0] - elements[0][1]) / s;

	r_axis = Vector3(x, y, z);
	r_angle = angle;
}

Basis::Basis(const Vector3 &p_euler) {

	set_euler(p_euler);
}

Basis::Basis(const Quat &p_quat) {

	real_t d = p_quat.length_squared();
	real_t s = 2.0 / d;
	real_t xs = p_quat.x * s, ys = p_quat.y * s, zs = p_quat.z * s;
	real_t wx = p_quat.w * xs, wy = p_quat.w * ys, wz = p_quat.w * zs;
	real_t xx = p_quat.x * xs, xy = p_quat.x * ys, xz = p_quat.x * zs;
	real_t yy = p_quat.y * ys, yz = p_quat.y * zs, zz = p_quat.z * zs;
	set(1.0 - (yy + zz), xy - wz, xz + wy,
			xy + wz, 1.0 - (xx + zz), yz - wx,
			xz - wy, yz + wx, 1.0 - (xx + yy));
}

void Basis::set_axis_angle(const Vector3 &p_axis, real_t p_phi) {
// Rotation matrix from axis and angle, see https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_angle
#ifdef MATH_CHECKS
	ERR_FAIL_COND(p_axis.is_normalized() == false);
#endif
	Vector3 axis_sq(p_axis.x * p_axis.x, p_axis.y * p_axis.y, p_axis.z * p_axis.z);

	real_t cosine = Math::cos(p_phi);
	real_t sine = Math::sin(p_phi);

	elements[0][0] = axis_sq.x + cosine * (1.0 - axis_sq.x);
	elements[0][1] = p_axis.x * p_axis.y * (1.0 - cosine) - p_axis.z * sine;
	elements[0][2] = p_axis.z * p_axis.x * (1.0 - cosine) + p_axis.y * sine;

	elements[1][0] = p_axis.x * p_axis.y * (1.0 - cosine) + p_axis.z * sine;
	elements[1][1] = axis_sq.y + cosine * (1.0 - axis_sq.y);
	elements[1][2] = p_axis.y * p_axis.z * (1.0 - cosine) - p_axis.x * sine;

	elements[2][0] = p_axis.z * p_axis.x * (1.0 - cosine) - p_axis.y * sine;
	elements[2][1] = p_axis.y * p_axis.z * (1.0 - cosine) + p_axis.x * sine;
	elements[2][2] = axis_sq.z + cosine * (1.0 - axis_sq.z);
}

Basis::Basis(const Vector3 &p_axis, real_t p_phi) {
	set_axis_angle(p_axis, p_phi);
}
