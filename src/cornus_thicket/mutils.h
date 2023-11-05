#ifndef cornus_thicket_mutils_hpp
#define cornus_thicket_mutils_hpp


#define CORNUS_THICKET_CONCAT(a,b) CORNUS_THICKET_CONCAT_(a,b)
#define CORNUS_THICKET_CONCAT_(a,b) a##b

#define CORNUS_THICKET_STRINGIZE(x) CORNUS_THICKET_STRINGIZE_(x, )
#define CORNUS_THICKET_STRINGIZE_(x, _) _#x

#endif
