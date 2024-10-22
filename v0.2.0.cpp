#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>  // 用于 inet_pton 和 inet_ntop
#pragma comment(lib, "ws2_32.lib")  // 链接 ws2_32.lib
#include <stdio.h>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#define MAX_BUFFER 128

using namespace cv;
using namespace std;

// 初始化 Winsock
void init_winsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup 失败，错误代码: %d\n", result);
        exit(1);
    }
}

// 连接到 ADB 服务器的函数
int connect_to_adb(const char* server_ip, int port) {
    // 创建套接字
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("无法创建套接字\n");
        return -1;
    }

    // 设置服务器地址信息
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(server_ip);  // ADB服务器地址
    server.sin_family = AF_INET;
    server.sin_port = htons(port);  // ADB服务器端口

    // 连接到ADB服务器
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("ADB服务器连接失败\n");
        closesocket(sock);
        return -1;
    }
    printf("已连接到ADB服务器\n");

    // 发送命令：host:transport:<device_id>，以选定设备
    const char* device_cmd = "host:transport:127.0.0.1:7555";
    char device_cmd_len[5];
    snprintf(device_cmd_len, sizeof(device_cmd_len), "%04x", (unsigned int)strlen(device_cmd));  // 命令长度

    // 将命令长度和命令发送到ADB
    send(sock, device_cmd_len, 4, 0);
    send(sock, device_cmd, strlen(device_cmd), 0);

    // 接收ADB服务器响应
    char response[1024] = { 0 };
    if (recv(sock, response, sizeof(response), 0) < 0) {
        printf("未收到设备响应\n");
        return -1;
    }
    printf("设备响应: %s\n", response);

    return sock;  // 返回套接字描述符
}

// 发送Swipe命令的函数
//int send_swipe_command(int sock ,int start_x ,int start_y,int end_x,int end_y,int duration) {
//    //// 发送命令：host:transport:<device_id>，以选定设备
//    //const char* device_cmd = "host:transport:DEVICE_ID";
//    //char device_cmd_len[5];
//    //snprintf(device_cmd_len, sizeof(device_cmd_len), "%04x", (unsigned int)strlen(device_cmd));  // 命令长度
//
//    //// 将命令长度和命令发送到ADB
//    //send(sock, device_cmd_len, 4, 0);
//    //send(sock, device_cmd, strlen(device_cmd), 0);
//
//    //// 接收ADB服务器响应
//    char response[1024] = { 0 };
//    //if (recv(sock, response, sizeof(response), 0) < 0) {
//    //    printf("未收到设备响应\n");
//    //    return -1;
//    //}
//    //printf("设备响应: %s\n", response);
//
//    // 格式化Swipe命令
//    char swipe_cmd[128];  // 存储完整的 swipe 命令
//    snprintf(swipe_cmd, sizeof(swipe_cmd), "shell:input swipe %d %d %d %d %d", start_x, start_y, end_x, end_y, duration);
//
//    // 计算Swipe命令的长度
//    char swipe_cmd_len[5];
//    snprintf(swipe_cmd_len, sizeof(swipe_cmd_len), "%04x", (unsigned int)strlen(swipe_cmd));
//
//    // 将命令长度和命令发送到ADB
//    send(sock, swipe_cmd_len, 4, 0);
//    send(sock, swipe_cmd, strlen(swipe_cmd), 0);
//
//
//    // 接收ADB服务器响应
//    memset(response, 0, sizeof(response));
//    if (recv(sock, response, sizeof(response), 0) < 0) {
//        printf("未收到Swipe命令的响应\n");
//        return -1;
//    }
//    printf("Swipe命令响应: %s\n", response);
//
//    return 0;  // 成功
//}

