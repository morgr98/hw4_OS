
#include <iostream>
#include "malloc_3.cpp"
#include <iostream>
using namespace std;
/*
 * main2.cpp is used to test your malloc_2.cpp file.
 * testing is done by redirecting output (in testing scripting) to the output/expected folder,
 * and comparing files (in bash).
 */

#define MD_SIZE _size_meta_data()
#define NUM_TESTS 4
#define MAX_SIZE 100000000
#define ASSERT_EQUAL(a,b,line)  \
if(a != b){  \
    cout << "FAILED:" << endl << "\t\t" << #a << " is not equal to " << #b  << ", file: " << __FILE__ << ", line: " << line << endl;    \
    cout << "\t\t" << "yours: " << a << ", expected: " << b << endl << endl;  \
    exit(0); \
}
#define RUN_TEST(indx) 		\
	do {			 \
		tests[indx]();	  \
	} while(0)


class Stats
{
public:

    size_t num_free_blocks;
    size_t num_free_bytes;
    size_t num_allocated_blocks;
    size_t num_allocated_bytes;
    size_t num_meta_bytes;
    size_t size_meta_data;

    Stats();
    Stats(size_t a, size_t b, size_t c, size_t d, size_t e, size_t f);
    void set();
    void print();
    void setAndPrint();
    // bool compare(size_t a, size_t b, size_t c, size_t d, size_t e, size_t f);
    bool compare(size_t expected_num_free_blocks, size_t expected_num_free_bytes, size_t expected_num_allocated_blocks,
                 size_t expected_num_allocated_bytes, size_t expected_num_meta_bytes, size_t expected_size_meta_data, int line);
};
Stats::Stats()
{
    num_free_blocks = 0;
    num_free_bytes = 0;
    num_allocated_blocks = 0;
    num_allocated_bytes = 0;
    num_meta_bytes = 0;
    size_meta_data = 0;
}
Stats::Stats(size_t a, size_t b, size_t c, size_t d, size_t e, size_t f)
{
    num_free_blocks = a;
    num_free_bytes = b;
    num_allocated_blocks = c;
    num_allocated_bytes = d;
    num_meta_bytes = e;
    size_meta_data = f;
};

void Stats::set()
{
    num_free_blocks = _num_free_blocks();
    num_free_bytes = _num_free_bytes();
    num_allocated_blocks =_num_allocated_blocks();
    num_allocated_bytes =_num_allocated_bytes();
    num_meta_bytes =_num_meta_data_bytes();
    size_meta_data =_size_meta_data();
};

void Stats::print()
{
    cout << "num_free_blocks = " << num_free_blocks << endl;
    cout << "num_free_bytes = " <<  num_free_bytes << endl;
    cout << "num_allocated_blocks = " << num_allocated_blocks  << endl;
    cout << "num_allocated_bytes = " <<  num_allocated_bytes << endl;
    cout << "num_meta_bytes = " <<  num_meta_bytes << endl;
    cout << "size_meta_data = " <<  size_meta_data << endl;
    cout << std::endl;
}

void Stats::setAndPrint()
{
    set();
    print();
}

bool Stats::compare(size_t expected_num_free_blocks, size_t expected_num_free_bytes, size_t expected_num_allocated_blocks,
                    size_t expected_num_allocated_bytes, size_t expected_num_meta_bytes, size_t expected_size_meta_data, int line)
{
    ASSERT_EQUAL(num_free_blocks, expected_num_free_blocks, line);
    ASSERT_EQUAL(num_free_bytes, expected_num_free_bytes, line);
    ASSERT_EQUAL(num_allocated_blocks, expected_num_allocated_blocks, line);
    ASSERT_EQUAL(num_allocated_bytes, expected_num_allocated_bytes, line);
    ASSERT_EQUAL(num_meta_bytes, expected_num_meta_bytes, line);
    ASSERT_EQUAL(size_meta_data, expected_size_meta_data, line);
}
// bool Stats::compare(size_t expected_num_free_blocks, size_t expected_num_free_bytes, size_t expected_num_allocated_blocks,
//                     size_t expected_num_allocated_bytes, size_t expected_num_meta_bytes, size_t expected_size_meta_data)
// {
//     ASSERT_EQUAL(num_free_blocks, expected_num_free_blocks);
//     ASSERT_EQUAL(num_free_bytes, expected_num_free_bytes);
//     ASSERT_EQUAL(num_allocated_blocks, expected_num_allocated_blocks);
//     ASSERT_EQUAL(num_allocated_bytes, expected_num_allocated_bytes);
//     ASSERT_EQUAL(num_meta_bytes, expected_num_meta_bytes);
//     ASSERT_EQUAL(size_meta_data, expected_size_meta_data);
// }

