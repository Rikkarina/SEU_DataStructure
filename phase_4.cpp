#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <thread>
#include <semaphore>
#include "LoserTree.h"

const int blockSize = 50000;  // ���뻺������
const int outputQueueSize = 500000;  // �����������С
using namespace std;
namespace fs = std::filesystem;

const int k = 64; // ������˳����
vector<queue<int>> inputBuffer(2 * k);
vector<queue<int>> outputBuffer(2); // ���������
vector<int> lastValue(k, 0);

int finishedRun = 0;
ifstream inputFile[k];

// ��¼����buffer
string outputFilename = "sorted_output.txt";
queue<int> outputIndex, availableIndex, availableOutputIndex;
vector<queue<int>> inputIndex(k);
// ���߳���
mutex availablemtx;
mutex availOutputmtx;
mutex inputmtx;
mutex outputmtx;
condition_variable cv_avail;
condition_variable cv_input;
condition_variable cv_availoutput;
condition_variable cv_output;

// ����ÿ��˳���ļ�����ʽΪ "runs/run_X.txt"
string getRunFileName(int index) {
    return "runs/run_" + to_string(index) + ".txt";
}


// ��˳���ļ��ж�ȡ���ݵ�����
void readRunData(ifstream& file, queue<int>& runQueue, int index) {
    int value;
    while (runQueue.size() < blockSize && file >> value) {
        runQueue.push(value);
    }
    lastValue[index] = value;
}

void readDataIn() {
    unique_lock<mutex> lock(availablemtx);

    // һ��ѭ������һ������
    while (true) {
        // �õ�����buffer
        cv_avail.wait(lock, [] { return !availableIndex.empty();  });
        int key = availableIndex.front();
        availableIndex.pop();

        // �ҵ��������buffer
        int index = -1;
        auto min_it = min_element(lastValue.begin(), lastValue.end());
        if (min_it != lastValue.end()) {
            index = min_it - lastValue.begin();
        }

        // ���
        int value;
        while (inputBuffer[key].size() < blockSize && inputFile[index] >> value)
            inputBuffer[key].push(value);

        if (inputBuffer[key].size() < blockSize) {
            if (inputBuffer[key].size())inputIndex[index].push(key);
            inputIndex[index].push(-1);  // �����˳���ѱ�����


            cv_input.notify_one();
            lastValue[index] = INT_MAX; // ���¸�˳����β���ֵ

            finishedRun++;
            if (finishedRun == k)break;

            continue;
        }
        inputIndex[index].push(key);
        lastValue[index] = value;
        cv_input.notify_one();
    }
    cout << '\n' << "���߳��������" << '\n' << endl;
    int i = 0;
    for (auto inputBuf : inputIndex) {
        while (!inputBuf.empty()) {
            cout << i << "˳��λ��" << inputBuf.front() << endl;
            inputBuf.pop();
        }
        i++;
    }
}
// ������д�뵽����ļ�
void writeDataBlock() {
    unique_lock<mutex> lock(outputmtx);

    cv_output.wait(lock, [] { return !outputIndex.empty(); });
    int key = outputIndex.front();
    outputIndex.pop();

    ofstream outputFile(outputFilename, ios::app);

    while (key != -1) { // -1�������
        int value;
        while (!outputBuffer[key].empty()) {
            value = outputBuffer[key].front();
            outputFile << value << endl;
            outputBuffer[key].pop();
        }

        // �ͷ��Ѿ�д�õ�
        availableOutputIndex.push(key);
        cout << "д�߳��ѽ�outputIndex������д����̣����" << key << endl;
        cv_availoutput.notify_one();

        
        // �ó���һ����Ҫд��
        cv_output.wait(lock, [] {return !outputIndex.empty(); });
        key = outputIndex.front();
        outputIndex.pop();
    }
    outputFile.close();
}


int main() {
    
    fs::remove(outputFilename);
    availableOutputIndex.push(0);
    availableOutputIndex.push(1);
    for(int i = 0;i < k;i++)
        availableIndex.push(i + k);
    // ��ʼ���������
    for (int i = 0; i < k; ++i) {
        string fileName = getRunFileName(i);
        inputFile[i] = ifstream(fileName);
        readRunData(inputFile[i], inputBuffer[i], i);
        inputIndex[i].push(i);
    }

    thread readThread(readDataIn);
    thread writeThread(writeDataBlock);

    // ʹ�����ȶ��У���С�ѣ���ʵ��k·�鲢
    LoserTree loserTree(k, inputBuffer); // ֱ��������
    pair<int, int> popNode;

    int inValid = false;
    // �鲢����

    unique_lock<mutex> outlock(availOutputmtx); // �����ڿ������buffer
    unique_lock<mutex> inlock(inputmtx); // �����ڿ�������buffer
    int key;
    cv_availoutput.wait(outlock, [] { return !availableOutputIndex.empty(); });
    key = availableOutputIndex.front();
    availableOutputIndex.pop();

    cout << "��������ȡ����outputBuffer������Ϊ " << key << endl;
    while (!inValid) {
        popNode = loserTree.popMaxForMerge();

        outputBuffer[key].push(popNode.first);

        // ��������������ˣ�ִ���������
        if (outputBuffer[key].size() == outputQueueSize) {
            
            outputIndex.push(key);
            cv_output.notify_one();
            cout << "����������Buffer��outputIndex�����" << key << endl;

            cv_availoutput.wait(outlock, [] { return !availableOutputIndex.empty(); });
            key = availableOutputIndex.front();
            availableOutputIndex.pop();
            cout << "��������ȡ����outputBuffer������Ϊ " << key << endl;
        }

        int runIndex = popNode.second; 
        cv_input.wait(inlock, [runIndex] { return !inputIndex[runIndex].empty(); });

        int bufferIndex = inputIndex[runIndex].front();
        if (inputBuffer[bufferIndex].empty()) { // bufferΪ����
            // �ͷ�����buffer
            inputIndex[runIndex].pop();
            availableIndex.push(bufferIndex);
            cv_avail.notify_one();
            
            // �õ���buffer
            cv_input.wait(inlock, [runIndex] { return !inputIndex[runIndex].empty(); });
            bufferIndex = inputIndex[runIndex].front();

            if (bufferIndex == -1) {
                inValid = loserTree.insertForMerge(0, false);
                cout << "˳�� " << runIndex << "�Ѵ������" << endl;
                continue;
            }
            
        }
        inValid = loserTree.insertForMerge(inputBuffer[bufferIndex].front());
        inputBuffer[bufferIndex].pop();
    }

    // ��ʣ�������д���ļ�
    if (outputBuffer[key].size()) {
        outputIndex.push(key);
        cv_output.notify_one();
    }
    outputIndex.push(-1);
    cout << '\n' << "�������������" << '\n' << endl;
    cv_output.notify_one();

    readThread.join();
    writeThread.join();

    cout << "K-way merge completed successfully." << endl;

    return 0;
}