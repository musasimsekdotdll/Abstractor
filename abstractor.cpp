
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <queue>
#include <mutex>


// static queue<String> q;

using namespace std;
static queue<string> q;
static bool is_queue_empty = false;
static mutex mtx;

static vector<bool> turn;


void abstract(char name, int id) {

	int counter = 0;
	vector<string> handled;
	while(!is_queue_empty) {
		string s = "";
		mtx.lock();

		if(turn[id]==false) {
			mtx.unlock();
			continue;
		}

		if(q.empty()) {
			is_queue_empty = true;
			mtx.unlock();
			break;
		}

		s = q.front();
		q.pop();
		
		// cout << "Thread " << name << " handles " << s << endl;
		
		turn[id] = false;
		turn[(id+1) % turn.size()] = true;

		mtx.unlock();
		
		handled.push_back(s);
		counter++;

	}

	cout << "Thread " << name << " handles " << counter << " elements" << endl;

}


int main(int argc, char const *argv[])
{

	string input_file = argv[1];
	string output_file = argv[2];

	ifstream in(input_file);
	ofstream out(output_file);

	if(in.is_open() && out.is_open()) {
		vector<thread> ts;
		char name = 'A';
		int num_of_threads = 5;

		for(int i=0; i<41; i++) {
			q.push("" + i);
		}

		turn.push_back(true);
		for(int i=0; i<num_of_threads-1; i++) {
			turn.push_back(false);
		}


		for(int i=0; i<num_of_threads; i++) {		
			thread t(abstract, name, i);
	//		ts.emplace_back(t);
			ts.push_back(move(t));
			name++;
		}

		for(auto& t : ts) {
			t.join();
		}
	
	} else {
		cout << "Error occured opening files!" << endl;
	}

	
	// cout << "End of threads" << endl;
	return 0;
}