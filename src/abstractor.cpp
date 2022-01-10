
#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <queue>
#include <mutex>
#include <bits/stdc++.h>
#include <unordered_map>
#include <set>


// static queue<String> q;

using namespace std;

// static vector<bool> turn;



struct similarity_info {
	string file_name;
	float similarity;
};

struct Compare { 
    bool operator()(similarity_info const& s1, similarity_info const& s2) 
    { 
    	if(s1.similarity <= s2.similarity) {
    		return true;
    	} else {
    		return false;
    	}
    } 
};


static queue<string> q;
static bool is_queue_empty = false;
static mutex mtx;
static unordered_map<string, vector<string>> summaries;
static priority_queue<similarity_info, vector<similarity_info>, Compare> results;
static ofstream out;



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


void calculateJaccard(const string& check_file, const vector<string>& queries) {
	vector<string> words;
	string word;
	string path = "../abstracts/" + check_file; 

	fstream file;
	file.open(path.c_str());

	while(file >> word) {
		words.push_back(word);
	}

	bool added = false;

	int counter = 0;
	int line_number = 0;
	string line = "";

	char* intersection_array = new char[queries.size()];
	for(int i=0; i<queries.size(); i++) {
		intersection_array[i] = '0';
	}

	bool checked_before = false;
	for(int x=0; x<words.size(); x++) {
		if(words[x]==".") {
			line_number++;
			if(added) {
				line += ". ";

				mtx.lock();
				if(line_number==1 && summaries.count(check_file)!=0) {
					checked_before = true;
				}

				if(!checked_before) {				
					summaries[check_file].push_back(line);
				}

				mtx.unlock();


			}
			line = "";
			added = false;
			continue;
		}

		for(int y=0; y<queries.size(); y++) {
			if(words[x] == queries[y]) {
				counter++;
				added = true;
				intersection_array[y] = '1';
				break;
			}
		}

		line += words[x] + " ";
	}

	int intersection = 0;
	for(int i=0; i<queries.size(); i++) {
		intersection += (intersection_array[i] - 48);
	}

	set<string> uni(words.begin(), words.end());
	float res = ((float) intersection) / (uni.size() + queries.size() - intersection);

	similarity_info* new_sim = new similarity_info();
	new_sim->file_name = check_file;
	new_sim->similarity = res;


	mtx.lock();
	results.push(*new_sim);
	mtx.unlock();

}


void abstract(char name, int id, vector<string> query_list) {

	while(!is_queue_empty) {
		string s = "";
		mtx.lock();


		if(q.empty()) {
			is_queue_empty = true;
			mtx.unlock();
			break;
		}

		s = q.front();
		q.pop();
		
		out << "Thread " << name << " is calculating " << s << endl;
		
		// turn[id] = false;
		// turn[(id+1) % turn.size()] = true;

		mtx.unlock();

		sleep(0.03);
		calculateJaccard(s, query_list);

	}

}


int main(int argc, char const *argv[])
{

	string input_file = argv[1];
	string output_file = argv[2];

	ifstream in(input_file);
	out.open(output_file);

	if(in.is_open() && out.is_open()) {

		string int_line;
		getline(in, int_line);
		vector<string> int_list;
		splitString(int_line, " ", int_list);

		int t, a, n;
		for(int i=0; i<int_list.size(); i++) {
			if(!isNumber(int_list[i])) {

				throw invalid_argument("Input is not in correct form");
				return -1;
			}
		}

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
		for(int i=0; i<t; i++) {		
			thread t(abstract, name, i, match_list);
			ts.push_back(move(t));
			name++;
		}


		for(auto& t : ts) {
			t.join();
		}

		out << "###" << endl;
		
		for(int x=0; x<n; x++) {
			out << "Result " << x+1 << ":" << endl;

			similarity_info res = results.top();
			results.pop();

			out << "File: " << res.file_name << endl;
			out << "Score: " << fixed << setprecision(4) << res.similarity << endl;
			out << "Summary: ";

			for(auto element : summaries.at(res.file_name)) {
				out << element;
			}
			out << "\n###" << endl;
		}
	
	} else {
		cout << "Error occured opening files!" << endl;
	}

	return 0;
}