#include <iostream>
#include <unistd.h>

int main()
{
    std::cout << "Goes into cycle" << std::endl; 
    while (true)
    {
        sleep(1);
    }


    std::cout << "Done!" << std::endl; 


}

