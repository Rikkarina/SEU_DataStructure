#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <string>
#include <algorithm>
#include <sstream>
#include <filesystem>

using namespace std;

const int chunkSize = 100000; // 假定内存能够产生的最大顺串的大小
const string inputFilename = "large_data.txt";
const string tempDir = "dataset";
const string outputFilename = "sorted.txt";
int numFiles = 0;

void copyFile(const string& sourceFile, const string& destinationFile) {
   ifstream in(sourceFile, ios::binary);
   ofstream out(destinationFile, ios::binary);
   out << in.rdbuf();
   in.close();
   out.close();
}

void readAndWriteRuns() {
   ifstream inFile(inputFilename);
   if (!inFile) {
       cerr << "Cannot open input file." << endl;
       return;
   }

   while (!inFile.eof()) {
       vector<int> data(chunkSize);
       int itemsRead = 0;
       while (itemsRead < chunkSize && inFile >> data[itemsRead]) {
           itemsRead++;
       }
       if (itemsRead == 0 )break; // 代表数据已经全部读取完毕

       sort(data.begin(), data.end());

       stringstream ss;
       ss << tempDir << "/temp_merged_" << 0 << "_" << numFiles++ << ".txt";
       ofstream outFile(ss.str());
       for (int i = 0; i < itemsRead; ++i) {
           outFile << data[i] << '\n';
       }
       outFile.close();
   }
   inFile.close();
}

void mergeTwoFiles(const string& file1, const string& file2, const string& outputFile) {
   ifstream inFile1(file1);
   if (!inFile1) {
       cerr << "Cannot open file1." << endl;
       return;
   }
   ifstream inFile2(file2);
   if (!inFile2) {
       cerr << "Cannot open file2." << endl;
       return;
   }

   ofstream outFile(outputFile);
   int num1, num2;
   inFile1 >> num1 && inFile2 >> num2;

   while (true) {
       if (num1 <= num2) {
           outFile << num1 << '\n';
           if (inFile1 >> num1) {
               continue;
           }
           else {
               do {
                   outFile << num2 << '\n';
               } while (inFile2 >> num2);
               break;
           }
       }
       else {
           outFile << num2 << '\n';
           if (inFile2 >> num2) {
               continue;
           }
           else {
               do {
                   outFile << num1 << '\n';
               } while (inFile1 >> num1);
               break;
           }
       }
   }

   inFile1.close();
   inFile2.close();
   outFile.close();
}

void multiPhaseMerge() {
   int phase = 0;
   do {
       for (int i = 0; i < numFiles; i += 2) {
           if (i + 1 < numFiles) {
               stringstream ss1, ss2, ss3;
               ss1 << tempDir << "/temp_merged_" << phase << "_" << i << ".txt";
               ss2 << tempDir << "/temp_merged_" << phase << "_" << i + 1 << ".txt";
               ss3 << tempDir << "/temp_merged_" << (phase + 1) << "_" << i / 2 << ".txt";
               mergeTwoFiles(ss1.str(), ss2.str(), ss3.str());
           }
           else {
               stringstream ss, ss_target;
               ss << tempDir << "/temp_merged_" << phase << "_" << i << ".txt";
               ss_target << tempDir << "/temp_merged_" << (phase + 1) << "_" << i / 2 << ".txt";
               copyFile(ss.str(), ss_target.str());
           }
       }
       numFiles = (numFiles + 1) / 2;
       phase++;
   } while (numFiles > 1);

   stringstream ss, s_dest;
   ss << tempDir << "/temp_merged_" << phase << "_" << 0 << ".txt";
   s_dest << outputFilename;
   copyFile(ss.str(), s_dest.str());
}

int main() {
   if (!filesystem::exists(tempDir)) {
       filesystem::create_directory(tempDir);
   }

   readAndWriteRuns();
   multiPhaseMerge();

   cout << "External sort complete. Sorted data is in '" << outputFilename << "'." << endl;

   return 0;
}
