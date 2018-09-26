#include <iostream>
#include <opencv2/opencv.hpp>
using namespace cv;
using namespace std;

int bilinear_interpolation(const Mat& src, Mat& dst, float src_row, float src_col, int dst_row, int dst_col) {
	if (src_row >= 0 && src_row <= src.rows - 1 && src_col >= 0 && src_col <= src.cols - 1) {
		int floor_row = floor(src_row), ceil_row = ceil(src_row);
		int floor_col = floor(src_col), ceil_col = ceil(src_col);
		float ratio_row = src_row - floor_row, ratio_col = src_col - floor_col;
		dst.at<Vec3b>(dst_row, dst_col) = (1 - ratio_row) * (1 - ratio_col) * src.at<Vec3b>(floor_row, floor_col) +
			ratio_row * (1 - ratio_col) * src.at<Vec3b>(ceil_row, floor_col) +
			(1 - ratio_row) * ratio_col * src.at<Vec3b>(floor_row, ceil_col) +
			ratio_row * ratio_col * src.at<Vec3b>(ceil_row, ceil_col);
	}
	else if (src_row < 0 && src_col >= 0 && src_col <= src.cols - 1) {
		int floor_col = floor(src_col), ceil_col = ceil(src_col);
		float ratio_col = src_col - floor_col;
		dst.at<Vec3b>(dst_row, dst_col) = (1 - ratio_col) * src.at<Vec3b>(0, floor_col) +
			ratio_col * src.at<Vec3b>(0, ceil_col);
	}
	else if (src_row > src.rows - 1 && src_col >= 0 && src_col <= src.cols - 1) {
		int floor_col = floor(src_col), ceil_col = ceil(src_col);
		float ratio_col = src_col - floor_col;
		dst.at<Vec3b>(dst_row, dst_col) = (1 - ratio_col) * src.at<Vec3b>(src.rows - 1, floor_col) +
			ratio_col * src.at<Vec3b>(src.rows - 1, ceil_col);
	}
	else if (src_row >= 0 && src_row <= src.rows - 1 && src_col < 0) {
		int floor_row = floor(src_row), ceil_row = ceil(src_row);
		float ratio_row = src_row - floor_row;
		dst.at<Vec3b>(dst_row, dst_col) = (1 - ratio_row) * src.at<Vec3b>(floor_row, 0) +
			ratio_row * src.at<Vec3b>(ceil_row, 0);
	}
	else if (src_row >= 0 && src_row <= src.rows - 1 && src_col > src.cols - 1) {
		int floor_row = floor(src_row), ceil_row = ceil(src_row);
		float ratio_row = src_row - floor_row;
		dst.at<Vec3b>(dst_row, dst_col) = (1 - ratio_row) * src.at<Vec3b>(floor_row, src.cols - 1) +
			ratio_row * src.at<Vec3b>(ceil_row, src.cols - 1);
	}
	else if (src_row < 0 && src_col < 0) {
		dst.at<Vec3b>(dst_row, dst_col) = src.at<Vec3b>(0, 0);
	}
	else if (src_row > src.rows - 1 && src_col < 0) {
		dst.at<Vec3b>(dst_row, dst_col) = src.at<Vec3b>(src.rows - 1, 0);
	}
	else if (src_row < 0 && src_col > src.cols - 1) {
		dst.at<Vec3b>(dst_row, dst_col) = src.at<Vec3b>(0, src.cols - 1);
	}
	else if (src_row > src.rows - 1 && src_col > src.cols - 1) {
		dst.at<Vec3b>(dst_row, dst_col) = src.at<Vec3b>(src.rows - 1, src.cols - 1);
	}
	return 0;
}

