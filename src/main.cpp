#include <pthread.h>
#include <set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>
using namespace std;

typedef pair<int,int> pii;

class triplet {
public:
	int first, second, third;
	triplet() {
		first = second = third = 0;
	}
	triplet(int f, int s, int t) {
		first = f;
		second = s;
		third = t;
	}
	bool operator < (const triplet &a) const {
		if (first != a.first) {
			return first < a.first;
		} else if (second != a.second) {
			return second < a.second;
		}
		return third < a.third;
	}
	bool operator == (const triplet &a) const {
		return first == a.first && second == a.second && third == a.third;
	}
};

pthread_mutex_t read_mutex, write_mutex, index_mutex, cost_mutex;
int read_count;
FILE* log_file;

vector <unordered_set <int> > edges;
unordered_map <unsigned int, int> node_map;
vector <unordered_map <int, int> > cost_map;

void new_node (int x) {
	int s = node_map.size();
	node_map[x] = s;
	unordered_set <int> new_set;
	edges.push_back(new_set);
	//pthread_mutex_t mutex;
	//cost_mutex.push_back(mutex);
	unordered_map <int, int> cost;
	cost[s] = 0;
	cost_map.push_back(cost);
}

void read_input() {
	char line[100];
	while (gets(line)) {
		if (line[0] == 'S') {
			return;
		}
		unsigned int x, y;
		sscanf (line, "%u %u", &x, &y);
		if (node_map.find(x) == node_map.end()) {
			new_node(x);
		}
		if (node_map.find(y) == node_map.end()) {
			new_node(y);
		}
		edges[node_map[x]].insert(node_map[y]);
	}
}

void *query(void *input) {
	pii *p = (pii*)input;
	pthread_mutex_unlock(&index_mutex);
	if (cost_map[p->first].find(p->second) == cost_map[p->first].end()) {
		printf ("-1\n");
	} else {
		printf ("%d\n", cost_map[p->first][p->second]);
	}
	pthread_mutex_lock(&read_mutex);
	read_count--;
	if (read_count == 0) {
		pthread_mutex_unlock(&write_mutex);
	}
	pthread_mutex_unlock(&read_mutex);
	pthread_exit(NULL);
}

void *ssp (void *input) {
	int src = *((int*)input);
	//printf ("Inside %d\n", src);
	pthread_mutex_unlock(&index_mutex);

    //printf ("Computing SSP for %d\n", src);
    
	queue <int> q;
	unordered_set <int> vis;
	q.push(src);
	vis.insert(src);
	cost_map[src].clear();
	cost_map[src][src] = 0;

	unordered_set <int> :: iterator it;

	while (!q.empty()) {
		int c = q.front();
		q.pop();

		//printf ("%d %d\n", src, c);

		for (it = edges[c].begin(); it != edges[c].end(); it++) {
			int nxt = *it;
			if (vis.find(nxt) == vis.end()) {
				vis.insert(nxt);
				q.push(nxt);
				cost_map[src][nxt] = cost_map[src][c]+1;
			}
		}
	}
	pthread_exit(NULL);
}

void lock_for_write (){ 
	pthread_mutex_lock (&read_mutex);
	if (read_count == 0) {
		pthread_mutex_lock (&write_mutex);
	} else {
		pthread_mutex_unlock (&read_mutex);
		pthread_mutex_lock (&write_mutex);
		pthread_mutex_lock (&read_mutex);
	}	
}

void unlock_write (){
	pthread_mutex_unlock(&write_mutex);
	pthread_mutex_unlock(&read_mutex);
}

void *add_edge (void *input) {
	triplet *t = (triplet*)input;
	int x, y, ind;
	x = t->first, y = t->second, ind = t->third;
	pthread_mutex_unlock(&index_mutex);

	unordered_map <unsigned int, int> :: iterator it;

	if (cost_map[ind].find(x) != cost_map[ind].end()) {
		for (it = node_map.begin(); it != node_map.end(); it++) {
			int ind2 = it->second;
			if (cost_map[y].find(ind2) != cost_map[y].end()) {
				if (cost_map[ind].find(ind2) == cost_map[ind].end() || cost_map[ind][ind2] > cost_map[ind][x] + cost_map[y][ind2] + 1) {
					cost_map[ind][ind2] = cost_map[ind][x] + cost_map[y][ind2] + 1;
				}
			}
		}
	}
	pthread_exit(NULL);
}

