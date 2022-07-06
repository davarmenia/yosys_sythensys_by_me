#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <cstring>
#include <stdlib.h>
#include <regex>
#include <thread>
#include <chrono>
#include <stdio.h>
#include <future>
#include <mutex>
#include <iomanip>
#include <nlohmann/json.hpp>

#include <algorithm>

namespace fs = std::filesystem;

struct benchmark_prototype {
	std::string name;
	std::string rtl_path;
	std::string top_module;
};

std::vector<benchmark_prototype> benchmark_contener;
std::string path;
std::string template_script_file;
int NUM_THREADS = 0;

void read_json_file_v2(const std::string& file_path){
	using json = nlohmann::json;
	using namespace std;
	ifstream file;
	file.open(file_path);
	if (!file.is_open()) return;
	json j = json::parse(file);

	std::string f_yosys_template_script = j.value("yosys_template_script", "oops");
	int f_num_process = j.value("num_process", 0);
	//std::string f_benchmarks = j.value("benchmarks", "oops");

	template_script_file = "/" + f_yosys_template_script;
	NUM_THREADS = f_num_process;

	benchmark_prototype benchmark_tmp_obj;

	for (auto& elem : j["benchmarks"]){
   		std::string f_name = elem.value("name", "oops");
   		std::string f_rtl_path = elem.value("rtl_path", "oops");
   		std::string f_top_module = elem.value("top_module", "oops");
		benchmark_tmp_obj.name = f_name;
		benchmark_tmp_obj.rtl_path = f_rtl_path;
		benchmark_tmp_obj.top_module = f_top_module;
		benchmark_contener.push_back(benchmark_tmp_obj);
	}

}

int cut_lut_numbers_from_text(std::string text)
{
	std::istringstream iss{text};
    std::string word;
    int x;
    while(iss >> word)
    	{
            try {x = std::stoi(std::regex_replace(word, std::regex{ R"([^\d])" }, ""));}
            catch(std::invalid_argument& e){continue;}
        }
	return x;
}

int get_lut_count_from_log_file(benchmark_prototype& benchmark, std::string log_type)
{
	using namespace std;
	int x;
	std::string file_path = benchmark.rtl_path + "/" + benchmark.name + "_" + log_type + ".txt";
	string line;
  	ifstream myfile (file_path.c_str());
	if (myfile.is_open())
	{
		while (getline(myfile, line))
		{
			if (line.find("$lut") != std::string::npos){
				x = cut_lut_numbers_from_text(line);
			}			
		}

	}	
	else cout << "Unable to open file";
	return x;
}

void Inference_function(std::vector<float> & numbers){
	float procent_count = 0;
	float procent_sum = 0;
	for (float i : numbers){
		procent_count++;
		procent_sum += i;
	}

	std::cout << "\nТotal number of results received = " << procent_count << std::endl;
	float tmp = procent_sum / procent_count;
	printf ("The efficiency of the script is = %f\n", tmp);
}

bool chack_file_exist(const char* file){
	using namespace std;
	ifstream myfile;
	myfile.open(file);
	if(myfile) {
		return true;
   	} else {
		return false;
   	}
}

int check_json_file(int argc, char* argv[]){
	switch (argc)
	{	
	case 1:
		if (chack_file_exist("/home/utilities/example.json")){
			path = "/home/utilities/example.json";
		}else{
			std::cout << "Cant find json file" << std::endl;
			return -1;
		}
		break;
	case 2:
			std::cout << "Invalid command" << std::endl;
			return -1;
		break;
	case 3:
		if (std::string(argv[1]) == "-json"){
			if (chack_file_exist(argv[2])){
				path = argv[2];
			}else{
				std::cout << "Json file path is invalid" << std::endl;
				return -1;
			}
		} else{
			std::cout << "Invalid command" << std::endl;
			return -1;
		}
		break;
	default:
		break;
	}
	
	return 1;
}

