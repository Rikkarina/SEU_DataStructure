#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <climits>

using namespace std;

struct treeNode {
    int value;
    bool invalid = false;
};

class LoserTree {
private:
    vector<int> tree; // 仅记录key, tree[0]为胜出者
    vector<treeNode> leaves; 
    int numLeaves;
    int currentMax = INT_MIN;
    int waitingKey = -1;
    int numInvalidNode = 0;

    bool isReadyToRun = false;

    int leavesParent(int i) {
        return (i + numLeaves) / 2;
    }
    int parent(int i) {
        if (i == 0)return -1;
        return i / 2;
    }

public:
    LoserTree(int numLeaves, queue<int>&data) : numLeaves(numLeaves), tree(numLeaves, -1), leaves(numLeaves) {
        if (data.size() >= numLeaves) {
            for (int i = 0; i < numLeaves; i++) {
                int value = data.front();
                data.pop();
                leaves[i].value = value;
            }
            isReadyToRun = true;
            initialize();
        }
        else {
            waitingKey = data.size();
            for (int i = 0; data.size(); i++) {
                int value = data.front();
                data.pop();
                leaves[i].value = value;
            }
        }
    }

    LoserTree(int numLeaves, vector<queue<int>>& data) : numLeaves(numLeaves), tree(numLeaves, -1), leaves(numLeaves) {
        for (int i = 0; i < numLeaves; i++) {
            int value = data[i].front();
            data[i].pop();
            // 这里可以修改，看看是否可以检查data为空，但初始化应该是不会出现这个状态才对，可以设置一个报错
            leaves[i].value = value;
        }

        // 初始化非叶子节点
        for (int i = numLeaves - 1; i >= 0; i--) {
            int winnerKey = i;
            int p = leavesParent(winnerKey);
            while (p >= 0) {
                if (tree[p] == -1) {
                    tree[p] = winnerKey;
                    break;
                }
                else {
                    int oldLoserValue = leaves[tree[p]].value;
                    if (leaves[winnerKey].value > oldLoserValue && !leaves[tree[p]].invalid) {
                        swap(winnerKey, tree[p]);
                    }
                    p = parent(p);
                }
            }

        }
    }

    bool isReady() {
        return isReadyToRun;
    }

    bool fillLeaves(queue<int>&data) {
        if (isReadyToRun)return true;
        while(data.size() && waitingKey < numLeaves) {
            int value = data.front();
            data.pop();
            leaves[waitingKey].value = value;
            waitingKey++;
        }
        if (waitingKey == numLeaves) {
            isReadyToRun = true;
            waitingKey = -1;
            initialize();
            return true;
        }
        return false;
    }

    void initialize() {
        // 初始化非叶子节点
        for (int i = numLeaves - 1; i >= 0; i--) {
            int winnerKey = i;
            int p = leavesParent(winnerKey);
            while (p >= 0) {
                if (tree[p] == -1) {
                    tree[p] = winnerKey;
                    break;
                }
                else {
                    int oldLoserValue = leaves[tree[p]].value;
                    if (leaves[winnerKey].value > oldLoserValue && !leaves[tree[p]].invalid) {
                        swap(winnerKey, tree[p]);
                    }
                    p = parent(p);
                }
            }

        }
    }

    // 插入元素并维护败者树
    void insert(int value, bool isValid = true) { // 返回叶子是否均无效，代表是否可以开始生成下一个顺串
        if (waitingKey == -1)
            cerr << "Insert Failed: No free node" << endl;
        leaves[waitingKey].value = value;
        if (value < currentMax || !isValid){
            leaves[waitingKey].invalid = true;
            numInvalidNode++;
        }

        int winnerKey = waitingKey;
        int p = leavesParent(winnerKey);
        while (p >= 0) {
            if (tree[p] == -1) {
                tree[p] = winnerKey;
                break;
            }

			treeNode loserNode = leaves[tree[p]];
			if ((leaves[winnerKey].invalid && !loserNode.invalid) || // 败者有效而胜者无效时
				(leaves[winnerKey].value > loserNode.value && !(loserNode.invalid ^ leaves[winnerKey].invalid))) { // 两者状态相同时需要比较大小 
				swap(winnerKey, tree[p]);
			}
			p = parent(p);
        }

        // 在线程版本中，这个不需要了
        if (numInvalidNode == numLeaves) { // 当所有节点均无效时
            numInvalidNode = 0;
            setValid();
        }
    }

    // 插入元素并维护败者树
    bool insertForMerge(int value, bool isValid = true) { // 返回叶子是否均无效，代表是否可以开始生成下一个顺串
        if (waitingKey == -1)
            cerr << "Insert Failed: No free node" << endl;
        leaves[waitingKey].value = value;
        if (value < currentMax || !isValid) {
            leaves[waitingKey].invalid = true;
            numInvalidNode++;
        }

        int winnerKey = waitingKey;
        int p = leavesParent(winnerKey);
        while (p >= 0) {
            if (tree[p] == -1) {
                tree[p] = winnerKey;
                break;
            }

            treeNode loserNode = leaves[tree[p]];
            if ((leaves[winnerKey].invalid && !loserNode.invalid) || // 败者有效而胜者无效时
                (leaves[winnerKey].value > loserNode.value && !(loserNode.invalid ^ leaves[winnerKey].invalid))) { // 两者状态相同时需要比较大小 
                swap(winnerKey, tree[p]);
            }
            p = parent(p);
        }

        // 在线程版本中，这个不需要了
        if (numInvalidNode == numLeaves) { // 当所有节点均无效时
            return true;
        }
        return false;
    }
    void setValid() {
        for (auto& leaf : leaves) {
            leaf.invalid = false;
        }
        currentMax = INT_MIN;
    }
    // 获取当前最大值
    int popMax() {
        int maxKey = tree[0];
        int maxValue = leaves[maxKey].value;
        currentMax = max(maxValue, currentMax);
        waitingKey = maxKey;
        tree[0] = -1;
        return leaves[maxKey].value;
    }

    pair<int, int> popMaxForMerge() {
        int maxKey = tree[0];
        int maxValue = leaves[maxKey].value;
        currentMax = max(maxValue, currentMax);
        waitingKey = maxKey;
        tree[0] = -1;
        return make_pair(leaves[maxKey].value, waitingKey);
    }
};