void *remove_edge (void *input) {
	triplet *t = (triplet*) input;
	int x, y, ind;
	x = t->first, y = t->second, ind = t->third;
	pthread_mutex_unlock(&index_mutex);

    //printf ("Removing edge (%d, %d) effect for %d\n", x, y, ind);
    
	unordered_map <unsigned int, int> :: iterator it;

	for (it = node_map.begin(); it != node_map.end(); it++) {
		int ind2 = it->second;
		if (cost_map[ind].find(ind2) != cost_map[ind].end()) {
			if (cost_map[ind].find(x)  != cost_map[ind].end() &&
					cost_map[y].find(ind2) != cost_map[y].end()) {
				if (cost_map[ind][ind2] == cost_map[ind][x] + cost_map[y][ind2] + 1) {
					pthread_attr_t pt_attr;
					pthread_attr_init(&pt_attr);
					pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_JOINABLE);

					pthread_t pt;
					pthread_mutex_lock (&index_mutex);
					fprintf (log_file, "Attampting to start thread recompute ssp for triplet: (%d,%d,%d)\n", x, y, ind);
					int rc = pthread_create(&pt, NULL, ssp, (void*)&ind);
					while (rc) {
						fprintf (log_file, "Unable to start thread recompute ssp for triplet: (%d,%d,%d), error = %d\n", x, y, ind, rc);
						rc = pthread_create(&pt, NULL, ssp, (void*)&ind);
					}
					fprintf (log_file, "Successfuly started thread recompute ssp for triplet: (%d,%d,%d)\n", x, y, ind);
					void *status;
					fprintf (log_file, "Attampting to join thread recompute ssp for triplet: (%d,%d,%d)\n", x, y, ind);
					int st = pthread_join(pt, &status);
					while (st) {
						fprintf (log_file, "Unable to join thread recompute ssp for triplet: (%d,%d,%d), error = %d\n", x, y, ind, st);
						st = pthread_join(pt, &status);
					}
					fprintf (log_file, "Successfuly joined thread recompute ssp for triplet: (%d,%d,%d)\n", x, y, ind);
					pthread_exit(NULL);
				}
			}
		}
	}
	pthread_exit(NULL);
}