int rectify_orientation(const Mat& src, Mat& dst, int row_pole, int col_pole) {
	if (src.cols != 2 * src.rows) {
		cerr << "Error: aspect ratio is not 2:1!" << endl;
		return -1;
	}
	if (row_pole < 0 || row_pole >= src.rows || col_pole < 0 || col_pole >= src.cols) {
		cerr << "Error: new pole coordinates out of range!" << endl;
		return -1;
	}
	dst = Mat(Size(src.cols, src.rows), CV_8UC3);
	float pi = 3.1415927, res = (float)src.rows;
	float theta_pole = row_pole * pi / res, phi_pole = col_pole * pi / res;
	// Compute rotation axis, angle, and matrix
	float nx = -sin(phi_pole), ny = cos(phi_pole), nz = 0, rotang = -theta_pole;
	Matx<float, 3, 3> R(nx*nx*(1 - cos(rotang)) + cos(rotang),
		nx*ny*(1 - cos(rotang)) + nz * sin(rotang),
		nx*nz*(1 - cos(rotang)) - ny * sin(rotang),
		nx*ny*(1 - cos(rotang)) - nz * sin(rotang),
		ny*ny*(1 - cos(rotang)) + cos(rotang),
		ny*nz*(1 - cos(rotang)) + nx * sin(rotang),
		nx*nz*(1 - cos(rotang)) + ny * sin(rotang),
		nz*ny*(1 - cos(rotang)) - nx * sin(rotang),
		nz*nz*(1 - cos(rotang)) + cos(rotang));
	Matx<float, 3, 3> R_inv = R.inv();
	for (int dst_row = 0; dst_row < dst.rows; dst_row++) {
		for (int dst_col = 0; dst_col < dst.cols; dst_col++) {
			float dst_theta = dst_row / res * pi;
			float dst_phi = dst_col / res * pi;
			Vec3f dst_xyz(sin(dst_theta) * cos(dst_phi), sin(dst_theta) * sin(dst_phi), cos(dst_theta));
			Vec3f src_xyz = R_inv * dst_xyz;
			float src_theta = acos(src_xyz[2]);
			float src_phi = atan2(src_xyz[1], src_xyz[0]) + pi;
			float src_row = src_theta * res / pi;
			float src_col = src_phi * res / pi;
			bilinear_interpolation(src, dst, src_row, src_col, dst_row, dst_col);
		}
	}
	return 0;
}

