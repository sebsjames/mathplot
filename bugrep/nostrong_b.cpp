module;
#include <iostream>
export module nostrong.b;

import nostrong.a;

export struct B : public A
{
    void me() { std::cout << "B\n"; }
};
