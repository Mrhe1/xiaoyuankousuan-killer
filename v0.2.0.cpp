#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>  // ���� inet_pton �� inet_ntop
#pragma comment(lib, "ws2_32.lib")  // ���� ws2_32.lib
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

// ��ʼ�� Winsock
void init_winsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        printf("WSAStartup ʧ�ܣ��������: %d\n", result);
        exit(1);
    }
}

// ���ӵ� ADB �������ĺ���
int connect_to_adb(const char* server_ip, int port) {
    // �����׽���
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("�޷������׽���\n");
        return -1;
    }

    // ���÷�������ַ��Ϣ
    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(server_ip);  // ADB��������ַ
    server.sin_family = AF_INET;
    server.sin_port = htons(port);  // ADB�������˿�

    // ���ӵ�ADB������
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        printf("ADB����������ʧ��\n");
        closesocket(sock);
        return -1;
    }
    printf("�����ӵ�ADB������\n");

    // �������host:transport:<device_id>����ѡ���豸
    const char* device_cmd = "host:transport:127.0.0.1:7555";
    char device_cmd_len[5];
    snprintf(device_cmd_len, sizeof(device_cmd_len), "%04x", (unsigned int)strlen(device_cmd));  // �����

    // ������Ⱥ�����͵�ADB
    send(sock, device_cmd_len, 4, 0);
    send(sock, device_cmd, strlen(device_cmd), 0);

    // ����ADB��������Ӧ
    char response[1024] = { 0 };
    if (recv(sock, response, sizeof(response), 0) < 0) {
        printf("δ�յ��豸��Ӧ\n");
        return -1;
    }
    printf("�豸��Ӧ: %s\n", response);

    return sock;  // �����׽���������
}

// ����Swipe����ĺ���
//int send_swipe_command(int sock ,int start_x ,int start_y,int end_x,int end_y,int duration) {
//    //// �������host:transport:<device_id>����ѡ���豸
//    //const char* device_cmd = "host:transport:DEVICE_ID";
//    //char device_cmd_len[5];
//    //snprintf(device_cmd_len, sizeof(device_cmd_len), "%04x", (unsigned int)strlen(device_cmd));  // �����
//
//    //// ������Ⱥ�����͵�ADB
//    //send(sock, device_cmd_len, 4, 0);
//    //send(sock, device_cmd, strlen(device_cmd), 0);
//
//    //// ����ADB��������Ӧ
//    char response[1024] = { 0 };
//    //if (recv(sock, response, sizeof(response), 0) < 0) {
//    //    printf("δ�յ��豸��Ӧ\n");
//    //    return -1;
//    //}
//    //printf("�豸��Ӧ: %s\n", response);
//
//    // ��ʽ��Swipe����
//    char swipe_cmd[128];  // �洢������ swipe ����
//    snprintf(swipe_cmd, sizeof(swipe_cmd), "shell:input swipe %d %d %d %d %d", start_x, start_y, end_x, end_y, duration);
//
//    // ����Swipe����ĳ���
//    char swipe_cmd_len[5];
//    snprintf(swipe_cmd_len, sizeof(swipe_cmd_len), "%04x", (unsigned int)strlen(swipe_cmd));
//
//    // ������Ⱥ�����͵�ADB
//    send(sock, swipe_cmd_len, 4, 0);
//    send(sock, swipe_cmd, strlen(swipe_cmd), 0);
//
//
//    // ����ADB��������Ӧ
//    memset(response, 0, sizeof(response));
//    if (recv(sock, response, sizeof(response), 0) < 0) {
//        printf("δ�յ�Swipe�������Ӧ\n");
//        return -1;
//    }
//    printf("Swipe������Ӧ: %s\n", response);
//
//    return 0;  // �ɹ�
//}

