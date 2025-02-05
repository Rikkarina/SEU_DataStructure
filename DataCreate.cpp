//#include <iostream>
//#include <fstream>
//#include <random>
//#include <vector>
//using namespace std;
//const int dataSize = 10000000; 
//const string outputFilename = "large_data.txt";
//
//int main() {
//	// ����һ��ofstream��������д������
//	ofstream outFile(outputFilename);
//
//	if (!outFile) {
//		cerr << "Cannot open output file." << endl;
//		return -1;
//	}
//
//	// ����һ�������������
//	random_device rd; // ����һ�����������
//	mt19937 gen(rd()); // ��������ʹ�ã�ĳ����������һ��������ÿ�ζ��������������������
//	uniform_int_distribution<> dis(1, 10000000); // �������ݷ�Χ��1��10000000֮��
//
//	for (int i = 0; i < dataSize; ++i) {
//		int randomValue = dis(gen);
//		outFile << randomValue << '\n';
//	}
//
//	outFile.close(); // �ر��ļ�
//	cout << "Data file generated with " << dataSize << " entries." << endl;
//	return 0;
//}