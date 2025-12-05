#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "APP3: Goes into cycle" << std::endl; 
    while (true)
    {
        sleep(1);
    }


    std::cout << "APP3: Done!" << std::endl; 


}

