//
// Created by selman.ozleyen2017 on 8.07.2020.
//

#include <cstdlib>
#include <iostream>
#include <fstream>

using namespace std;

int main(){
    srand(1000);
    int actual;
    int temp = 0,temp2;
    fstream f("mem.dat", ios::binary| ios::in | ios::out);
    fstream f2("list.txt", ios::out);
    fstream f3("list2.txt", ios::out);
    for (int i = 0; i < (1 << 11); ++i) {
        f3 << rand() << endl;
    }
    for (int i = 0; i < (1 << 11); ++i) {
        temp2 = temp;
        f.read((char *)&temp,sizeof(int));
        f2 << temp << endl;

        //if(temp )
    }
    f2.close();
}