std::string find_verilog_extension_files_in_folder(const char* dir){
	std::string path(dir);
	std::string ext[4] = { ".v", ".vh", ".verilog", ".vlg"};
	std::string all_files;
	for (auto& p : fs::recursive_directory_iterator(path))
	{
		if (p.path().extension() == ext[0] || p.path().extension() == ext[1] || p.path().extension() == ext[2] || p.path().extension() == ext[3])
		{
			if (all_files.empty()){
				all_files = p.path().string();
			}else{
				all_files = all_files + " " + p.path().string();
			}
		}
	}
	return all_files;
}

void make_own_yosys_synth_file(benchmark_prototype& benchmark) 
{
	using namespace std;

	std::string new_file_full_name = benchmark.rtl_path + "/" + benchmark.name + "_own.ys";
	ofstream myfile(new_file_full_name);

	if (myfile.is_open())
	{
		myfile << "read_verilog " << find_verilog_extension_files_in_folder(benchmark.rtl_path.c_str()) << "\n";
		myfile << "hierarchy -check -top " << benchmark.top_module << "\n";
		myfile << "proc\n";
		myfile << "fsm\n";
		myfile << "opt\n";
		myfile << "memory\n";
		myfile << "wreduce\n";
		myfile << "peepopt\n";
		myfile << "submod\n";
		myfile << "scc -all_cell_types\n";
		myfile << "techmap -map +/cmp2lut.v -map +/cmp2lcu.v -D LUT_WIDTH=6\n";
		myfile << "dfflibmap -liberty /home/utilities/libs/mycells.lib\n";
		myfile << "sim -sim-gold -sim-cmp\n";
		myfile << "qwp -alpha\n";
		myfile << "alumacc\n";
		myfile << "memory\n";
		myfile << "qwp -ltr\n";
		myfile << "aigmap\n";
		myfile << "opt_clean\n";
		myfile << "abc -D 1000\n";
		myfile << "opt -fast -full\n";
		myfile << "abc -fast -lut 6\n";
		myfile << "techmap\n";
		myfile << "splitnets -ports\n";
		myfile << "anlogic_fixcarry\n";
		myfile << "opt -full\n";
		myfile << "greenpak4_dffinv\n";
		myfile << "memory_memx\n";
		myfile << "clean -purge\n";
		myfile << "chformal -delay 1000\n";
		myfile << "share -aggressive\n";
		myfile << "techmap -map +/cmp2lut.v -map +/cmp2lcu.v -D LUT_WIDTH=6\n";
		myfile << "nlutmap -luts 6\n";
		myfile << "scatter\n";
		myfile << "opt -fast\n";
		myfile << "simplemap\n";
		myfile << "flatten\n";
		myfile << "bmuxmap\n";
		myfile << "attrmap -tocase keep -imap keep=\"true\" keep=1\n";
		myfile << "abc -fast -lut 6 -dress\n";
		myfile << "clean\n";
		myfile << "opt\n";
		myfile << "stat\n";
		myfile << "check\n";
		myfile << "tee -o " << benchmark.rtl_path << "/" << benchmark.name << "_own.txt stat";

		myfile.close();

		std::cout << " - Maked yosys OWN script file" << std::endl;
	}

	else cout << "Unable to open file" << endl;
}

void copy_standart_yosys_script(std::vector<std::string> &temp, benchmark_prototype& benchmark)
{	

	using namespace std;
		
	fstream myFile;
	const char * c = template_script_file.c_str();
	myFile.open(c, ios::in);
	if (myFile.is_open()){
		string line;
		while (getline(myFile, line))
		{			
			if (line.find("$verilog_file_name") != std::string::npos){
				line.replace(line.find("$verilog_file_name"), sizeof("$verilog_file_name") - 1, find_verilog_extension_files_in_folder(benchmark.rtl_path.c_str()));
			}
			if (line.find("$top_module_name") != std::string::npos){
				line.replace(line.find("$top_module_name"), sizeof("$top_module_name") - 1, benchmark.top_module);
			}
			temp.push_back(line);
		}
		myFile.close();
	}

}