// �ȽϺ�����ʵ��
double compare(const Mat& img, const Mat& temp) {
    Mat result;
    double threshold = 0.60;

    matchTemplate(img, temp, result, TM_CCOEFF_NORMED);
    double maxVal;
    minMaxLoc(result, nullptr, &maxVal, nullptr, nullptr);

    return (maxVal > threshold) ? maxVal : 0; // ʹ������������򻯴���
}
// ����ʶ�����ֵĺ���
vector<int> recognizeNumbers(string imgPath, vector<string> templatePaths) {
    Mat img = imread(imgPath);  // ��ȡ����ͼƬ
    Mat clone_img = img.clone(); // ԭͼ����
    cvtColor(img, img, COLOR_BGR2GRAY); // ͼƬת�Ҷ�
    adaptiveThreshold(img, img, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);//����Ӧ��ֵ
    Mat clone_img1 = img.clone(); // ���ݶ�ֵ�����ԭͼ
    imwrite("binary_image.bmp", img);

    // ��������
    vector<vector<Point>> contours; // �洢ԭͼ����
    vector<Vec4i> hierarchy;
    findContours(img, contours, hierarchy, RETR_TREE, CHAIN_APPROX_SIMPLE); // ������е�����
    //drawContours(clone_img, contours, -1, Scalar(0, 255, 0), 1, 8); // ��������
    //imwrite("contours_image.bmp", clone_img);

    vector<Rect> validRects; // ����洢��Ч��Ӿ��ε�����
    for (const auto& contour : contours) {
        Rect boundingRect = cv::boundingRect(contour);
        double area = cv::contourArea(contour);
        float aspectRatio = (float)boundingRect.width / (float)boundingRect.height;

        // ���˵�������Ԥ�ڳ���ȵ�����
        if (aspectRatio > 0.2 && aspectRatio < 1.0 && boundingRect.width > 13 && boundingRect.height > 45) {
            validRects.push_back(boundingRect);
        }
    }

    if (validRects.empty()) {
        cerr << "δ�ҵ�����������������" << endl;
        return {};
    }

    // ����Ч��������ȷ��������˳��
    sort(validRects.begin(), validRects.end(), [](const Rect& a, const Rect& b) {
        return a.x < b.x;
        });

    vector<Mat> img_mat; // �洢ԭͼ�е���Ч��������

    for (const auto& rect : validRects) {
        Mat roi = clone_img1(rect); // ��ȡ��Ӿ�������
        Mat dstroi;
        if (roi.cols >= 13 && roi.rows >= 50) {
            resize(roi, dstroi, Size(35, 53), 0, 0); // �����С��ͳһ�ߴ�
            img_mat.push_back(dstroi); // �洢��������������
        }
    }

    if (img_mat.empty()) {
        cerr << " img_mat is empty!" << endl;
    }

    //// ����img_mat������ΪBMPͼƬ
    //for (size_t i = 0; i < img_mat.size(); ++i) {
    //    string fileName = "recognized_digit_" + to_string(i) + ".bmp"; // �����ļ���
    //    imwrite(fileName, img_mat[i]); // ����BMPͼƬ
    //}


    // ����ģ��ͼƬ
    vector<Mat> myTemplate; // ģ������
    for (int i = 0; i < templatePaths.size(); i++) {
        Mat temp = imread(templatePaths[i]); // ��ȡģ��ͼƬ
        cvtColor(temp, temp, COLOR_BGR2GRAY); // ת��Ϊ�Ҷ�ͼ
        adaptiveThreshold(temp, temp, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2); // ����Ӧ��ֵ
        myTemplate.push_back(temp); // �洢ģ��
        //string fileName = "binary_template" + to_string(i) + ".bmp";
        //imwrite(fileName, temp);
    }

    // ʶ�����
    vector<int> seq;  // �ڴ˴�����seq����
    vector<int> templateNumbers = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };  // ģ���Ӧ������
    for (const auto& mat : img_mat) {
        double max_match = 0;
        int matched_number = -1; // ��¼��ƥ�������
        double match;
         // ��������ģ�壬�ҵ���ƥ���ģ��
        for (size_t j = 0; j < myTemplate.size(); j++) {
            match = compare(mat, myTemplate[j]);
            if (match > max_match) {
                max_match = match;
                matched_number = j;
            }
        }
        if (matched_number != -1) {
            seq.push_back(templateNumbers[matched_number]); // ��¼ʶ�������
        }
    }
    // ����ʶ����
    return seq;
}