// 比较函数的实现
double compare(const Mat& img, const Mat& temp) {
    Mat result;
    double threshold = 0.60;

    matchTemplate(img, temp, result, TM_CCOEFF_NORMED);
    double maxVal;
    minMaxLoc(result, nullptr, &maxVal, nullptr, nullptr);

    return (maxVal > threshold) ? maxVal : 0; // 使用条件运算符简化代码
}
// 定义识别数字的函数
vector<int> recognizeNumbers(string imgPath, vector<string> templatePaths) {
    Mat img = imread(imgPath);  // 读取输入图片
    Mat clone_img = img.clone(); // 原图备份
    cvtColor(img, img, COLOR_BGR2GRAY); // 图片转灰度
    adaptiveThreshold(img, img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);//自适应阈值
    Mat clone_img1 = img.clone(); // 备份二值化后的原图
    imwrite("binary_image.bmp", img);

    // 查找轮廓
    vector<vector<Point>> contours; // 存储原图轮廓
    vector<Vec4i> hierarchy;
    findContours(img, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); // 检测所有的轮廓
    //drawContours(clone_img, contours, -1, Scalar(0, 255, 0), 1, 8); // 绘制轮廓
    //imwrite("contours_image.bmp", clone_img);

    vector<Rect> validRects; // 定义存储有效外接矩形的向量
    for (const auto& contour : contours) {
        Rect boundingRect = cv::boundingRect(contour);
        double area = cv::contourArea(contour);
        float aspectRatio = (float)boundingRect.width / (float)boundingRect.height;

        // 过滤掉不符合预期长宽比的轮廓
        if (aspectRatio > 0.2 && aspectRatio < 1.0 && boundingRect.width > 13 && boundingRect.height > 45) {
            validRects.push_back(boundingRect);
        }
    }

    if (validRects.empty()) {
        cerr << "未找到符合条件的轮廓！" << endl;
        return {};
    }

    // 对有效矩形排序，确保从左到右顺序
    sort(validRects.begin(), validRects.end(), [](const Rect& a, const Rect& b) {
        return a.x < b.x;
        });

    vector<Mat> img_mat; // 存储原图中的有效数字区域

    for (const auto& rect : validRects) {
        Mat roi = clone_img1(rect); // 提取外接矩形区域
        Mat dstroi;
        if (roi.cols >= 13 && roi.rows >= 50) {
            resize(roi, dstroi, Size(35, 53), 0, 0); // 重设大小到统一尺寸
            img_mat.push_back(dstroi); // 存储处理后的数字区域
        }
    }

    if (img_mat.empty()) {
        cerr << " img_mat is empty!" << endl;
    }

    //// 遍历img_mat并保存为BMP图片
    //for (size_t i = 0; i < img_mat.size(); ++i) {
    //    string fileName = "recognized_digit_" + to_string(i) + ".bmp"; // 生成文件名
    //    imwrite(fileName, img_mat[i]); // 保存BMP图片
    //}


    // 加载模板图片
    vector<Mat> myTemplate; // 模板容器
    for (int i = 0; i < templatePaths.size(); i++) {
        Mat temp = imread(templatePaths[i]); // 读取模板图片
        cvtColor(temp, temp, COLOR_BGR2GRAY); // 转换为灰度图
        adaptiveThreshold(temp, temp, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2); // 自适应阈值
        myTemplate.push_back(temp); // 存储模板
        //string fileName = "binary_template" + to_string(i) + ".bmp";
        //imwrite(fileName, temp);
    }

    // 识别过程
    vector<int> seq;  // 在此处定义seq变量
    vector<int> templateNumbers = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };  // 模板对应的数字
    for (const auto& mat : img_mat) {
        double max_match = 0;
        int matched_number = -1; // 记录最匹配的数字
        double match;
         // 遍历所有模板，找到最匹配的模板
        for (size_t j = 0; j < myTemplate.size(); j++) {
            match = compare(mat, myTemplate[j]);
            if (match > max_match) {
                max_match = match;
                matched_number = j;
            }
        }
        if (matched_number != -1) {
            seq.push_back(templateNumbers[matched_number]); // 记录识别的数字
        }
    }
    // 返回识别结果
    return seq;
}