void make_new_synth_file_with_temp(std::vector<std::string> &temp, benchmark_prototype& benchmark){
	using namespace std;
	
	string new_file_name = benchmark.rtl_path + "/" + benchmark.name + "_standart.ys";
	
	ofstream myfile (new_file_name.c_str());
	if (myfile.is_open())
	{
		for (int i = 0; i < temp.size(); i++){
			myfile << temp[i];
		}
		myfile << "\ntee -o " << benchmark.rtl_path << "/" << benchmark.name << "_standart.txt stat";
		myfile.close();
		std::cout << " - Maked yosys STANDART script file" << std::endl;
	}
	else cout << "Unable to open file";
}

void make_yosys_script_file (benchmark_prototype& benchmark){
	std::vector<std::string> temp;
	copy_standart_yosys_script(temp, benchmark);
	make_new_synth_file_with_temp(temp, benchmark);
}

/*
void call_synthensys_command(int &id)
{
	{
		std::lock_guard<std::mutex> guard(thr_mutex);
		if (id == 100){
			return;
		}
		id++;
	}	
	std::cout << id << std::endl;
	call_synthensys_command(id);
}
*/

int main(int argc, char* argv[])
{
	if (check_json_file(argc, argv) == -1){return -1;}
	//read_json_file(path);
	read_json_file_v2(path);
	if (!chack_file_exist(template_script_file.c_str())){std::cout << "Worning !!! : Yosys template script file directory is invalid" << std::endl; return -1;}
	std::vector<std::string> commands;

	for (int i = 0; i < benchmark_contener.size(); i++) //
	{
		//std::cout << std::endl << "File name: \t" << benchmark_contener[i].name << std::endl;
		//std::cout << "File path: \t" << benchmark_contener[i].rtl_path << std::endl;
		//std::cout << "Top module: \t" << benchmark_contener[i].top_module << std::endl;
		make_yosys_script_file(benchmark_contener[i]);
		make_own_yosys_synth_file(benchmark_contener[i]);

		std::string tmp_command;
		tmp_command = "yosys " + benchmark_contener[i].rtl_path + "/" + benchmark_contener[i].name + "_standart.ys";
		commands.push_back(tmp_command);
		tmp_command.clear();
		tmp_command = "yosys " + benchmark_contener[i].rtl_path + "/" + benchmark_contener[i].name + "_own.ys";
		commands.push_back(tmp_command);
		tmp_command.clear();
	}
	
	std::mutex thr_mutex;
	int done_work_count = 0;
	std::vector<std::thread> Threads;
	std::chrono::duration<double> threads_start_time [NUM_THREADS];
	std::chrono::duration<double> threads_end_time [NUM_THREADS];
	for (int i = 0; i < NUM_THREADS; i++) {
		Threads.push_back(std::thread([&thr_mutex, &done_work_count, &commands, &threads_start_time, &threads_end_time]() {
			while (true) {
				{					
					std::lock_guard<std::mutex>  lg(thr_mutex);
					if (done_work_count >= benchmark_contener.size()*2) {
						break;
					}
					else {
						done_work_count++;
						//threads_start_time[done_work_count-1] = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::);
					}					
				}
				system(commands[done_work_count-1].c_str());
				auto end_time = std::chrono::system_clock::now();
			}
		}));
	}
	for (std::thread &it : Threads) {
		it.join();
	}
	
	std::vector<float> numbers;
	for(auto it : benchmark_contener){
		float own = get_lut_count_from_log_file(it, "own");
		float standart = get_lut_count_from_log_file(it, "standart");	
		float tmp = ((standart - own) / standart) * 100;
		numbers.push_back(tmp);
	}

	Inference_function(numbers);

	return 0;
}