int main () {

	log_file = fopen ("process.log", "w");

	read_input();
	unordered_map <unsigned int, int> :: iterator it;

	vector <pthread_t> child_threads;
	pthread_attr_t pt_attr;
	pthread_attr_init(&pt_attr);
	pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutex_init (&read_mutex, NULL);
	pthread_mutex_init (&write_mutex,NULL);
	pthread_mutex_init (&index_mutex,NULL);
	pthread_mutex_init (&cost_mutex, NULL);
	for (it = node_map.begin(); it != node_map.end(); it++) {
		pthread_t pt;
		int ret = pthread_mutex_lock (&index_mutex);
		int ind = it->second;
		//printf ("Initializing for node: %d\n", ind);
		int st = pthread_create (&pt, NULL, ssp, (void*)&ind);
		fprintf (log_file, "Attempting to start init thread for node: %d\n", ind);
		while (st) {
			fprintf (log_file, "Unable to start init thread for node: %d, error = %d\n", ind, st);
			int st = pthread_create (&pt, NULL, ssp, (void*)&ind);
		}
		fprintf (log_file, "Successfuly started init thread for node: %d\n", ind);
		//printf ("Thead run for node: %d\n", ind);
		child_threads.push_back(pt);
	}

	pthread_attr_destroy(&pt_attr);

	for (int i=0;i<child_threads.size();i++) {
		void *status;
		fprintf (log_file, "Attempting to join init thread for node: %d\n", i);
		int st = pthread_join (child_threads[i], &status);
		while (st) {
			fprintf (log_file, "Unable to join init thread for node: %d, error = %d\n", i, st);
			st = pthread_join (child_threads[i], &status);
		}
		fprintf (log_file, "Successfuly started init thread for node: %d\n", i);
	}

	printf ("R\n");

	char line[100];

	while (gets(line)) {
		if (line[0] == 'F') {
			break;
		}
		char command;
		int x, y;
		sscanf (line, "%c %u %u", &command, &x, &y);
		if (node_map.find(x) == node_map.end()) {
			new_node(x);
		}
		if (node_map.find(y) == node_map.end()) {
			new_node(y);
		}
		if (command == 'Q') {
			pthread_mutex_lock(&read_mutex);
			if (read_count == 0) {
				pthread_mutex_lock(&write_mutex);
			}
			read_count ++;
			pthread_mutex_unlock(&read_mutex);
			pthread_mutex_lock(&index_mutex);
			pii p = pii (node_map[x], node_map[y]);
			pthread_t pt;
			int rc = pthread_create (&pt, NULL, query, (void*)&p);
			fprintf (log_file, "Attempting to start thread for query: (%d,%d)\n", node_map[x], node_map[y]);
			while (rc) {
				fprintf (log_file, "Unable to start thread for query: (%d,%d), error = %d\n", node_map[x], node_map[y], rc);
				rc = pthread_create (&pt, NULL, query, (void*)&p);
			}
			fprintf (log_file, "Successfuly started thread for query: (%d,%d)\n", node_map[x], node_map[y]);
		} else if (command == 'A') {
			lock_for_write ();
			if (edges[node_map[x]].find(node_map[y]) == edges[node_map[x]].end()) {
				edges[node_map[x]].insert(node_map[y]);
				child_threads.clear();
				for (it = node_map.begin(); it != node_map.end(); it++) {
					pthread_mutex_lock (&index_mutex);
					triplet t = triplet (node_map[x], node_map[y], it->second);
					pthread_t pt;
					int rc = pthread_create (&pt, NULL, add_edge, (void*)&t);
					fprintf (log_file, "Attempting to start thread for add edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
					while (rc) {
						fprintf (log_file, "Unable to start thread for add edge triplet: (%d,%d,%d), error = %d\n", node_map[x], node_map[y], it->second, rc);
						rc = pthread_create (&pt, NULL, add_edge, (void*)&t);
					}
					fprintf (log_file, "Successfuly started thread for add edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
					child_threads.push_back(pt);
					for (int i=0;i<child_threads.size();i++) {
						void* status;
						fprintf (log_file, "Attempting to join thread for add edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
						int st = pthread_join(child_threads[i], &status);
						while (st) {
							fprintf (log_file, "Unable to join thread for add edge triplet: (%d,%d,%d), error = %d\n", node_map[x], node_map[y], it->second, st);
							st = pthread_join(child_threads[i], &status);
						}
						fprintf (log_file, "Successfuly joined thread for add edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
					}
				}
			}
			unlock_write();
		} else if (command == 'D') {
			lock_for_write ();
			if (edges[node_map[x]].find(node_map[y]) != edges[node_map[x]].end()) {
				edges[node_map[x]].erase(node_map[y]);
				child_threads.clear();
				for (it = node_map.begin(); it != node_map.end(); it++) {
					pthread_mutex_lock(&index_mutex);
					triplet t = triplet(node_map[x], node_map[y], it->second);
					pthread_t pt;
					int rc = pthread_create(&pt, NULL, remove_edge, (void*)&t);
					fprintf (log_file, "Attempting to start thread for remove edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
					while (rc) {
						fprintf (log_file, "Unable to start thread for remove edge triplet: (%d,%d,%d), error = %d\n", node_map[x], node_map[y], it->second, rc);
						rc = pthread_create(&pt, NULL, remove_edge, (void*)&t);
					}
					fprintf (log_file, "Successfuly started thread for remove edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
					child_threads.push_back(pt);
					for (int i=0;i<child_threads.size();i++) {
						void* status;
						fprintf (log_file, "Attempting to join thread for remove edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
						int st = pthread_join(child_threads[i], &status);
						while (st) {
							fprintf (log_file, "Unable to join thread for remove edge triplet: (%d,%d,%d), error = %d\n", node_map[x], node_map[y], it->second, st);
							int st = pthread_join(child_threads[i], &status);
						}
						fprintf (log_file, "Successfuly joined thread for remove edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], it->second);
					}
				}
			}
			unlock_write ();
		} else if (command == 'F') {
			break;
		} else {
			printf ("INVALID COMMAND\n");
		}
	}

	fclose (log_file);

	return 0;
}