void CaptureScreen(int start_x, int start_y, int width, int height, const char* capturename) {//截图开始xy坐标（从左上算起），图片的长宽像素
    // 获取屏幕的DC（设备上下文）
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // 创建兼容的位图
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    // 复制屏幕内容到内存DC
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, start_x, start_y, SRCCOPY);

    // 创建BITMAPINFOHEADER结构
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // 负号表示自上而下的位图
    bi.biPlanes = 1;
    bi.biBitCount = 24; // 每像素24位
    bi.biCompression = BI_RGB;

    // 计算每行的字节数（确保4字节对齐）
    int rowSize = (width * 3 + 3) & ~3;

    // 为图像数据分配内存
    unsigned char* image = (unsigned char*)malloc(rowSize * height);
    GetDIBits(hMemoryDC, hBitmap, 0, height, image, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // 拼接文件名
    char filename[256];  // 分配足够大的缓冲区
    sprintf(filename, "%s.bmp", capturename);  // 将 .bmp 扩展名拼接到文件名上

    // 将图像保存为BMP文件
    FILE* file = fopen(filename, "wb");
    if (file) {
        BITMAPFILEHEADER bfh = { 0 };
        bfh.bfType = 0x4D42; // "BM"
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bfh.bfSize = bfh.bfOffBits + rowSize * height;

        // 写入位图文件头
        fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
        // 写入位图信息头
        fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, file);
        // 写入位图数据
        fwrite(image, rowSize * height, 1, file);
        fclose(file);
    }

    // 释放资源
    free(image);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

// 执行adb命令并读取输出
int execute_command(const char* command, char* result, int max_size) {
    FILE* fp;
    char buffer[MAX_BUFFER];
    int success = 0;

    // 使用popen运行命令并打开管道
    fp = _popen(command, "r");
    if (fp == NULL) {
        printf("无法执行命令\n");
        return -1;
    }

    // 读取命令的输出
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strncat(result, buffer, max_size - strlen(result) - 1);  // 拼接到结果字符串中
    }

    // 关闭管道
    success = _pclose(fp);

    return success;
}

// 检查ADB设备是否连接成功
int check_adb_connection() {
    char result[1024] = { 0 };  // 存储adb devices的输出
    execute_command("adb devices", result, sizeof(result));

    printf("adb devices output:\n%s\n", result);  // 打印输出

    // 检查输出中是否包含“device”字样，表示设备已连接
    if (strstr(result, "7555")) {
        printf("设备已成功连接\n");
        return 1;  // 成功
    }
    else {
        printf("没有检测到设备\n");
        return 0;  // 失败
    }
}

// 模拟分段滑动
void draw_polyline(int sock11, int points[][2], int num_points, int total_duration) {
    int segment_duration = total_duration / (num_points - 1);
    char response[1024] = { 0 }; // 清空响应缓冲区

    if (num_points < 2) {
        printf("Not enough points to draw a polyline.\n");
        return;
    }

    for (int i = 0; i < num_points - 1; ++i) {
        int sock = connect_to_adb("127.0.0.1", 5037);
        char swipe_cmd[128];
        snprintf(swipe_cmd, sizeof(swipe_cmd), "shell:input swipe %d %d %d %d %d",
            points[i][0], points[i][1],
            points[i + 1][0], points[i + 1][1],
            segment_duration);

        // 计算命令长度
        char swipe_cmd_len[5];
        snprintf(swipe_cmd_len, sizeof(swipe_cmd_len), "%04x", (unsigned int)strlen(swipe_cmd));

        // 发送命令长度和命令到 ADB
        if (send(sock, swipe_cmd_len, 4, 0) < 0) {
            printf("Failed to send command length.\n");
            closesocket(sock);
            return;
        }
        if (send(sock, swipe_cmd, strlen(swipe_cmd), 0) < 0) {
            printf("Failed to send swipe command.\n");
            closesocket(sock);
            return;
        }

        // 调试输出
        printf("Sending swipe command: %s\n", swipe_cmd);

        // 接收 ADB 服务器的响应
        memset(response, 0, sizeof(response)); // 清空响应缓冲区
        if (recv(sock, response, sizeof(response), 0) < 0) {
            printf("未收到Swipe命令的响应\n");
            closesocket(sock);
            return;
        }
        printf("Swipe命令响应: %s\n", response);
        closesocket(sock);
        // 等待一段时间以确保命令执行
        std::this_thread::sleep_for(std::chrono::milliseconds(segment_duration + 10));
    }
}

