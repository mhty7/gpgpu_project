#include <unistd.h>
#include <iostream>
#include <string>



class ResourceManager {
public:
    
    std::string GetResourcePath() const{
        char temp[PATH_MAX];
        if(getcwd(temp, PATH_MAX)){
            return std::string(temp);
        }
        return nullptr;
    }
    
};



