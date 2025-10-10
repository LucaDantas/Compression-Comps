/*
	Matrix operations:
	This contains an optimized version for matrix exponentiation (fixed size square matrices)

	Also a version with more functionalities for general purpose
*/

#include <vector>
#include <iostream>
#include <cassert>
#include <string>

template<typename T>
class Matrix
{
private:
	std::vector<std::vector<T>> mat;

public:
	Matrix(int n) {
		mat.resize(n, std::vector<T>(n));
	}

	Matrix(int n, int m) {
		mat.resize(n, std::vector<T>(m));
	}

	Matrix() {}

	Matrix operator*(const Matrix& a) {
		int n = mat.size(), m = mat[0].size();
		int n2 = a.mat.size(), m2 = a.mat[0].size();

		assert(m == n2);

		Matrix c(n, m2);
		
		for(int i = 0; i < n; i++)
			for(int j = 0; j < m2; j++)
				for(int k = 0; k < m; k++)
					c.mat[i][j] += mat[i][k] * a.mat[k][j];

		return c;
	}

	Matrix transpose() {
		int n = mat.size(), m = mat[0].size();
		Matrix result(m, n);
		for(int i = 0; i < n; i++) {
			for(int j = 0; j < m; j++) {
				result.mat[j][i] = mat[i][j];
			}
		}
		return result;
	}

	friend std::ostream& operator<<(std::ostream &os, const Matrix& v) { 
		int n = v.mat.size(), m = v.mat[0].size();
		for(int i = 0; i < n; i++) {
			os << '['; std::string sep = "";
			for(int j = 0; j < m; j++) {
				os << sep << v.mat[i][j], sep = ", ";
			}
			os << "]\n";
		}
		return os;
	}
};

template<typename T>
class Vector
{
private:
	std::vector<T> vec;

public:
	Vector(int n) {
		vec.resize(n);
	}

	Vector() {}

	Vector(const std::vector<T>& v) : vec(v) {}

	// Convert to Matrix (row vector by default)
	Matrix<T> toMatrix(bool asRowVector = true) {
		if (asRowVector) {
			Matrix<T> result(1, vec.size());
			for(int i = 0; i < vec.size(); i++) {
				result.mat[0][i] = vec[i];
			}
			return result;
		} else {
			Matrix<T> result(vec.size(), 1);
			for(int i = 0; i < vec.size(); i++) {
				result.mat[i][0] = vec[i];
			}
			return result;
		}
	}

	// Dot product operator
	T operator*(const Vector& other) {
		assert(vec.size() == other.vec.size());
		T result = T(0);
		for(int i = 0; i < vec.size(); i++) {
			result += vec[i] * other.vec[i];
		}
		return result;
	}

	friend std::ostream& operator<<(std::ostream &os, const Vector& v) {
		os << '[';
		std::string sep = "";
		for(int i = 0; i < v.vec.size(); i++) {
			os << sep << v.vec[i];
			sep = ", ";
		}
		os << ']';
		return os;
	}
};


