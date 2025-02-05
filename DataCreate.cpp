//#include <iostream>
//#include <fstream>
//#include <random>
//#include <vector>
//using namespace std;
//const int dataSize = 10000000; 
//const string outputFilename = "large_data.txt";
//
//int main() {
//	// 创建一个ofstream对象，用于写入数据
//	ofstream outFile(outputFilename);
//
//	if (!outFile) {
//		cerr << "Cannot open output file." << endl;
//		return -1;
//	}
//
//	// 创建一个随机数生成器
//	random_device rd; // 生成一个随机的种子
//	mt19937 gen(rd()); // 搭配种子使用，某种意义上是一个函数，每次都会根据种子来生成数据
//	uniform_int_distribution<> dis(1, 10000000); // 假设数据范围在1到10000000之间
//
//	for (int i = 0; i < dataSize; ++i) {
//		int randomValue = dis(gen);
//		outFile << randomValue << '\n';
//	}
//
//	outFile.close(); // 关闭文件
//	cout << "Data file generated with " << dataSize << " entries." << endl;
//	return 0;
//}