int panorama_to_cube(const Mat& src, vector<Mat>& dst, int cube_res, Mat& cutline) {
	if (src.cols != 2 * src.rows) {
		cerr << "Error: aspect ratio is not 2:1!" << endl;
		return -1;
	}
	if (cube_res <= 0) {
		cerr << "Error: wrong cube specification!" << endl;
		return -1;
	}
	float pi = 3.1415927, res = (float)src.rows;
	cutline = src.clone();
	dst.clear();
	for (int i = 0; i < 6; i++) {
		dst.push_back(Mat(Size(cube_res, cube_res), CV_8UC3));
		for (int dst_row = 0; dst_row < cube_res; dst_row++) {
			for (int dst_col = 0; dst_col < cube_res; dst_col++) {
				float cube_x, cube_y, cube_z;
				switch (i) {
				// Top
				case 0:
					cube_x = (2 * dst_row / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_y = (2 * dst_col / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_z = sqrt(3) / 3.0;
					break;
				// Front
				case 1:
					cube_x = sqrt(3) / 3.0;
					cube_y = (2 * dst_col / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_z = -(2 * dst_row / (float)cube_res - 1) * sqrt(3) / 3.0;
					break;
				// Right
				case 2:
					cube_x = -(2 * dst_col / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_y = sqrt(3) / 3.0;
					cube_z = -(2 * dst_row / (float)cube_res - 1) * sqrt(3) / 3.0;
					break;
				// Back
				case 3:
					cube_x = -sqrt(3) / 3.0;
					cube_y = -(2 * dst_col / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_z = -(2 * dst_row / (float)cube_res - 1) * sqrt(3) / 3.0;
					break;
				// Left
				case 4:
					cube_x = (2 * dst_col / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_y = -sqrt(3) / 3.0;
					cube_z = -(2 * dst_row / (float)cube_res - 1) * sqrt(3) / 3.0;
					break;
				// Bottom
				case 5:
					cube_x = -(2 * dst_row / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_y = (2 * dst_col / (float)cube_res - 1) * sqrt(3) / 3.0;
					cube_z = -sqrt(3) / 3.0;
					break;
				default:
					break;
				}
				Vec3f cube_p(cube_x, cube_y, cube_z);
				Vec3f sphere_p = cube_p / norm(cube_p);
				float sphere_theta = acos(sphere_p[2]);
				float sphere_phi = atan2(sphere_p[1], sphere_p[0]) + pi;
				float src_row = sphere_theta * res / pi;
				float src_col = sphere_phi * res / pi;
				bilinear_interpolation(src, dst[i], src_row, src_col, dst_row, dst_col);
				// Render cutline
				if (dst_row == 0 || dst_row == cube_res - 1 || dst_col == 0 || dst_col == cube_res - 1) {
					if (src_row > 0 && src_row < src.rows && src_col > 0 && src_col < src.cols) {
						cutline.at<Vec3b>(src_row, src_col)[0] = 0;
						cutline.at<Vec3b>(src_row, src_col)[1] = 0;
						cutline.at<Vec3b>(src_row, src_col)[2] = 255;
					}
				}
			}
		}
	}
	return 0;
}

int panorama_to_cuboid(const Mat& src, vector<Mat>& dst, int cuboid_x_res, int cuboid_y_res, int cuboid_z_res, Mat& cutline) {
	if (src.cols != 2 * src.rows) {
		cerr << "Error: aspect ratio is not 2:1!" << endl;
		return -1;
	}
	if (cuboid_x_res <= 0 || cuboid_y_res <= 0 || cuboid_z_res <= 0) {
		cerr << "Error: wrong cuboid specification!" << endl;
		return -1;
	}
	float ratio_yx = cuboid_y_res / (float)cuboid_x_res;
	float ratio_zx = cuboid_z_res / (float)cuboid_x_res;
	float ratio_xd = sqrt(1 + ratio_yx * ratio_yx + ratio_zx * ratio_zx) / (1 + ratio_yx * ratio_yx + ratio_zx * ratio_zx);
	float pi = 3.1415927, res = (float)src.rows;
	cutline = src.clone();
	dst.clear();
	for (int i = 0; i < 6; i++) {
		switch (i) {
		// Top
		case 0: dst.push_back(Mat(Size(cuboid_y_res, cuboid_x_res), CV_8UC3)); break;
		// Front
		case 1: dst.push_back(Mat(Size(cuboid_y_res, cuboid_z_res), CV_8UC3)); break;
		// Right
		case 2: dst.push_back(Mat(Size(cuboid_x_res, cuboid_z_res), CV_8UC3)); break;
		// Back
		case 3: dst.push_back(Mat(Size(cuboid_y_res, cuboid_z_res), CV_8UC3)); break;
		// Left
		case 4: dst.push_back(Mat(Size(cuboid_x_res, cuboid_z_res), CV_8UC3)); break;
		// Bottom
		case 5: dst.push_back(Mat(Size(cuboid_y_res, cuboid_x_res), CV_8UC3)); break;
		default: break;
		}
		for (int dst_row = 0; dst_row < dst[i].rows; dst_row++) {
			for (int dst_col = 0; dst_col < dst[i].cols; dst_col++) {
				float cuboid_x, cuboid_y, cuboid_z;
				switch (i) {
				// Top
				case 0:
					cuboid_x = (2 * dst_row / (float)dst[i].rows - 1) * ratio_xd;
					cuboid_y = (2 * dst_col / (float)dst[i].cols - 1) * ratio_yx * ratio_xd;
					cuboid_z = ratio_zx * ratio_xd;
					break;
				// Front
				case 1:
					cuboid_x = ratio_xd;
					cuboid_y = (2 * dst_col / (float)dst[i].cols - 1) * ratio_yx * ratio_xd;
					cuboid_z = -(2 * dst_row / (float)dst[i].rows - 1) * ratio_zx * ratio_xd;
					break;
				// Right
				case 2:
					cuboid_x = -(2 * dst_col / (float)dst[i].cols - 1) * ratio_xd;
					cuboid_y = ratio_yx * ratio_xd;
					cuboid_z = -(2 * dst_row / (float)dst[i].rows - 1) * ratio_zx * ratio_xd;
					break;
				// Back
				case 3:
					cuboid_x = -ratio_xd;
					cuboid_y = -(2 * dst_col / (float)dst[i].cols - 1) * ratio_yx * ratio_xd;
					cuboid_z = -(2 * dst_row / (float)dst[i].rows - 1) * ratio_zx * ratio_xd;
					break;
				// Left
				case 4:
					cuboid_x = (2 * dst_col / (float)dst[i].cols - 1) * ratio_xd;
					cuboid_y = -ratio_yx * ratio_xd;
					cuboid_z = -(2 * dst_row / (float)dst[i].rows - 1) * ratio_zx * ratio_xd;
					break;
				// Bottom
				case 5:
					cuboid_x = -(2 * dst_row / (float)dst[i].rows - 1) * ratio_xd;
					cuboid_y = (2 * dst_col / (float)dst[i].cols - 1) * ratio_yx * ratio_xd;
					cuboid_z = -ratio_zx * ratio_xd;
					break;
				default:
					break;
				}
				Vec3f cuboid_p(cuboid_x, cuboid_y, cuboid_z);
				Vec3f sphere_p = cuboid_p / norm(cuboid_p);
				float sphere_theta = acos(sphere_p[2]);
				float sphere_phi = atan2(sphere_p[1], sphere_p[0]) + pi;
				float src_row = sphere_theta * res / pi;
				float src_col = sphere_phi * res / pi;
				bilinear_interpolation(src, dst[i], src_row, src_col, dst_row, dst_col);
				// Render cutline
				if (dst_row == 0 || dst_row == dst[i].rows - 1 || dst_col == 0 || dst_col == dst[i].cols - 1) {
					if (src_row > 0 && src_row < src.rows && src_col > 0 && src_col < src.cols) {
						cutline.at<Vec3b>(src_row, src_col)[0] = 0;
						cutline.at<Vec3b>(src_row, src_col)[1] = 0;
						cutline.at<Vec3b>(src_row, src_col)[2] = 255;
					}
				}
			}
		}
	}
	return 0;
}

int main() {
	Mat old_pano = imread("panorama.jpg");
	/*Mat shifted_pano(Size(old_pano.cols, old_pano.rows), CV_8UC3);
	int shift = 500;
	old_pano(Rect(shift, 0, old_pano.cols - shift, old_pano.rows)).copyTo(shifted_pano(Rect(0, 0, old_pano.cols - shift, old_pano.rows)));
	old_pano(Rect(0, 0, shift, old_pano.rows)).copyTo(shifted_pano(Rect(old_pano.cols - shift, 0, shift, old_pano.rows)));
	imwrite("shift.jpg", shifted_pano);*/
	// Mat new_pano;
	// rectify_orientation(old_pano, new_pano, 1250, 2500);
	vector<Mat> cube_faces;
	Mat cutline;
	// panorama_to_cube(old_pano, cube_faces, 1000, cutline);
	panorama_to_cuboid(old_pano, cube_faces, 1000, 1200, 800, cutline);
	imwrite("a0.jpg", cube_faces[0]);
	imwrite("a1.jpg", cube_faces[1]);
	imwrite("a2.jpg", cube_faces[2]);
	imwrite("a3.jpg", cube_faces[3]);
	imwrite("a4.jpg", cube_faces[4]);
	imwrite("a5.jpg", cube_faces[5]);
	imwrite("a6.jpg", cutline);
	return 0;
}