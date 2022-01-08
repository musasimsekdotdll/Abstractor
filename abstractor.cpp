
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


void splitString(string text, string condition, vector<string>& list) {

	int index = 0;
	string s="";
	for(int i=0; i<text.length()-condition.length()+1; i++) {
		if(text.substr(i, condition.length()) == condition) {
			list.push_back(s);
			i = i + condition.length()-1;
			index = i+1;
			s = "";
		} else {
			s += text[i];
		}
	}

	if(s != "") {
		list.push_back(s);
	}
}

bool isNumber(string& s) {

	for(int i=0; i<s.length(); i++) {
		if(!isdigit(s[i])) {
			return false;
		}
	}

	return true;
}


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
		
		cout << "Thread " << name << " is calculating " << s << endl;
		
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

		string int_line;
		getline(in, int_line);
		vector<string> int_list;
		splitString(int_line, " ", int_list);
		cout << "int_line is splitted, size: " << int_list.size() << endl;

		int t, a, n;
		for(int i=0; i<int_list.size(); i++) {
			if(!isNumber(int_list[i])) {

				throw invalid_argument("Input is not in correct form");
				return -1;
			}
		}

		cout << "Numbers are taken" << endl;
		t = stoi(int_list[0]);
		a = stoi(int_list[1]);
		n = stoi(int_list[2]);


		string match_line;
		getline(in, match_line);
		vector<string> match_list;
		splitString(match_line, " ", match_list);


		string input_line;
		while( getline(in, input_line) ) {
			q.push(input_line);
		}


		vector<thread> ts;
		char name = 'A';

		turn.push_back(true);
		for(int i=0; i<t-1; i++) {
			turn.push_back(false);
		}


		for(int i=0; i<t; i++) {		
			thread t(abstract, name, i);
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