size_t align(size_t a)
{
    a=((a+7)/8)*8;
    return a;
}



void print(void) {
    unsigned num_free_blocks = _num_free_blocks();
    unsigned num_free_bytes = _num_free_bytes();
    unsigned num_allocated_blocks =_num_allocated_blocks();
    unsigned num_allocated_bytes =_num_allocated_bytes();
    unsigned num_beta_bytes =_num_meta_data_bytes();
    unsigned size_meta_data =_size_meta_data();
    std::cout << "num_free_blocks = " << num_free_blocks << std::endl;
    std::cout << "num_free_bytes = " <<  num_free_bytes << std::endl;
    std::cout << "num_allocated_blocks = " << num_allocated_blocks  << std::endl;
    std::cout << "num_allocated_bytes = " <<  num_allocated_bytes << std::endl;
    std::cout << "num_beta_bytes = " <<  num_beta_bytes << std::endl;
    std::cout << "size_meta_data = " <<  size_meta_data << std::endl;
    std::cout << std::endl;
}


void test1() {
    Stats s;

    void* p9 = smalloc(5000);
    void* p10 = smalloc(5000);

    sfree(p9);
    void* p11 = srealloc(p10, 20000); // b, p9 is wilderness - merge with lower address, and enlarge
    s.set();
    s.compare(0,0,1,20000,1*MD_SIZE,MD_SIZE,__LINE__);
}



void test2() {
    void* p1 = smalloc(10);
    print();
    sfree(p1);
    print();
    p1 = smalloc(5);
    print();
    sfree(p1);
    print();
    p1 = smalloc(10);
    sfree(p1);
    print();
    p1 = smalloc(11);
    print();
    sfree(p1);
    print();
}


void test3() {
    void* p1 = smalloc(100);
    void* p2 = p1;
    p1 = smalloc(MAX_SIZE+1);
    if(p1 == nullptr) {
        std::cout << "p1 is nullptr" << std::endl;
    } else {
        std::cout << "p1 is not nullptr" << std::endl;
    }
    p1 = p2;
    print();
    p2 = smalloc(50);
    p1 = srealloc(p1,300);
    void* p3 = smalloc(25);
    print();
    sfree(p3);
    void* p4 = scalloc(10,4);
    for(unsigned i = 0; i < 40; i++) {
        char* tmp = (char*)(p4)+i;
        if(*tmp != 0) {
            std::cerr << "should be zero" << std::endl;
        }
    }
    sfree(p1);
    sfree(p2);
    sfree(p4);
    print();
}


void test4() {
    void* p1 = srealloc(nullptr,10);
    print();
    p1 = srealloc(nullptr,20);
    print();
    p1 = srealloc(nullptr,30);
    print();
    p1 = srealloc(nullptr,40);
    print();
    sfree(p1);
    print();
    p1 = scalloc(5,2);
    sfree(p1);
    print();
    p1 = scalloc(5,8);
    sfree(p1);
    print();
    p1 = scalloc(5,12);
    print();
    sfree(p1);
}


void (*tests[NUM_TESTS])(void) = {
        test1,
        test2,
        test3,
        test4
};


int main(int argc,char* argv[]) {
    if(argc != 2) {
        std::cerr << "Invalid Usage - Use: <prog_name> <index>" << std::endl;
        return 1;
    }
    unsigned indx = strtol(argv[1],nullptr,10);
    if(indx >= NUM_TESTS) {
        std::cerr << "Invalid Test Index - Index: {0.." << NUM_TESTS - 1 << "}" << std::endl;
        return 1;
    }
    RUN_TEST(indx);
    return 0;
}