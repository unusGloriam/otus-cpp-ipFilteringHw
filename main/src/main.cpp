#include "version_api/version_api.h"
#include <iostream>

int main() {
	
    std::cout << "Version: " << VersionAPI::getVersion() << std::endl;
    std::cout << "Hello, world!" << std::endl;
	
	return 0;
}
