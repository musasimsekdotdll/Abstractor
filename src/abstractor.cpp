
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


using namespace std;


/* A struct which will help to find the correct files to print. It has a string variable corresponding to name of the abstractor file,
  and a float which will represents the similarity of the file with the given query. After each thread completed scanning an abstract file,
  an instance of similarity_info is created and pushed to a maxheap so that at the end, the top element will be the most similar abstract file.
 */
struct similarity_info {
	string file_name;
	float similarity;
};


/* Compare struct should be overwrited for similarity_info since a priority queue will accept elements from that struct */
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




static queue<string> q;		// A queue which will contain the names of the abstract files from which each 
							//available thread will take an element and begins to the abstraction

static bool is_queue_empty = false; // indicates that whether there still is files that should be scanned
static mutex mtx;		// a mutex lock so that the filename queue, the file summaries map and similarity_info 
						//priority_queue will be accessed mutually exclusively

static unordered_map<string, vector<string>> summaries; // a map structure which will links the file names to the 
														//sentence in that file which contains at least one element from the query list

static priority_queue<similarity_info, vector<similarity_info>, Compare> results; // a maxheap which will sorts abstract files with 
																				  //respect to their similarity proportions

static ofstream out;



/* A basic method which will splits the given string only when the given pattern is whitespace, a dot etc */
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


/* A basic funtion for determining if a given string can be transformed into a number without any error */
bool isNumber(string& s) {

	for(int i=0; i<s.length(); i++) {
		if(!isdigit(s[i])) {
			return false;
		}
	}

	return true;
}



/* A method calculating Jaccard similarity with respect to the given formulas in the description*/
void calculateJaccard(const string& check_file, const vector<string>& queries) {

	vector<string> words;		// a vector for all the words in the file
	string path = "../abstracts/" + check_file;		// actual path of the given abstract file

	fstream file;
	file.open(path.c_str());

	string word;
	while(file >> word) {
		words.push_back(word);
	}



	bool added = false;		// a boolean value representing whether the currently scanning sentence is added to the summary or not

	int counter = 0;		// a counter for the total number of occurence of queries in the abstract file
	int line_number = 0;	// the number of lines in the file
	string line = "";		// the currently scanning sentence itself


	char* intersection_array = new char[queries.size()];		 //an array which will contain at every index 0 or 1 indicating that ith element of the 
															    //query list is used in the abstract file, this information will be useful when we 
															   //calculate the result since the denominator will be the intersection of two sets and 
															  //each element whould be included once 
	/* all indexes should be initialized to 0 */
	for(int i=0; i<queries.size(); i++) {
		intersection_array[i] = '0';
	}


	bool checked_before = false;		// tells whether we checked the given abstract file before 
	for(int x=0; x<words.size(); x++) {
		if(words[x]==".") {		// indicates that we hit the end of the sentence
			line_number++;		// increment the total number of the lines
			if(added) {			// this conditional body will be ran only when at leat one of the query words is found in the sentence
				line += ". ";

				mtx.lock();
				if(line_number==1 && summaries.count(check_file)!=0) {		 //if it is the end of first line and there is an entry in the summaries 
																			//map which is not empty, that means another thread checked that file before
					checked_before = true;								   //the current thread did
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

	/* Count how many entries of the query is there in the given abstract file */
	int intersection = 0;
	for(int i=0; i<queries.size(); i++) {
		intersection += (intersection_array[i] - 48);
	}


	/* No words should be included twice in the nominator, also. Therefore, the vector for all the words can be transformed to a set*/
	set<string> uni(words.begin(), words.end());

	float res = ((float) intersection) / (uni.size() + queries.size() - intersection);		// the similarity calculation


	/* create an instance of similarity_info struct and push that into the maxheap*/
	similarity_info* new_sim = new similarity_info();
	new_sim->file_name = check_file;
	new_sim->similarity = res;


	mtx.lock();
	results.push(*new_sim);
	mtx.unlock();

	/* deallocate the resources you used dynamically*/
	free(intersection_array);
	free(new_sim);

}

/* This method is where threads are born, they take an element from the queue, which contains the name of the abstract file to be scanned. 
  Then, they go to calculation part and joins to the main thread after that
*/
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
		
		mtx.unlock();

		sleep(0.08);
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


		/* All threads must be in a list so that main thread can trace them */
		vector<thread> ts;
		char name = 'A';
		for(int i=0; i<t; i++) {		
			thread t(abstract, name, i, match_list);
			ts.push_back(move(t));
			name++;
		}

		/* The main thread will wait until all threads are done and terminated */
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