void CaptureScreen(int start_x, int start_y, int width, int height, const char* capturename) {//��ͼ��ʼxy���꣨���������𣩣�ͼƬ�ĳ�������
    // ��ȡ��Ļ��DC���豸�����ģ�
    HDC hScreenDC = GetDC(NULL);
    HDC hMemoryDC = CreateCompatibleDC(hScreenDC);

    // �������ݵ�λͼ
    HBITMAP hBitmap = CreateCompatibleBitmap(hScreenDC, width, height);
    SelectObject(hMemoryDC, hBitmap);

    // ������Ļ���ݵ��ڴ�DC
    BitBlt(hMemoryDC, 0, 0, width, height, hScreenDC, start_x, start_y, SRCCOPY);

    // ����BITMAPINFOHEADER�ṹ
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height; // ���ű�ʾ���϶��µ�λͼ
    bi.biPlanes = 1;
    bi.biBitCount = 24; // ÿ����24λ
    bi.biCompression = BI_RGB;

    // ����ÿ�е��ֽ�����ȷ��4�ֽڶ��룩
    int rowSize = (width * 3 + 3) & ~3;

    // Ϊͼ�����ݷ����ڴ�
    unsigned char* image = (unsigned char*)malloc(rowSize * height);
    GetDIBits(hMemoryDC, hBitmap, 0, height, image, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // ƴ���ļ���
    char filename[256];  // �����㹻��Ļ�����
    sprintf(filename, "%s.bmp", capturename);  // �� .bmp ��չ��ƴ�ӵ��ļ�����

    // ��ͼ�񱣴�ΪBMP�ļ�
    FILE* file = fopen(filename, "wb");
    if (file) {
        BITMAPFILEHEADER bfh = { 0 };
        bfh.bfType = 0x4D42; // "BM"
        bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
        bfh.bfSize = bfh.bfOffBits + rowSize * height;

        // д��λͼ�ļ�ͷ
        fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
        // д��λͼ��Ϣͷ
        fwrite(&bi, sizeof(BITMAPINFOHEADER), 1, file);
        // д��λͼ����
        fwrite(image, rowSize * height, 1, file);
        fclose(file);
    }

    // �ͷ���Դ
    free(image);
    DeleteObject(hBitmap);
    DeleteDC(hMemoryDC);
    ReleaseDC(NULL, hScreenDC);
}

// ִ��adb�����ȡ���
int execute_command(const char* command, char* result, int max_size) {
    FILE* fp;
    char buffer[MAX_BUFFER];
    int success = 0;

    // ʹ��popen��������򿪹ܵ�
    fp = _popen(command, "r");
    if (fp == NULL) {
        printf("�޷�ִ������\n");
        return -1;
    }

    // ��ȡ��������
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strncat(result, buffer, max_size - strlen(result) - 1);  // ƴ�ӵ�����ַ�����
    }

    // �رչܵ�
    success = _pclose(fp);

    return success;
}

// ���ADB�豸�Ƿ����ӳɹ�
int check_adb_connection() {
    char result[1024] = { 0 };  // �洢adb devices�����
    execute_command("adb devices", result, sizeof(result));

    printf("adb devices output:\n%s\n", result);  // ��ӡ���

    // ���������Ƿ������device����������ʾ�豸������
    if (strstr(result, "7555")) {
        printf("�豸�ѳɹ�����\n");
        return 1;  // �ɹ�
    }
    else {
        printf("û�м�⵽�豸\n");
        return 0;  // ʧ��
    }
}

// ģ��ֶλ���
void draw_polyline(int sock11, int points[][2], int num_points, int total_duration) {
    int segment_duration = total_duration / (num_points - 1);
    char response[1024] = { 0 }; // �����Ӧ������

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

        // ���������
        char swipe_cmd_len[5];
        snprintf(swipe_cmd_len, sizeof(swipe_cmd_len), "%04x", (unsigned int)strlen(swipe_cmd));

        // ��������Ⱥ���� ADB
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

        // �������
        printf("Sending swipe command: %s\n", swipe_cmd);

        // ���� ADB ����������Ӧ
        memset(response, 0, sizeof(response)); // �����Ӧ������
        if (recv(sock, response, sizeof(response), 0) < 0) {
            printf("δ�յ�Swipe�������Ӧ\n");
            closesocket(sock);
            return;
        }
        printf("Swipe������Ӧ: %s\n", response);
        closesocket(sock);
        // �ȴ�һ��ʱ����ȷ������ִ��
        std::this_thread::sleep_for(std::chrono::milliseconds(segment_duration + 10));
    }
}

