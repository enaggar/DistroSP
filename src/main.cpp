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

pthread_mutex_t read_mutex, write_mutex, index_mutex, cost_mutex, count_mutex;
pthread_cond_t count_max;
pthread_attr_t pt_attr;
int thread_count;
int thread_max;
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
	pthread_mutex_lock (&count_mutex);
	if (thread_count == thread_max) {
		pthread_cond_signal(&count_max);
	}
	thread_count--;
	pthread_mutex_unlock (&count_mutex);
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
	pthread_mutex_lock (&count_mutex);
	if (thread_count == thread_max) {
		pthread_cond_signal(&count_max);
	}
	thread_count--;
	pthread_mutex_unlock (&count_mutex);
	pthread_exit(NULL);
}

pthread_t* new_thread (const pthread_attr_t *attr, void *(*start_routine) (void*), void *arg) {
	pthread_t *thread = (pthread_t*)malloc(sizeof(pthread_t));
	//cout << "Open new thread\n";
	//if (start_routine == ssp) {
	//	printf ("SSP called from remove\n");
	//}
	//cout << "Prelock\n";
	pthread_mutex_lock (&count_mutex);
	//cout << "After Lock\n";
	//sleep (3);
	//cout << "wait over\n";
	if (thread_count == thread_max) {
		fprintf (log_file, "Attempting to start thread on max threshold\n");
		//cout << "Attempting to start thread for on max threshold\n";
		//cout << "al3ab baleeh\n";
		int st = pthread_create (thread, attr, start_routine, arg);
		if (!st) {
			thread_max ++;
		}
		while (st) {
			fprintf (log_file, "Failed to push max threshold to start thread, error = %d\n", st);
			//printf ("Failed to push max threshold to start thread, error = %d\n", st);
			pthread_cond_wait(&count_max, &count_mutex);
			st = pthread_create (thread, attr, start_routine, arg);
		}
		thread_count ++;
		fprintf (log_file, "Successfuly started thread\n");
	} else {
		fprintf (log_file, "Attempting to start thread below max threshold\n");
		//cout << "Attempting to start thread below max threshold\n";
		int st = pthread_create (thread, attr, start_routine, arg);
		//cout << st << endl;
		while (st) {
			thread_max = thread_count;
			fprintf (log_file, "Unable to start thread for although below threshold, error = %d\n", st);
			//printf ("Unable to start thread for although below threshold, error = %d\n", st);
			pthread_cond_wait(&count_max, &count_mutex);
			st = pthread_create (thread, attr, start_routine, arg);
		}
		thread_count ++;
		fprintf (log_file, "Successfuly started thread\n");
		//printf ("Successfuly started thread\n");
	}
	pthread_mutex_unlock (&count_mutex);
	//cout << "Success\n";
	//printf ("%p\n", thread);
	return thread;
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
	pthread_mutex_lock (&count_mutex);
	if (thread_count == thread_max) {
		pthread_cond_signal(&count_max);
	}
	thread_count--;
	pthread_mutex_unlock (&count_mutex);
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

					pthread_mutex_lock (&index_mutex);
					pthread_t *pt = new_thread (&pt_attr, ssp, (void*)&ind);
					void *status;
					fprintf (log_file, "Attampting to join thread recompute ssp for triplet: (%d,%d,%d)\n", x, y, ind);
					int st = pthread_join(*pt, &status);
					while (st) {
						fprintf (log_file, "Unable to join thread recompute ssp for triplet: (%d,%d,%d), error = %d\n", x, y, ind, st);
						st = pthread_join(*pt, &status);
					}
					//printf ("%p\n", pt);
					free (pt);
					fprintf (log_file, "Successfuly joined thread recompute ssp for triplet: (%d,%d,%d)\n", x, y, ind);
					goto a;
				}
			}
		}
	}
	a:
	//printf ("Effect checked\n");
	pthread_mutex_lock (&count_mutex);
	if (thread_count == thread_max) {
		//printf ("Attempting to send signal\n");
		pthread_cond_signal(&count_max);
		//printf ("Signal sent\n");
	}
	thread_count--;
	//printf ("Count decremented\n");
	pthread_mutex_unlock (&count_mutex);
	//printf ("Ready to exit %d\n", ind);
	pthread_exit(NULL);
}

