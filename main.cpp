#include <pthread.h>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <map>
using namespace std;

pthread_mutex_t read_mutex;
pthread_mutex_t write_mutex;

int read_count;
vector <set <int> > edges;
map <unsigned int, int> node_map;

void read_input() {
	char line[100];
	while (gets(line)) {
		if (line[0] == 'S') {
			return;
		}
		unsigned int x, y;
		sscanf (line, "%u %u", &x &y);
		if (node_map.find(x) == node_map.end()) {
			int s = node_map.size();
			node_map[x] = s;
			set <int> new_set;
			edges.push_back(new_set);
		}
		if (node_map.find(y) == node_map.end()) {
			int s = node_map.size();
			node_map[y] = s;
			set <int> new_set;
			edges.push_back(new_set);
		}
		edges[node_map[x]].insert(node_map[y]);
	}
}

int main () {

	read_input();
	char line[100];

	while (gets(line)) {
		if (line[0] == 'F') {
			break;
		}
		char command;
		int x, y;
		sscanf (line, "%c %d %d", &command, &x, &y);
		if (command == 'Q') {

		} else if (command == 'A') {

		} else if (command == 'D') {

		} else {
			
		}
	}

	return 0;
}