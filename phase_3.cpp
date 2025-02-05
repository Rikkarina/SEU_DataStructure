#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string>
#include <queue>
#include <limits>
#include <thread>
#include <mutex>
#include <filesystem>
#include <condition_variable>
#include <semaphore> 
#include "LoserTree.h"

const int numLeaves = 80000;
const int blockSize = 30000;  // 缓存区大小

using namespace std;
namespace fs = std::filesystem;

queue<int> buffer[3];

// 输出统计用
int runSum = 0;
int totalSum = 0;
int runIndex = 0;
string outputDir = "runs";  // 存储顺串文件的目录

// 记录可用buffer
queue<int> inputIndex, outputIndex, availableIndex;

// 多线程用
mutex availablemtx;
mutex inputmtx;
mutex outputmtx;
condition_variable cv_avail;
condition_variable cv_input;
condition_variable cv_output;

// 输入缓冲区读取数据
void readDataBlock(ifstream& inputFile) {
   unique_lock<mutex> lock(availablemtx);

   while (true) {
       cv_avail.wait(lock, [] { return !availableIndex.empty(); });
       int key = availableIndex.front();
       availableIndex.pop();

       int value;
       while (buffer[key].size() < blockSize && inputFile >> value) {
           buffer[key].push(value);
       }
       if (buffer[key].size() < blockSize) {
           if(buffer[key].size())inputIndex.push(key);
           inputIndex.push(-1);
           // cout << key << "buffer,input时容量为：" << buffer[key].size() << endl;
           cv_input.notify_one();

           break;
       }
       inputIndex.push(key);
       // cout << key << "buffer,input时容量为：" << buffer[key].size() << endl;
       cv_input.notify_one();
   }
}

// 输出缓冲区输出数据
void writeDataBlock(queue<int>& data, const string& fileName) {

   ofstream outputFile(fileName, ios::app);
   int lastValue = INT_MIN;
   if (!outputFile.is_open()) {
       cerr << "无法打开输出文件: " << fileName << endl;
       return;
   }

   while (data.size()) {
       int value = data.front();
       
       if (lastValue > value) {
           cout << "生成顺串文件: " << fileName << "，输入 " << runSum << " 条数据。" << endl;
           totalSum += runSum;
           runSum = 0;
           runIndex++;
           break;
       }
       data.pop();

       outputFile << value << endl;
       runSum++;
       lastValue = value;
   }
   outputFile.close();
}

void writeDataOut() {
   unique_lock<mutex> lock(outputmtx);

   cv_output.wait(lock, [] {return !outputIndex.empty(); });
   int key = outputIndex.front();
   outputIndex.pop();

   while (key != -1) {
       string outputFileName = outputDir + "/run_" + to_string(runIndex) + ".txt";

       // cout << key << "buffer,真排出去时容量为：" << buffer[key].size() << endl;
       writeDataBlock(buffer[key], outputFileName);
       if (!buffer[key].empty())continue;
       availableIndex.push(key);
       cv_avail.notify_one();

       cv_output.wait(lock, [] {return !outputIndex.empty(); });
       key = outputIndex.front();
       outputIndex.pop();
   }
   string outputFileName = outputDir + "/run_" + to_string(runIndex) + ".txt";
   cout << "生成顺串文件: " << outputFileName << "，输入 " << runSum << " 条数据。" << endl;
   totalSum += runSum;
}

// 生成初始顺串并保存到多个文件中
void generateInitialRuns() {
   unique_lock<mutex> lock(inputmtx);
   cv_input.wait(lock, [] {return !inputIndex.empty(); });
   int key = inputIndex.front();
   inputIndex.pop();

   if (key == -1) {
       // cerr << "输入文件为空或数据不足以填满初始块。" << endl;
       return;
   }

   LoserTree loserTree(numLeaves,buffer[key]);

   // 初始化，再填满
   if (!loserTree.isReady()) {
       availableIndex.push(key);
       cv_avail.notify_one();

       cv_input.wait(lock, [] {return !inputIndex.empty(); });
       key = inputIndex.front();
       inputIndex.pop();

       while (key != -1) {
           bool tag = loserTree.fillLeaves(buffer[key]);
           if (tag)break;
           else {
               availableIndex.push(key);
               cv_avail.notify_one();

               cv_input.wait(lock, [] {return !inputIndex.empty(); });
               key = inputIndex.front();
               inputIndex.pop();
           }
       }
   }
   

   while(true) {
       // 满树
       buffer[key].push(loserTree.popMax());

       for (int i = 0; i < buffer[key].size() - 1; i++) { // 存疑
           int value = buffer[key].front();
           buffer[key].pop();
           loserTree.insert(value);
           buffer[key].push(loserTree.popMax());
       }
       // 缺顶
       outputIndex.push(key);
       // cout << key << "buffer,ouput时容量为："<< buffer[key].size() << endl;
       cv_output.notify_one();

       cv_input.wait(lock, [] {return !inputIndex.empty(); });
       key = inputIndex.front();
       inputIndex.pop();
       if (key == -1)break; // 表示所有数据均读完

       // 添加数据，变成满树
       int value = buffer[key].front();
       buffer[key].pop();
       loserTree.insert(value);
   }


   // 将剩余数据挤出来
   unique_lock<mutex> availLock(availablemtx);
   cv_avail.wait(availLock, [] {return !availableIndex.empty(); });
   key = availableIndex.front();
   availableIndex.pop();

   for (int i = 0; i < numLeaves - 1; i++) {
       loserTree.insert(INT_MAX, false);

       buffer[key].push(loserTree.popMax());

       if (buffer[key].size() == blockSize) {
           outputIndex.push(key);
           // cout << key << "buffer,ouput时容量为：" << buffer[key].size() << endl;
           cv_output.notify_one();

           cv_avail.wait(lock, [] {return !availableIndex.empty(); });
           key = availableIndex.front();
           availableIndex.pop();
       }
   }
   outputIndex.push(key);
   // cout << key << "buffer,ouput时容量为：" << buffer[key].size() << endl;
   outputIndex.push(-1);
}

int main() {
   string inputFileName = "large_data.txt";  // 输入数据文件名
   ifstream inputFile(inputFileName);
   if (!inputFile.is_open()) {
       cerr << "无法打开输入文件: " << inputFileName << endl;
       return 0;
   }
   fs::remove_all(outputDir);  
   fs::create_directory(outputDir);
   for(int i = 0;i < 3;i++)
       availableIndex.push(i);


   vector<thread> threads(3);
   threads[0] = thread(readDataBlock, ref(inputFile));
   threads[1] = thread(generateInitialRuns);
   threads[2] = thread(writeDataOut);

   for (auto& t : threads) {
       t.join();
   }

   cout << "顺串生成完成，总共生成了 " << runIndex + 1 << " 个顺串文件。" << "共" << totalSum << "条数据" << endl;
   return 0;
}