int main () {

	thread_max = 1;

	log_file = fopen ("process.log", "w");

	read_input();
	unordered_map <unsigned int, int> :: iterator it;

	vector <pthread_t*> child_threads;
	pthread_attr_init(&pt_attr);
	pthread_attr_setdetachstate(&pt_attr, PTHREAD_CREATE_JOINABLE);

	pthread_mutex_init (&read_mutex, NULL);
	pthread_mutex_init (&write_mutex,NULL);
	pthread_mutex_init (&index_mutex,NULL);
	pthread_mutex_init (&cost_mutex, NULL);
	pthread_mutex_init (&count_mutex,NULL);
	pthread_cond_init  (&count_max,  NULL);
	for (it = node_map.begin(); it != node_map.end(); it++) {
		int ret = pthread_mutex_lock (&index_mutex);
		int ind = it->second;
		//printf ("Initializing for node: %d\n", ind);
		pthread_t *pt = new_thread (&pt_attr, ssp, &ind);
		//printf ("Thead run for node: %d\n", ind);
		child_threads.push_back(pt);
	}

	/*for (int i=0;i<child_threads.size();i++) {
		void *status;
		fprintf (log_file, "Attempting to join init thread for node: %d\n", i);
		int st = pthread_join (*child_threads[i], &status);
		while (st) {
			fprintf (log_file, "Unable to join init thread for node: %d, error = %d\n", i, st);
			st = pthread_join (*child_threads[i], &status);
		}
		free (child_threads[i]);
		fprintf (log_file, "Successfuly started init thread for node: %d\n", i);
	}*/

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
			pthread_t *pt = new_thread (&pt_attr, query, (void*)&p);
		} else if (command == 'A') {
			lock_for_write ();
			if (edges[node_map[x]].find(node_map[y]) == edges[node_map[x]].end()) {
				edges[node_map[x]].insert(node_map[y]);
				child_threads.clear();
				for (it = node_map.begin(); it != node_map.end(); it++) {
					pthread_mutex_lock (&index_mutex);
					triplet t = triplet (node_map[x], node_map[y], it->second);
					pthread_t* pt = new_thread (&pt_attr, add_edge, (void*)&t);
					child_threads.push_back(pt);
				}
				for (int i=0;i<child_threads.size();i++) {
					void* status;
					fprintf (log_file, "Attempting to join thread for add edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], i);
					int st = pthread_join(*child_threads[i], &status);
					while (st) {
						fprintf (log_file, "Unable to join thread for add edge triplet: (%d,%d,%d), error = %d\n", node_map[x], node_map[y], i, st);
						st = pthread_join(*child_threads[i], &status);
					}
					free(child_threads[i]);
					fprintf (log_file, "Successfuly joined thread for add edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], i);
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
					pthread_t *pt = new_thread (&pt_attr, remove_edge, (void*)&t);
					child_threads.push_back(pt);
				}
				for (int i=0;i<child_threads.size();i++) {
					void* status;
					fprintf (log_file, "Attempting to join thread for remove edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], i);
					int st = pthread_join(*child_threads[i], &status);
					while (st) {
						fprintf (log_file, "Unable to join thread for remove edge triplet: (%d,%d,%d), error = %d\n", node_map[x], node_map[y], i, st);
						int st = pthread_join(*child_threads[i], &status);
					}
					free (child_threads[i]);
					fprintf (log_file, "Successfuly joined thread for remove edge triplet: (%d,%d,%d)\n", node_map[x], node_map[y], i);
				}
			}
			unlock_write ();
		} else if (command == 'F') {
			break;
		} else {
			printf ("INVALID COMMAND\n");
		}
	}

	pthread_attr_destroy(&pt_attr);
	fclose (log_file);

	return 0;
}