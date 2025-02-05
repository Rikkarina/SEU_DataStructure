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

const int blockSize = 50000;  // 输入缓冲区大
const int outputQueueSize = 500000;  // 输出缓冲区大小
using namespace std;
namespace fs = std::filesystem;

const int k = 64; // 设置总顺串数
vector<queue<int>> inputBuffer(2 * k);
vector<queue<int>> outputBuffer(2); // 输出缓冲区
vector<int> lastValue(k, 0);

int finishedRun = 0;
ifstream inputFile[k];

// 记录可用buffer
string outputFilename = "sorted_output.txt";
queue<int> outputIndex, availableIndex, availableOutputIndex;
vector<queue<int>> inputIndex(k);
// 多线程用
mutex availablemtx;
mutex availOutputmtx;
mutex inputmtx;
mutex outputmtx;
condition_variable cv_avail;
condition_variable cv_input;
condition_variable cv_availoutput;
condition_variable cv_output;

// 假设每个顺串文件名格式为 "runs/run_X.txt"
string getRunFileName(int index) {
    return "runs/run_" + to_string(index) + ".txt";
}


// 从顺串文件中读取数据到队列
void readRunData(ifstream& file, queue<int>& runQueue, int index) {
    int value;
    while (runQueue.size() < blockSize && file >> value) {
        runQueue.push(value);
    }
    lastValue[index] = value;
}

void readDataIn() {
    unique_lock<mutex> lock(availablemtx);

    // 一次循环读好一次数据
    while (true) {
        // 拿到可用buffer
        cv_avail.wait(lock, [] { return !availableIndex.empty();  });
        int key = availableIndex.front();
        availableIndex.pop();

        // 找到最快耗完的buffer
        int index = -1;
        auto min_it = min_element(lastValue.begin(), lastValue.end());
        if (min_it != lastValue.end()) {
            index = min_it - lastValue.begin();
        }

        // 填充
        int value;
        while (inputBuffer[key].size() < blockSize && inputFile[index] >> value)
            inputBuffer[key].push(value);

        if (inputBuffer[key].size() < blockSize) {
            if (inputBuffer[key].size())inputIndex[index].push(key);
            inputIndex[index].push(-1);  // 代表该顺串已被读完


            cv_input.notify_one();
            lastValue[index] = INT_MAX; // 更新该顺串结尾最大值

            finishedRun++;
            if (finishedRun == k)break;

            continue;
        }
        inputIndex[index].push(key);
        lastValue[index] = value;
        cv_input.notify_one();
    }
    cout << '\n' << "读线程完成任务" << '\n' << endl;
    int i = 0;
    for (auto inputBuf : inputIndex) {
        while (!inputBuf.empty()) {
            cout << i << "顺串位置" << inputBuf.front() << endl;
            inputBuf.pop();
        }
        i++;
    }
}
// 将数据写入到输出文件
void writeDataBlock() {
    unique_lock<mutex> lock(outputmtx);

    cv_output.wait(lock, [] { return !outputIndex.empty(); });
    int key = outputIndex.front();
    outputIndex.pop();

    ofstream outputFile(outputFilename, ios::app);

    while (key != -1) { // -1代表结束
        int value;
        while (!outputBuffer[key].empty()) {
            value = outputBuffer[key].front();
            outputFile << value << endl;
            outputBuffer[key].pop();
        }

        // 释放已经写好的
        availableOutputIndex.push(key);
        cout << "写线程已将outputIndex的数据写入磁盘，编号" << key << endl;
        cv_availoutput.notify_one();

        
        // 拿出下一个需要写的
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
    // 初始化输入队列
    for (int i = 0; i < k; ++i) {
        string fileName = getRunFileName(i);
        inputFile[i] = ifstream(fileName);
        readRunData(inputFile[i], inputBuffer[i], i);
        inputIndex[i].push(i);
    }

    thread readThread(readDataIn);
    thread writeThread(writeDataBlock);

    // 使用优先队列（最小堆）来实现k路归并
    LoserTree loserTree(k, inputBuffer); // 直接填满了
    pair<int, int> popNode;

    int inValid = false;
    // 归并过程

    unique_lock<mutex> outlock(availOutputmtx); // 受限于可用输出buffer
    unique_lock<mutex> inlock(inputmtx); // 受限于可用输入buffer
    int key;
    cv_availoutput.wait(outlock, [] { return !availableOutputIndex.empty(); });
    key = availableOutputIndex.front();
    availableOutputIndex.pop();

    cout << "败者树获取可用outputBuffer，索引为 " << key << endl;
    while (!inValid) {
        popNode = loserTree.popMaxForMerge();

        outputBuffer[key].push(popNode.first);

        // 当输出缓冲区满了，执行输出操作
        if (outputBuffer[key].size() == outputQueueSize) {
            
            outputIndex.push(key);
            cv_output.notify_one();
            cout << "败者树传递Buffer给outputIndex，编号" << key << endl;

            cv_availoutput.wait(outlock, [] { return !availableOutputIndex.empty(); });
            key = availableOutputIndex.front();
            availableOutputIndex.pop();
            cout << "败者树获取可用outputBuffer，索引为 " << key << endl;
        }

        int runIndex = popNode.second; 
        cv_input.wait(inlock, [runIndex] { return !inputIndex[runIndex].empty(); });

        int bufferIndex = inputIndex[runIndex].front();
        if (inputBuffer[bufferIndex].empty()) { // buffer为空了
            // 释放输入buffer
            inputIndex[runIndex].pop();
            availableIndex.push(bufferIndex);
            cv_avail.notify_one();
            
            // 得到新buffer
            cv_input.wait(inlock, [runIndex] { return !inputIndex[runIndex].empty(); });
            bufferIndex = inputIndex[runIndex].front();

            if (bufferIndex == -1) {
                inValid = loserTree.insertForMerge(0, false);
                cout << "顺串 " << runIndex << "已处理完毕" << endl;
                continue;
            }
            
        }
        inValid = loserTree.insertForMerge(inputBuffer[bufferIndex].front());
        inputBuffer[bufferIndex].pop();
    }

    // 将剩余的数据写入文件
    if (outputBuffer[key].size()) {
        outputIndex.push(key);
        cv_output.notify_one();
    }
    outputIndex.push(-1);
    cout << '\n' << "败者树完成任务" << '\n' << endl;
    cv_output.notify_one();

    readThread.join();
    writeThread.join();

    cout << "K-way merge completed successfully." << endl;

    return 0;
}