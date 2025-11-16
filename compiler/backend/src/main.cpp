#include <fstream>
#include <iostream>

int main() {
	std::string output_path = "/app/out/test.txt";
	
	std::ofstream out_file(output_path);
	
	if (!out_file) {
		std::cerr << "error" << std::endl;
		return 1;
	}

	out_file << "Hello, world!" << std::endl;
	
	out_file.close();

	std::cout << "done" << std::endl;

	return 0;
}
