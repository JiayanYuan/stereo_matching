/************************************************
* Copyright 2014 Baidu Inc. All Rights Reserved.
* Author: Luo Yao (luoyao@baidu.com)
* File: imageQualityChecker.cpp
* Date: 18-6-28 上午10:28
************************************************/

#include "imageSkyDetector.h"

#include <fstream>
#include <chrono>


namespace sky_detector {

/***
 * 类复制构造函数
 * @param _SkyAreaDetector
 */
SkyAreaDetector::SkyAreaDetector(const SkyAreaDetector &_SkyAreaDetector) {
    this->f_thres_sky_max = _SkyAreaDetector.f_thres_sky_max;
    this->f_thres_sky_min = _SkyAreaDetector.f_thres_sky_min;
    this->f_thres_sky_search_step = _SkyAreaDetector.f_thres_sky_search_step;
    this->f_thres_sky_width = _SkyAreaDetector.f_thres_sky_width;
}
/***
 * 类复制构造函数
 * @param _SkyAreaDetector
 * @return
 */
SkyAreaDetector& SkyAreaDetector::operator=(const SkyAreaDetector &_SkyAreaDetector) {
    this->f_thres_sky_max = _SkyAreaDetector.f_thres_sky_max;
    this->f_thres_sky_min = _SkyAreaDetector.f_thres_sky_min;
    this->f_thres_sky_search_step = _SkyAreaDetector.f_thres_sky_search_step;
    this->f_thres_sky_width = _SkyAreaDetector.f_thres_sky_width;

    return *this;
}

/***
 * 读取图像文件
 * @param image_file_path
 * @return
 */
bool SkyAreaDetector::load_image(const std::string &image_file_path,
                                 int width, int height) {

    _src_img = cv::imread(image_file_path/*, CV_LOAD_IMAGE_UNCHANGED*/);

    if (_src_img.channels() == 1)
        cv::cvtColor(_src_img, _src_img, CV_GRAY2BGR);

//    cv::imwrite("/home/hunterlew/tmp.png", _src_img);

    std::cout << "loading " << image_file_path << std::endl;
    std::cout << "image size: " << _src_img.size << std::endl;
    cv::resize(_src_img, _src_img, cv::Size(height, width));

//    assert (_src_img.channels() == 3);

    if (_src_img.empty() || !_src_img.data) {
        std::cout << "图像文件: " << image_file_path << "读取失败" << std::endl;
        return false;
    }

    return true;
}

/***
 * 提取图像天空区域
 * @param bgrimg
 * @param skybinaryimg
 * @param horizonline
 * @return
 */
bool SkyAreaDetector::extract_sky(const cv::Mat &src_image, cv::Mat &sky_mask) {

//    int image_height = src_image.size[0];
//    int image_width = src_image.size[1];

    std::vector<int> sky_border_optimal = extract_border_optimal(src_image);

    check_sky_border_by_gray_value(src_image, sky_border_optimal);

    sky_mask = make_sky_mask(src_image, sky_border_optimal);

    return true;
}

void SkyAreaDetector::check_sky_border_by_gray_value(const cv::Mat &src_image,
                                                     std::vector<int> &sky_border_optimal) {
    int image_width = src_image.size[1];

//    assert (image_width == sky_border_optimal.size());

    cv::Mat gray_image;
    if (_src_img.channels() != 1)
        cv::cvtColor(_src_img, gray_image, CV_BGR2GRAY);

    for (int i=0; i<image_width; ++i)
    {
        int border = sky_border_optimal[i];

        for (int j=0; j<border; ++j)
        {
            uchar val = gray_image.at<uchar>(j, i);
            if (val < 128)
            {
                sky_border_optimal[i] = -1;
                break;
            }
        }

        if (border != -1
            && (i>1 && sky_border_optimal[i-1] == -1)
            && (i<image_width-1 && sky_border_optimal[i+1] == -1))
        {
            sky_border_optimal[i] = -1;
        }
    }

    // remove those with short width
    int p = -1;
    int q = -1;
    for (int i=0; i<image_width; ++i)
    {
        int border = sky_border_optimal[i];

        if (border != -1)  // find left valid border [
        {
            p = i;

            if (i == image_width - 1)
                q = image_width;

            for (int j=i+1; j<image_width; ++j)
            {
                int border_tmp = sky_border_optimal[j];

                if (border_tmp == -1 || j == image_width - 1)  // find right valid border )
                {
                    q = j;
                    if (j == image_width - 1)
                        q = image_width;

                    break;
                }
            }

//            assert (q > p);

            if (q > p)
                printf("find valid len: %d\n", q-p);

            if (q > p && q - p < f_thres_sky_width)
            {
                for (int j=p; j<q; ++j)
                    sky_border_optimal[j] = -1;
            }

            i = q;
        }
    }
}

void SkyAreaDetector::detect(const cv::Mat &img,
                             const std::string file_name,
                             cv::Mat &sky_label) {

    _src_img = img.clone();
    if (img.channels() == 1)
        cv::cvtColor(_src_img, _src_img, CV_GRAY2BGR);

    int img_height = _src_img.rows;
    int img_width = _src_img.cols;
    sky_label = cv::Mat::zeros(img_height, img_width, CV_8UC1);  // 0-non-sky, 255-sky
    std::cout << "image size: " << img_height << ", " << img_width << std::endl;

    // 提取图像天空区域
    cv::Mat sky_mask;

    auto start_t = std::chrono::high_resolution_clock::now();

    if (extract_sky(_src_img, sky_mask)) {

        // 制作掩码输出
        _src_img.setTo(cv::Scalar(0, 0, 255), sky_mask);

        cv::imwrite(file_name, _src_img);

        sky_label.setTo(255, sky_mask);

        auto end_t = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> cost_time = end_t - start_t;

        std::cout << "---- " << "sky detector cost" << " ---- "
                  << cost_time.count() << "s" << std::endl;
    } else {

        cv::imwrite(file_name, _src_img);

        std::cout << "---- " << "sky detector cost" << " ---- "
                  << "Null s" << std::endl;
    }
}

/***
 * 提取图像梯度信息
 * @param src_image
 * @param gradient_image
 */
void SkyAreaDetector::extract_image_gradient(const cv::Mat &src_image, cv::Mat &gradient_image) {
    // 转灰度图像
    cv::Mat gray_image;
    cv::cvtColor(src_image, gray_image, cv::COLOR_BGR2GRAY);
    // Sobel算子提取图像梯度信息
    cv::Mat x_gradient;
    cv::Sobel(gray_image, x_gradient, CV_64F, 1, 0);
    cv::Mat y_gradient;
    cv::Sobel(gray_image, y_gradient, CV_64F, 0, 1);
    // 计算梯度信息图
    cv::Mat gradient;
    cv::pow(x_gradient, 2, x_gradient);
    cv::pow(y_gradient, 2, y_gradient);
    cv::add(x_gradient, y_gradient, gradient);
    cv::sqrt(gradient, gradient);

    gradient_image = gradient;

}

/***
 * 计算天空边界线
 * @param src_image
 * @return
 */
std::vector<int> SkyAreaDetector::extract_border_optimal(const cv::Mat &src_image) {

    // 提取梯度信息图
    cv::Mat gradient_info_map;
    extract_image_gradient(src_image, gradient_info_map);

//    cv::imwrite("/home/hunterlew/grad.png", gradient_info_map);

    int n = static_cast<int>(std::floor((f_thres_sky_max - f_thres_sky_min)
                                        / f_thres_sky_search_step)) + 1;

    int image_height = gradient_info_map.size[0];
    int image_width = gradient_info_map.size[1];

    std::vector<int> border_opt(image_width, image_height - 1);
    std::vector<int> b_tmp(image_width, image_height - 1);

    double jn_max = 0.0;

    double step = (std::floor((f_thres_sky_max - f_thres_sky_min) / n) - 1);

    for (int k = 1; k < n + 1; ++k) {
        double t = f_thres_sky_min + step * (k - 1);

        extract_border(b_tmp, gradient_info_map, t, src_image);
        double jn = calculate_sky_energy(b_tmp, src_image);

//        printf("%d: %.20lf\n", k, jn);

        if (std::isinf(jn)) {
            std::cout << "Jn is -inf" << std::endl;
        }

        if (jn > jn_max) {
            jn_max = jn;
            border_opt = b_tmp;
        }
    }

    return border_opt;
}

/***
 * 计算天空边界线
 * @param gradient_info_map
 * @param thresh
 * @return
 */
void SkyAreaDetector::extract_border(std::vector<int> &border,
                                     const cv::Mat &gradient_info_map,
                                     double thresh,
                                     const cv::Mat &src_image) {
    int image_height = gradient_info_map.size[0];
    int image_width = gradient_info_map.size[1];

//    assert (image_width == border.size());

#pragma omp parallel for
    for (int col = 0; col < image_width; ++col) {
        int row_index = -1;
        for (int row = 0; row < image_height; ++row) {
            row_index = row;

            if (gradient_info_map.at<double>(row, col) > thresh) {

                // for gray image
                if (src_image.at<cv::Vec3b>(row, col)[0] == src_image.at<cv::Vec3b>(row, col)[1]
                    && src_image.at<cv::Vec3b>(row, col)[0] == src_image.at<cv::Vec3b>(row, col)[2]) {

                    double grad_y = (2*src_image.at<cv::Vec3b>(row+1, col)[0]
                            + src_image.at<cv::Vec3b>(row+1, col+1)[0]
                            + src_image.at<cv::Vec3b>(row+1, col-1)[0])
                            - (2*src_image.at<cv::Vec3b>(row-1, col)[0]
                            + src_image.at<cv::Vec3b>(row-1, col+1)[0]
                            + src_image.at<cv::Vec3b>(row-1, col-1)[0]);

                    if (grad_y > 0)  // down white, up black
                        border[col] = -1;
                    else
                        border[col] = row;

                    break;
                }
                else
                {
                    border[col] = row;
                    break;
                }
            }

            if (row_index >= image_height / 2)
            {
                border[col] = -1;
                break;
            }
        }

        if (row_index >= image_height / 2)
            border[col] = -1;

        if (row_index <= 5) {
            border[col] = -1;
        }
    }

//    for (int h : border)
//        std::cout << h << ", ";
//    std::cout << std::endl;
}

/***
 * 改善天空边界线
 * @param border
 * @param src_image
 * @return
 */
std::vector<int> SkyAreaDetector::refine_border(const std::vector<int> &border,
        const cv::Mat &src_image) {

    int image_height = src_image.size[0];
    int image_width = src_image.size[1];

    /*
    // 制作天空图像掩码和地面图像掩码
    cv::Mat sky_mask = make_sky_mask(src_image, border, 1);
    cv::Mat ground_mask = make_sky_mask(src_image, border, 0);

    // 扣取天空图像和地面图像
    cv::Mat sky_image = cv::Mat::zeros(image_height, image_width, CV_8UC3);
    cv::Mat ground_image = cv::Mat::zeros(image_height, image_width, CV_8UC3);
    src_image.copyTo(sky_image, sky_mask);
    src_image.copyTo(ground_image, ground_mask);

    // 计算天空和地面图像协方差矩阵
    int ground_non_zeros_nums = cv::countNonZero(ground_mask);
    int sky_non_zeros_nums = cv::countNonZero(sky_mask);

    cv::Mat ground_image_non_zero = cv::Mat::zeros(ground_non_zeros_nums, 3, CV_8UC1);
    cv::Mat sky_image_non_zero = cv::Mat::zeros(sky_non_zeros_nums, 3, CV_8UC1);

    int row_index = 0;
    for (int col = 0; col < ground_image.cols; ++col) {
        for (int row = 0; row < ground_image.rows; ++row) {
            if (ground_image.at<cv::Vec3b>(row, col)[0] == 0 &&
                    ground_image.at<cv::Vec3b>(row, col)[1] == 0 &&
                    ground_image.at<cv::Vec3b>(row, col)[2] == 0) {
                continue;
            } else {
                cv::Vec3b intensity = ground_image.at<cv::Vec3b>(row, col);
                ground_image_non_zero.at<uchar>(row_index, 0) = intensity[0];
                ground_image_non_zero.at<uchar>(row_index, 1) = intensity[1];
                ground_image_non_zero.at<uchar>(row_index, 2) = intensity[2];
                row_index++;
            }
        }
    }

    row_index = 0;
    for (int col = 0; col < sky_image.cols; ++col) {
        for (int row = 0; row < sky_image.rows; ++row) {
            if (sky_image.at<cv::Vec3b>(row, col)[0] == 0 &&
                    sky_image.at<cv::Vec3b>(row, col)[1] == 0 &&
                    sky_image.at<cv::Vec3b>(row, col)[2] == 0) {
                continue;
            } else {
                cv::Vec3b intensity = sky_image.at<cv::Vec3b>(row, col);
                sky_image_non_zero.at<uchar>(row_index, 0) = intensity[0];
                sky_image_non_zero.at<uchar>(row_index, 1) = intensity[1];
                sky_image_non_zero.at<uchar>(row_index, 2) = intensity[2];
                row_index++;
            }
        }
    }
    */

    static std::vector<cv::Vec3b> sky_pixels(image_width*image_height);
    static std::vector<cv::Vec3b> ground_pixels(image_width*image_height);

    int sky_non_zeros_nums = 0;
    int ground_non_zeros_nums = 0;

    for (int row = 0; row < image_height; ++row) {
        const cv::Vec3b *ptr_src = src_image.ptr<cv::Vec3b>(row);

        for (int col = 0; col < image_width; ++col) {

            const cv::Vec3b &p = ptr_src[col];
            if (p[0] == 0 && p[1] == 0 && p[2] == 0)
                continue;

            if (row <= border[col]) {
                sky_pixels[sky_non_zeros_nums] = p;
                ++sky_non_zeros_nums;
            }
            else {
                ground_pixels[ground_non_zeros_nums] = p;
                ++ground_non_zeros_nums;
            }
        }
    }

    static cv::Mat sky_image_non_zero, ground_image_non_zero;
    sky_image_non_zero.create(sky_non_zeros_nums, 3, CV_8UC1);
    ground_image_non_zero.create(ground_non_zeros_nums, 3, CV_8UC1);

    #pragma omp parallel for
    for (int i=0; i<sky_non_zeros_nums; ++i)
    {
        const cv::Vec3b &p = sky_pixels[i];
        uchar *ptr_src = sky_image_non_zero.ptr<uchar>(i);
        ptr_src[0] = p[0];
        ptr_src[1] = p[1];
        ptr_src[2] = p[2];
    }

    #pragma omp parallel for
    for (int i=0; i<ground_non_zeros_nums; ++i)
    {
        const cv::Vec3b &p = ground_pixels[i];
        uchar *ptr_src = ground_image_non_zero.ptr<uchar>(i);
        ptr_src[0] = p[0];
        ptr_src[1] = p[1];
        ptr_src[2] = p[2];
    }

    // k均值聚类调整天空区域边界
    cv::Mat sky_image_float;
    sky_image_non_zero.convertTo(sky_image_float, CV_32FC1);
    cv::Mat labels;
    cv::kmeans(sky_image_float, 2, labels,
               cv::TermCriteria(CV_TERMCRIT_EPS + CV_TERMCRIT_ITER, 10, 1.0),
               10, cv::KMEANS_RANDOM_CENTERS);
    int label_1_nums = cv::countNonZero(labels);
    int label_0_nums = labels.rows - label_1_nums;

    cv::Mat sky_label_1_image = cv::Mat::zeros(label_1_nums, 3, CV_8UC1);
    cv::Mat sky_label_0_image = cv::Mat::zeros(label_0_nums, 3, CV_8UC1);

    int row_index = 0;
    for (int row = 0; row < labels.rows; ++row) {
        if (labels.at<float>(row, 0) == 0.0) {
            sky_label_0_image.at<uchar>(row_index, 0) = sky_image_non_zero.at<uchar>(row, 0);
            sky_label_0_image.at<uchar>(row_index, 1) = sky_image_non_zero.at<uchar>(row, 1);
            sky_label_0_image.at<uchar>(row_index, 2) = sky_image_non_zero.at<uchar>(row, 2);
            row_index++;
        }
    }
    row_index = 0;
    for (int row = 0; row < labels.rows; ++row) {
        if (labels.at<float>(row, 0) == 1.0) {
            sky_label_1_image.at<uchar>(row_index, 0) = sky_image_non_zero.at<uchar>(row, 0);
            sky_label_1_image.at<uchar>(row_index, 1) = sky_image_non_zero.at<uchar>(row, 1);
            sky_label_1_image.at<uchar>(row_index, 2) = sky_image_non_zero.at<uchar>(row, 2);
            row_index++;
        }
    }

    cv::Mat sky_covar_1;
    cv::Mat sky_mean_1;
    cv::calcCovarMatrix(sky_label_1_image, sky_covar_1,
                        sky_mean_1, CV_COVAR_ROWS | CV_COVAR_NORMAL | CV_COVAR_SCALE);
    cv::Mat ic_s1;
    cv::invert(sky_covar_1, ic_s1, cv::DECOMP_SVD);

    cv::Mat sky_covar_0;
    cv::Mat sky_mean_0;
    cv::calcCovarMatrix(sky_label_0_image, sky_covar_0,
                        sky_mean_0, CV_COVAR_ROWS | CV_COVAR_NORMAL | CV_COVAR_SCALE);
    cv::Mat ic_s0;
    cv::invert(sky_covar_0, ic_s0, cv::DECOMP_SVD);

    cv::Mat ground_covar;
    cv::Mat ground_mean;
    cv::calcCovarMatrix(ground_image_non_zero, ground_covar,
                        ground_mean, CV_COVAR_ROWS | CV_COVAR_NORMAL | CV_COVAR_SCALE);
    cv::Mat ic_g;
    cv::invert(ground_covar, ic_g, cv::DECOMP_SVD);

    cv::Mat sky_mean;
    cv::Mat sky_covar;
    cv::Mat ic_s;
    if (cv::Mahalanobis(sky_mean_0, ground_mean, ic_s0) > cv::Mahalanobis(sky_mean_1, ground_mean, ic_s1)) {
        sky_mean = sky_mean_0;
        sky_covar = sky_covar_0;
        ic_s = ic_s0;
    } else {
        sky_mean = sky_mean_1;
        sky_covar = sky_covar_1;
        ic_s = ic_s1;
    }

    std::vector<int> border_new(border.size(), 0);
    for (size_t col = 0; col < border.size(); ++col) {
        double cnt = 0.0;
        for (int row = 0; row < border[col]; ++row) {
            // 计算原始天空区域的区域每个像素点和修正过后的天空区域的每个点的Mahalanobis距离
            cv::Mat ori_pix;
            src_image.row(row).col(static_cast<int>(col)).convertTo(ori_pix, sky_mean.type());
            ori_pix = ori_pix.reshape(1, 1);
            double distance_s = cv::Mahalanobis(ori_pix,
                                                sky_mean, ic_s);
            double distance_g = cv::Mahalanobis(ori_pix,
                                                ground_mean, ic_g);

            if (distance_s < distance_g) {
                cnt++;
            }
        }
        if (cnt < (border[col] / 2)) {
            border_new[col] = 0;
        } else {
            border_new[col] = border[col];
        }
    }

    return border_new;
}

/***
 * 计算天空图像能量函数
 * @param border
 * @param src_image
 * @return
 */
double SkyAreaDetector::calculate_sky_energy(const std::vector<int> &border,
        const cv::Mat &src_image) {

    int image_height = src_image.size[0];
    int image_width = src_image.size[1];

    /*
    // 制作天空图像掩码和地面图像掩码
    cv::Mat sky_mask = make_sky_mask(src_image, border, 1);
    cv::Mat ground_mask = make_sky_mask(src_image, border, 0);

    // 扣取天空图像和地面图像
    cv::Mat sky_image = cv::Mat::zeros(image_height, image_width, CV_8UC3);
    cv::Mat ground_image = cv::Mat::zeros(image_height, image_width, CV_8UC3);
    src_image.copyTo(sky_image, sky_mask);
    src_image.copyTo(ground_image, ground_mask);

    // 计算天空和地面图像协方差矩阵
    int ground_non_zeros_nums = cv::countNonZero(ground_mask);
    int sky_non_zeros_nums = cv::countNonZero(sky_mask);

    if (ground_non_zeros_nums == 0 || sky_non_zeros_nums == 0) {
        return std::numeric_limits<double>::min();
    }

    cv::Mat ground_image_non_zero = cv::Mat::zeros(ground_non_zeros_nums, 3, CV_8UC1);
    cv::Mat sky_image_non_zero = cv::Mat::zeros(sky_non_zeros_nums, 3, CV_8UC1);

    assert (ground_image.cols == image_width && ground_image.rows == image_height);
    assert (ground_image.cols == sky_image.cols && ground_image.rows == sky_image.rows);

    int row_index = 0;
    int row_index_beta = 0;
    for (int col = 0; col < image_width; ++col) {
        for (int row = 0; row < image_height; ++row) {
            if (ground_image.at<cv::Vec3b>(row, col)[0] != 0 ||
                    ground_image.at<cv::Vec3b>(row, col)[1] != 0 ||
                    ground_image.at<cv::Vec3b>(row, col)[2] != 0) {

                cv::Vec3b intensity = ground_image.at<cv::Vec3b>(row, col);
                ground_image_non_zero.at<uchar>(row_index, 0) = intensity[0];
                ground_image_non_zero.at<uchar>(row_index, 1) = intensity[1];
                ground_image_non_zero.at<uchar>(row_index, 2) = intensity[2];
                row_index++;
            }

            if (sky_image.at<cv::Vec3b>(row, col)[0] != 0 ||
                    sky_image.at<cv::Vec3b>(row, col)[1] != 0 ||
                    sky_image.at<cv::Vec3b>(row, col)[2] != 0) {

                cv::Vec3b intensity = sky_image.at<cv::Vec3b>(row, col);
                sky_image_non_zero.at<uchar>(row_index_beta, 0) = intensity[0];
                sky_image_non_zero.at<uchar>(row_index_beta, 1) = intensity[1];
                sky_image_non_zero.at<uchar>(row_index_beta, 2) = intensity[2];
                row_index_beta++;
            }
        }
    }
    */

    static std::vector<cv::Vec3b> sky_pixels(image_width*image_height);
    static std::vector<cv::Vec3b> ground_pixels(image_width*image_height);

    int sky_non_zeros_nums = 0;
    int ground_non_zeros_nums = 0;

    for (int row = 0; row < image_height; ++row) {
        const cv::Vec3b *ptr_src = src_image.ptr<cv::Vec3b>(row);

        for (int col = 0; col < image_width; ++col) {

            const cv::Vec3b &p = ptr_src[col];
            if (p[0] == 0 && p[1] == 0 && p[2] == 0)
                continue;

            if (row < border[col]) {
                sky_pixels[sky_non_zeros_nums] = p;
                ++sky_non_zeros_nums;
            }
            else {
                ground_pixels[ground_non_zeros_nums] = p;
                ++ground_non_zeros_nums;
            }
        }
    }

    if (ground_non_zeros_nums == 0 || sky_non_zeros_nums == 0) {
        return std::numeric_limits<double>::min();
    }

    static cv::Mat sky_image_non_zero, ground_image_non_zero;
    sky_image_non_zero.create(sky_non_zeros_nums, 3, CV_8UC1);
    ground_image_non_zero.create(ground_non_zeros_nums, 3, CV_8UC1);

    #pragma omp parallel for
    for (int i=0; i<sky_non_zeros_nums; ++i)
    {
        const cv::Vec3b &p = sky_pixels[i];
        uchar *ptr_src = sky_image_non_zero.ptr<uchar>(i);
        ptr_src[0] = p[0];
        ptr_src[1] = p[1];
        ptr_src[2] = p[2];
    }

    #pragma omp parallel for
    for (int i=0; i<ground_non_zeros_nums; ++i)
    {
        const cv::Vec3b &p = ground_pixels[i];
        uchar *ptr_src = ground_image_non_zero.ptr<uchar>(i);
        ptr_src[0] = p[0];
        ptr_src[1] = p[1];
        ptr_src[2] = p[2];
    }

    static cv::Mat ground_covar;
    static cv::Mat ground_mean;
    static cv::Mat ground_eig_vec;
    static cv::Mat ground_eig_val;

    cv::calcCovarMatrix(ground_image_non_zero, ground_covar,
                        ground_mean, CV_COVAR_ROWS | CV_COVAR_NORMAL | CV_COVAR_SCALE);
    cv::eigen(ground_covar, ground_eig_val, ground_eig_vec);

    static cv::Mat sky_covar;
    static cv::Mat sky_mean;
    static cv::Mat sky_eig_vec;
    static cv::Mat sky_eig_val;

    cv::calcCovarMatrix(sky_image_non_zero, sky_covar,
                        sky_mean, CV_COVAR_ROWS | CV_COVAR_SCALE | CV_COVAR_NORMAL);
    cv::eigen(sky_covar, sky_eig_val, sky_eig_vec);

    int para = 2; // 论文原始参数
    double ground_det = fabs(cv::determinant(ground_covar));
    double sky_det = fabs(cv::determinant(sky_covar));
    double ground_eig_det = fabs(ground_eig_val.at<double>(0,0));
    double sky_eig_det = fabs(sky_eig_val.at<double>(0.0));

//    printf("%lf, %lf, %lf, %lf\n", ground_det, sky_det, ground_eig_det, sky_eig_det);

    return 1 / ((para * sky_det + ground_det) + (para * sky_eig_det + ground_eig_det));

}

/***
 * 确定图像是否含有天空区域
 * @param border
 * @param thresh_1
 * @param thresh_2
 * @param thresh_3
 * @return
 */
bool SkyAreaDetector::has_sky_region(const std::vector<int> &border,
                                     std::vector<int> &border_diff,
                                     double thresh_1, double thresh_2,
                                     double thresh_3) {
    double border_mean = 0.0;
    for (size_t i = 0; i < border.size(); ++i) {
        border_mean += border[i];
    }
    border_mean /= border.size();

    // 如果平均天际线太小认为没有天空区域
    if (border_mean < thresh_1) {
        printf("border mean too height\n");
        return false;
    }

    assert (border.size() - 1 == border_diff.size());
    for (auto i = static_cast<int>(border.size() - 1); i >= 0; --i) {
        border_diff[i] = std::abs(border[i + 1] - border[i]);
    }

    double border_diff_mean = 0.0;
    for (auto &diff_val : border_diff) {
        border_diff_mean += diff_val;
    }
    border_diff_mean /= border_diff.size();

//    printf("%lf, %lf; thresh1 %lf, thresh2 %lf, thresh3 %lf\n", border_mean, border_diff_mean, thresh_1, thresh_2, thresh_3);

    return !(border_mean < thresh_1 || (border_diff_mean > thresh_3 && border_mean < thresh_2));
}

/***
 * 判断图像是否有部分区域为天空区域
 * @param border
 * @param thresh_1
 * @return
 */
bool SkyAreaDetector::has_partial_sky_region(const std::vector<int> &border,
                                             const std::vector<int> &border_diff,
                                             double thresh_4) {
    assert (border.size() - 1 == border_diff.size());

    for (size_t i = 0; i < border_diff.size(); ++i) {
        if (border_diff[i] > thresh_4) {
            return true;
        }
    }

    return false;
}

/***
 * 天空区域和原始图像融合图
 * @param src_image
 * @param border
 * @param sky_image
 */
void SkyAreaDetector::display_sky_region(const cv::Mat &src_image,
        const std::vector<int> &border,
        cv::Mat &sky_image) {

    int image_height = src_image.size[0];
    int image_width = src_image.size[1];
    // 制作天空图掩码
    cv::Mat sky_mask = make_sky_mask(src_image, border, 1);

    // 天空和原始图像融合
    cv::Mat sky_image_full = cv::Mat::zeros(image_height, image_width, CV_8UC3);
    sky_image_full.setTo(cv::Scalar(0, 0, 255), sky_mask);
    cv::addWeighted(src_image, 1, sky_image_full, 1, 0, sky_image);
}

/***
 * 制作天空掩码图像
 * @param src_image
 * @param border
 * @param type: 1: 天空 0: 地面
 * @return
 */
cv::Mat SkyAreaDetector::make_sky_mask(const cv::Mat &src_image,
                                       const std::vector<int> &border,
                                       int type) {
    int image_height = src_image.size[0];
    int image_width = src_image.size[1];

    cv::Mat mask = cv::Mat::zeros(image_height, image_width, CV_8UC1);

    if (type == 1) {
        for (int row = 0; row < image_height; ++row) {
            uchar *p = mask.ptr<uchar>(row);
            for (int col = 0; col < image_width; ++col) {
                if (row <= border[col]) {
                    p[col] = 255;
                }
            }
        }
    } else if (type == 0) {
        for (int row = 0; row < image_height; ++row) {
            uchar *p = mask.ptr<uchar>(row);
            for (int col = 0; col < image_width; ++col) {
                if (row > border[col]) {
                    p[col] = 255;
                }
            }
        }
    } else {
        assert(type == 0 || type == 1);
    }

    return mask;
}

}
