//#include <iostream>
//#include <fstream>
//#include <vector>
//#include <algorithm>
//#include <string>
//#include <queue>
//#include <limits>
//#include <thread>
//#include <mutex>
//#include <filesystem>
//#include <condition_variable>
//#include <semaphore> 
//#include "LoserTree.h"
//
//const int numLeaves = 80000;
//const int blockSize = 30000;  // ��������С
//
//using namespace std;
//namespace fs = std::filesystem;
//
//queue<int> buffer[3];
//
//// ���ͳ����
//int runSum = 0;
//int totalSum = 0;
//int runIndex = 0;
//string outputDir = "runs";  // �洢˳���ļ���Ŀ¼
//
//// ��¼����buffer
//queue<int> inputIndex, outputIndex, availableIndex;
//
//// ���߳���
//mutex availablemtx;
//mutex inputmtx;
//mutex outputmtx;
//condition_variable cv_avail;
//condition_variable cv_input;
//condition_variable cv_output;
//
//// ���뻺������ȡ����
//void readDataBlock(ifstream& inputFile) {
//    unique_lock<mutex> lock(availablemtx);
//
//    while (true) {
//        cv_avail.wait(lock, [] { return !availableIndex.empty(); });
//        int key = availableIndex.front();
//        availableIndex.pop();
//
//        int value;
//        while (buffer[key].size() < blockSize && inputFile >> value) {
//            buffer[key].push(value);
//        }
//        if (buffer[key].size() < blockSize) {
//            if(buffer[key].size())inputIndex.push(key);
//            inputIndex.push(-1);
//            // cout << key << "buffer,inputʱ����Ϊ��" << buffer[key].size() << endl;
//            cv_input.notify_one();
//
//            break;
//        }
//        inputIndex.push(key);
//        // cout << key << "buffer,inputʱ����Ϊ��" << buffer[key].size() << endl;
//        cv_input.notify_one();
//    }
//}
//
//// ����������������
//void writeDataBlock(queue<int>& data, const string& fileName) {
//
//    ofstream outputFile(fileName, ios::app);
//    int lastValue = INT_MIN;
//    if (!outputFile.is_open()) {
//        cerr << "�޷�������ļ�: " << fileName << endl;
//        return;
//    }
//
//    while (data.size()) {
//        int value = data.front();
//        
//        if (lastValue > value) {
//            cout << "����˳���ļ�: " << fileName << "������ " << runSum << " �����ݡ�" << endl;
//            totalSum += runSum;
//            runSum = 0;
//            runIndex++;
//            break;
//        }
//        data.pop();
//
//        outputFile << value << endl;
//        runSum++;
//        lastValue = value;
//    }
//    outputFile.close();
//}
//
//void writeDataOut() {
//    unique_lock<mutex> lock(outputmtx);
//
//    cv_output.wait(lock, [] {return !outputIndex.empty(); });
//    int key = outputIndex.front();
//    outputIndex.pop();
//
//    while (key != -1) {
//        string outputFileName = outputDir + "/run_" + to_string(runIndex) + ".txt";
//
//        // cout << key << "buffer,���ų�ȥʱ����Ϊ��" << buffer[key].size() << endl;
//        writeDataBlock(buffer[key], outputFileName);
//        if (!buffer[key].empty())continue;
//        availableIndex.push(key);
//        cv_avail.notify_one();
//
//        cv_output.wait(lock, [] {return !outputIndex.empty(); });
//        key = outputIndex.front();
//        outputIndex.pop();
//    }
//    string outputFileName = outputDir + "/run_" + to_string(runIndex) + ".txt";
//    cout << "����˳���ļ�: " << outputFileName << "������ " << runSum << " �����ݡ�" << endl;
//    totalSum += runSum;
//}
//
//// ���ɳ�ʼ˳�������浽����ļ���
//void generateInitialRuns() {
//    unique_lock<mutex> lock(inputmtx);
//    cv_input.wait(lock, [] {return !inputIndex.empty(); });
//    int key = inputIndex.front();
//    inputIndex.pop();
//
//    if (key == -1) {
//        // cerr << "�����ļ�Ϊ�ջ����ݲ�����������ʼ�顣" << endl;
//        return;
//    }
//
//    LoserTree loserTree(numLeaves,buffer[key]);
//
//    // ��ʼ����������
//    if (!loserTree.isReady()) {
//        availableIndex.push(key);
//        cv_avail.notify_one();
//
//        cv_input.wait(lock, [] {return !inputIndex.empty(); });
//        key = inputIndex.front();
//        inputIndex.pop();
//
//        while (key != -1) {
//            bool tag = loserTree.fillLeaves(buffer[key]);
//            if (tag)break;
//            else {
//                availableIndex.push(key);
//                cv_avail.notify_one();
//
//                cv_input.wait(lock, [] {return !inputIndex.empty(); });
//                key = inputIndex.front();
//                inputIndex.pop();
//            }
//        }
//    }
//    
//
//    while(true) {
//        // ����
//        buffer[key].push(loserTree.popMax());
//
//        for (int i = 0; i < buffer[key].size() - 1; i++) { // ����
//            int value = buffer[key].front();
//            buffer[key].pop();
//            loserTree.insert(value);
//            buffer[key].push(loserTree.popMax());
//        }
//        // ȱ��
//        outputIndex.push(key);
//        // cout << key << "buffer,ouputʱ����Ϊ��"<< buffer[key].size() << endl;
//        cv_output.notify_one();
//
//        cv_input.wait(lock, [] {return !inputIndex.empty(); });
//        key = inputIndex.front();
//        inputIndex.pop();
//        if (key == -1)break; // ��ʾ�������ݾ�����
//
//        // ������ݣ��������
//        int value = buffer[key].front();
//        buffer[key].pop();
//        loserTree.insert(value);
//    }
//
//
//    // ��ʣ�����ݼ�����
//    unique_lock<mutex> availLock(availablemtx);
//    cv_avail.wait(availLock, [] {return !availableIndex.empty(); });
//    key = availableIndex.front();
//    availableIndex.pop();
//
//    for (int i = 0; i < numLeaves - 1; i++) {
//        loserTree.insert(INT_MAX, false);
//
//        buffer[key].push(loserTree.popMax());
//
//        if (buffer[key].size() == blockSize) {
//            outputIndex.push(key);
//            // cout << key << "buffer,ouputʱ����Ϊ��" << buffer[key].size() << endl;
//            cv_output.notify_one();
//
//            cv_avail.wait(lock, [] {return !availableIndex.empty(); });
//            key = availableIndex.front();
//            availableIndex.pop();
//        }
//    }
//    outputIndex.push(key);
//    // cout << key << "buffer,ouputʱ����Ϊ��" << buffer[key].size() << endl;
//    outputIndex.push(-1);
//}
//
//int main() {
//    string inputFileName = "large_data.txt";  // ���������ļ���
//    ifstream inputFile(inputFileName);
//    if (!inputFile.is_open()) {
//        cerr << "�޷��������ļ�: " << inputFileName << endl;
//        return 0;
//    }
//    fs::remove_all(outputDir);  
//    fs::create_directory(outputDir);
//    for(int i = 0;i < 3;i++)
//        availableIndex.push(i);
//
//
//    vector<thread> threads(3);
//    threads[0] = thread(readDataBlock, ref(inputFile));
//    threads[1] = thread(generateInitialRuns);
//    threads[2] = thread(writeDataOut);
//
//    for (auto& t : threads) {
//        t.join();
//    }
//
//    cout << "˳��������ɣ��ܹ������� " << runIndex + 1 << " ��˳���ļ���" << "��" << totalSum << "������" << endl;
//    return 0;
//}
