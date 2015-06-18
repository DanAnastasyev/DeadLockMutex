#pragma once
#include "Synchronizer.h"
#include <vector>
#include <memory>
#include <map>
#include <stack>

enum TColor {
	C_White,	//Вершину не посещали
	C_Grey,		//Вершина обрабатывается
	C_Black		//Обработаны
};

struct VertexStats {
	TColor color;
	long long parent;
    boost::mutex mutex;
};

// Граф зависимостей между мьютексами и удерживающими их ресурсами
class WaitForGraph {
public:
    WaitForGraph();
    ~WaitForGraph();
	
	void addEdge(long long from, long long to);
	void deleteEdge(long long from, long long to);
private:
	friend class Synchronizer;

	static boost::mutex graphMutex;
    static std::map<long long, std::pair<std::vector<long long>, VertexStats>> data;
};