int main() {
    // 设置当前进程的优先级为高
    if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        printf("进程优先级设置为实时优先级成功\n");
    }
    else {
        printf("进程优先级设置为实时优先级失败\n");
    }
    //while (1)
    //{
    //    // 检查是否有按键按下
    //    if (_kbhit()) {
    //        // 读取按键输入
    //        char c = _getch();
    //        if (c == 27) {  // 27 是 Esc 键的 ASCII 值
    //            //printf("Enter pressed. Exiting...\n");
    //            break;
    //        }
    //    }
    //}

    const char* connect_command = "adb connect 127.0.0.1:7555";

    // 执行连接命令
    int result = system(connect_command);

    // 检查命令执行的结果
    if (result == 0) {
        //printf("连接命令成功执行，接下来检查设备连接状态...\n");

        // 检查连接是否成功
        if (check_adb_connection()) {
            printf("ADB 连接设备成功！\n");
        }
        else {
            printf("ADB 连接设备失败。\n");
        }
    }
    else {
        printf("连接命令执行失败，错误码: %d\n", result);
    }

    // 初始化 Winsock
    init_winsock();

    // 连接到ADB服务器
    int sock = connect_to_adb("127.0.0.1", 5037);
    if (sock == -1) {
        WSACleanup();
        return 1;
    }

    std::cout << "Press Enter to start the program..." << std::endl;
    std::cin.get();  // 等待用户按下 Enter 键
    std::cout << "Program started!" << std::endl;

    int preresultleft = 0;
    int preresultright = 0;
    int times = 0;

    while (1)
    {
        // 记录开始时间
        clock_t start = clock();

        CaptureScreen(1060, 460, 160, 150, "screenshotleft");
        CaptureScreen(1340, 460, 160, 150, "screenshotright");//截图

        // 模板路径，假设模板为0-9的数字图片
        vector<string> templatePaths;
        for (int i = 0; i < 10; i++) {
            templatePaths.push_back(format("%d.bmp", i));
        }

        // 调用识别函数并输出结果
        vector<int> result0 = recognizeNumbers("screenshotleft.bmp", templatePaths);
        vector<int> result1 = recognizeNumbers("screenshotright.bmp", templatePaths);

        if (result1.size() == 0 || result0.size() == 0)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(70000));  // 暂停40,000微秒
            continue;  //重新开始循环
        }
        // 输出结果
        int resultleft = 0;
        int resultright = 0;
        std::cout << "识别结果为：";
        for (int i = 0; i < result0.size(); i++) {
            resultleft += result0[i] * pow(10, (result0.size() - i - 1));
            std::cout << result0[i];
        }
        std::cout << "    ";
        for (int i = 0; i < result1.size(); i++) {
            resultright += result1[i] * pow(10, (result1.size() - i - 1));
            std::cout << result1[i];
        }

        if (preresultleft == resultleft && preresultright == resultright && times <= 7)
        {
            times++;
            std::this_thread::sleep_for(std::chrono::microseconds(90000));  // 暂停20,000微秒
            continue;
        }

        if (times > 7)
        {
            times = 0;
        }

        preresultleft = resultleft;
        preresultright = resultright;

        // 定义折线的坐标点 (格式：{x, y})
        int points[3][2];
        if (resultleft > resultright)
        {
            points[0][0] = 1000; points[0][1] = 1000;  // 起点
            points[1][0] = 1080; points[1][1] = 1040;
            points[2][0] = 1000; points[2][1] = 1080;  // 终点       
        }
        else
        {
            // 直接对 points 数组赋值
            points[0][0] = 1000; points[0][1] = 1000;  // 起点
            points[1][0] = 920;  points[1][1] = 1040;
            points[2][0] = 1000; points[2][1] = 1080;  // 终点
        }
        //std::this_thread::sleep_for(std::chrono::microseconds(000));  // 暂停30,000微秒
        int num_points = sizeof(points) / sizeof(points[0]);  // 点的数量
        int duration = 66;  // 总滑动持续时间

        // 记录开始时间
        //clock_t start = clock();
        std::this_thread::sleep_for(std::chrono::microseconds(20000));
        // 执行折线绘制
        draw_polyline(sock ,points, num_points, duration);

        // 记录结束时间
        clock_t end = clock();

        // 计算执行时间
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        printf("Execution time: %f seconds\n", time_spent);
        std::this_thread::sleep_for(std::chrono::microseconds(373000));  // 暂停100,000微秒
    }

    // 关闭套接字并清理 Winsock
    closesocket(sock);
    WSACleanup();

    return 0;
}