int main() {
    // ���õ�ǰ���̵����ȼ�Ϊ��
    if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS)) {
        printf("�������ȼ�����Ϊʵʱ���ȼ��ɹ�\n");
    }
    else {
        printf("�������ȼ�����Ϊʵʱ���ȼ�ʧ��\n");
    }
    //while (1)
    //{
    //    // ����Ƿ��а�������
    //    if (_kbhit()) {
    //        // ��ȡ��������
    //        char c = _getch();
    //        if (c == 27) {  // 27 �� Esc ���� ASCII ֵ
    //            //printf("Enter pressed. Exiting...\n");
    //            break;
    //        }
    //    }
    //}

    const char* connect_command = "adb connect 127.0.0.1:7555";

    // ִ����������
    int result = system(connect_command);

    // �������ִ�еĽ��
    if (result == 0) {
        //printf("��������ɹ�ִ�У�����������豸����״̬...\n");

        // ��������Ƿ�ɹ�
        if (check_adb_connection()) {
            printf("ADB �����豸�ɹ���\n");
        }
        else {
            printf("ADB �����豸ʧ�ܡ�\n");
        }
    }
    else {
        printf("��������ִ��ʧ�ܣ�������: %d\n", result);
    }

    // ��ʼ�� Winsock
    init_winsock();

    // ���ӵ�ADB������
    int sock = connect_to_adb("127.0.0.1", 5037);
    if (sock == -1) {
        WSACleanup();
        return 1;
    }

    std::cout << "Press Enter to start the program..." << std::endl;
    std::cin.get();  // �ȴ��û����� Enter ��
    std::cout << "Program started!" << std::endl;

    int preresultleft = 0;
    int preresultright = 0;
    int times = 0;

    while (1)
    {
        // ��¼��ʼʱ��
        clock_t start = clock();

        CaptureScreen(1060, 460, 160, 150, "screenshotleft");
        CaptureScreen(1340, 460, 160, 150, "screenshotright");//��ͼ

        // ģ��·��������ģ��Ϊ0-9������ͼƬ
        vector<string> templatePaths;
        for (int i = 0; i < 10; i++) {
            templatePaths.push_back(format("%d.bmp", i));
        }

        // ����ʶ������������
        vector<int> result0 = recognizeNumbers("screenshotleft.bmp", templatePaths);
        vector<int> result1 = recognizeNumbers("screenshotright.bmp", templatePaths);

        if (result1.size() == 0 || result0.size() == 0)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(70000));  // ��ͣ40,000΢��
            continue;  //���¿�ʼѭ��
        }
        // ������
        int resultleft = 0;
        int resultright = 0;
        std::cout << "ʶ����Ϊ��";
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
            std::this_thread::sleep_for(std::chrono::microseconds(90000));  // ��ͣ20,000΢��
            continue;
        }

        if (times > 7)
        {
            times = 0;
        }

        preresultleft = resultleft;
        preresultright = resultright;

        // �������ߵ������ (��ʽ��{x, y})
        int points[3][2];
        if (resultleft > resultright)
        {
            points[0][0] = 1000; points[0][1] = 1000;  // ���
            points[1][0] = 1080; points[1][1] = 1040;
            points[2][0] = 1000; points[2][1] = 1080;  // �յ�       
        }
        else
        {
            // ֱ�Ӷ� points ���鸳ֵ
            points[0][0] = 1000; points[0][1] = 1000;  // ���
            points[1][0] = 920;  points[1][1] = 1040;
            points[2][0] = 1000; points[2][1] = 1080;  // �յ�
        }
        //std::this_thread::sleep_for(std::chrono::microseconds(000));  // ��ͣ30,000΢��
        int num_points = sizeof(points) / sizeof(points[0]);  // �������
        int duration = 66;  // �ܻ�������ʱ��

        // ��¼��ʼʱ��
        //clock_t start = clock();
        std::this_thread::sleep_for(std::chrono::microseconds(20000));
        // ִ�����߻���
        draw_polyline(sock ,points, num_points, duration);

        // ��¼����ʱ��
        clock_t end = clock();

        // ����ִ��ʱ��
        double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
        printf("Execution time: %f seconds\n", time_spent);
        std::this_thread::sleep_for(std::chrono::microseconds(373000));  // ��ͣ100,000΢��
    }

    // �ر��׽��ֲ����� Winsock
    closesocket(sock);
    WSACleanup();

    return 0;
}