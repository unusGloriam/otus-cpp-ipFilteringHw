#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "lib.h"

#define UNUSED(expr) (void)expr

// ("",  '.') -> [""]
// ("11", '.') -> ["11"]
// ("..", '.') -> ["", "", ""]
// ("11.", '.') -> ["11", ""]
// (".11", '.') -> ["", "11"]
// ("11.22", '.') -> ["11", "22"]
std::vector<std::string> split(const std::string &str, char d)
{
    std::vector<std::string> r;

    std::string::size_type start = 0;
    std::string::size_type stop = str.find_first_of(d);
    while(stop != std::string::npos)
    {
        r.push_back(str.substr(start, stop - start));

        start = stop + 1;
        stop = str.find_first_of(d, start);
    }

    r.push_back(str.substr(start));

    return r;
}

int main(int argc, char const *argv[])
{
    UNUSED(argc);
    UNUSED(argv);
    try
    {
        std::vector<std::vector<std::string> > ipPool;

        for(std::string line; std::getline(std::cin, line);)
        {
            std::vector<std::string> v = split(line, '\t');
            ipPool.push_back(split(v.at(0), '.'));
        }
        //std::cerr << "Done filling ips\n";

		auto isLesser = [](const std::string& numAStr, const std::string& numBStr){ return std::stoi(numAStr) < std::stoi(numBStr); };	//Лямбда сравнения двух чисел строкового формата
		
		auto doSortingIteration = [swapRequired = isLesser](decltype(ipPool.begin()) firstComparant, decltype(ipPool.begin()) secondComparant){
			//Лямбда сравнения пары адресов в векторе
			//firstComparant в векторе должен идти раньше, чем secondComparant
			//Выполняется обратная сортировка -> "больший" адрес оставляем слева
			for (uint8_t octetIdx = 0; octetIdx < 4; ++octetIdx){
                if (firstComparant[0][octetIdx] == secondComparant[0][octetIdx]) continue;  //Обработка случаев, когда первые октеты адресов одинаковые
				if (swapRequired(firstComparant[0][octetIdx], secondComparant[0][octetIdx])){
                    //std::cerr << "Swapped [" << firstComparant[0][octetIdx] << "] and [" << secondComparant[0][octetIdx] << "]\n";
					std::iter_swap(firstComparant, secondComparant);                   
					return true;
				}
                return false;
			}				
			return false;
		};

		//Обратная лексикографическая сортировка
		bool didSortingOnIteration = true;	
		while (didSortingOnIteration){		//Пока сортируется - сортируем
			didSortingOnIteration = false;
			for (auto ip = ipPool.begin(); ip + 1 != ipPool.end(); ++ip){
				if (doSortingIteration(ip, ip + 1)) didSortingOnIteration = true;
			}						
		}
        //std::cerr << "Done lexicographically sorting ips\n";

		//Вывод результата обратной лексикографической сортировки
        for(/*std::vector<std::vector<std::string> >::const_iterator*/decltype(ipPool.cbegin()) ip = ipPool.cbegin(); ip != ipPool.cend(); ++ip)
        {
            for(/*std::vector<std::string>::const_iterator*/decltype(ip->cbegin()) ip_part = ip->cbegin(); ip_part != ip->cend(); ++ip_part)
            {
                if (ip_part != ip->cbegin())
                {
                    std::cout << ".";
                }
                std::cout << *ip_part;
            }
            std::cout << std::endl;
        }

        // 222.173.235.246
        // 222.130.177.64
        // 222.82.198.61
        // ...
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        // TODO filter by first byte and output
        // ip = filter(1)		
		//Фильтр адресов, первый байт должен быть равен 1
		auto filter = [ &ipPool ](int firstOctet = 256, int secondOctet = 256) mutable {
            auto srcVector = ipPool;
            if (firstOctet == 256 && secondOctet == 256) return srcVector;      //Не фильтруем, так не фильтруем
			for (auto ipVectorItr = srcVector.begin(); ipVectorItr != srcVector.end(); ){
                if ((firstOctet != 256 && std::stoi(ipVectorItr[0][0]) != firstOctet)
                    || (secondOctet != 256 && std::stoi(ipVectorItr[0][1]) != secondOctet)){  //Фильтруем, так фильтруем
                        ipVectorItr = srcVector.erase(ipVectorItr); 
                    } else ++ipVectorItr;                   
            }
            return srcVector;
		};
        auto firstByteFilteredIPs = filter(1);

        //Вывод результата сортировки по первому байту
        for(/*std::vector<std::vector<std::string> >::const_iterator*/decltype(firstByteFilteredIPs.cbegin()) ip = firstByteFilteredIPs.cbegin(); ip != firstByteFilteredIPs.cend(); ++ip)
        {
            for(/*std::vector<std::string>::const_iterator*/decltype(ip->cbegin()) ip_part = ip->cbegin(); ip_part != ip->cend(); ++ip_part)
            {
                if (ip_part != ip->cbegin())
                {
                    std::cout << ".";
                }
                std::cout << *ip_part;
            }
            std::cout << std::endl;
        }

        // 1.231.69.33
        // 1.87.203.225
        // 1.70.44.170
        // 1.29.168.152
        // 1.1.234.8

        // TODO filter by first and second bytes and output
        // ip = filter(46, 70)
        //Фильтр адресов, первый байт должен быть равен 46, второй - 70
        auto secondByteFilteredIPs = filter(46, 70);

        //Вывод результата сортировки по первому и второму байтам
        for(/*std::vector<std::vector<std::string> >::const_iterator*/decltype(secondByteFilteredIPs.cbegin()) ip = secondByteFilteredIPs.cbegin(); ip != secondByteFilteredIPs.cend(); ++ip)
        {
            for(/*std::vector<std::string>::const_iterator*/decltype(ip->cbegin()) ip_part = ip->cbegin(); ip_part != ip->cend(); ++ip_part)
            {
                if (ip_part != ip->cbegin())
                {
                    std::cout << ".";
                }
                std::cout << *ip_part;
            }
            std::cout << std::endl;
        }

        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76

        // TODO filter by any byte and output
        // ip = filter_any(46)
        //Фильтр адресов, любой байт должен быть равен 46
        auto filter_any = [ &ipPool ](int refOctet = 256) mutable {
            auto srcVector = ipPool;
            if (refOctet == 256) return srcVector;      //Не фильтруем, так не фильтруем
            for (auto ipVectorItr = srcVector.begin(); ipVectorItr != srcVector.end(); ){
                if (std::stoi(ipVectorItr[0][0]) != refOctet
                    && std::stoi(ipVectorItr[0][1]) != refOctet
                    && std::stoi(ipVectorItr[0][2]) != refOctet
                    && std::stoi(ipVectorItr[0][3]) != refOctet){  //Фильтруем, так фильтруем
                    ipVectorItr = srcVector.erase(ipVectorItr); 
                } else ++ipVectorItr;                   
            }
            return srcVector;
        };

        auto anyBytesFilteredIPs = filter_any(46);

        //Вывод результата сортировки по любому из байтов
        for(/*std::vector<std::vector<std::string> >::const_iterator*/decltype(anyBytesFilteredIPs.cbegin()) ip = anyBytesFilteredIPs.cbegin(); ip != anyBytesFilteredIPs.cend(); ++ip)
        {
            for(/*std::vector<std::string>::const_iterator*/decltype(ip->cbegin()) ip_part = ip->cbegin(); ip_part != ip->cend(); ++ip_part)
            {
                if (ip_part != ip->cbegin())
                {
                    std::cout << ".";
                }
                std::cout << *ip_part;
            }
            std::cout << std::endl;
        }

        // 186.204.34.46
        // 186.46.222.194
        // 185.46.87.231
        // 185.46.86.132
        // 185.46.86.131
        // 185.46.86.131
        // 185.46.86.22
        // 185.46.85.204
        // 185.46.85.78
        // 68.46.218.208
        // 46.251.197.23
        // 46.223.254.56
        // 46.223.254.56
        // 46.182.19.219
        // 46.161.63.66
        // 46.161.61.51
        // 46.161.60.92
        // 46.161.60.35
        // 46.161.58.202
        // 46.161.56.241
        // 46.161.56.203
        // 46.161.56.174
        // 46.161.56.106
        // 46.161.56.106
        // 46.101.163.119
        // 46.101.127.145
        // 46.70.225.39
        // 46.70.147.26
        // 46.70.113.73
        // 46.70.29.76
        // 46.55.46.98
        // 46.49.43.85
        // 39.46.86.85
        // 5.189.203.46
    }
    catch(const std::exception &e)
    {
        std::cerr << e.what() << std::endl ;
    }

    return 0;
}
