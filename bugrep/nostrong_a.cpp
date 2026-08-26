// Attempt to reduce code down to reveal bug seen when compiling sm::mat<>::str_arr() in VisualOwnable
module;

#include <iostream>

export module nostrong.a;

import sm.mat;

export struct A
{
    sm::mat<float, 4> m;

    void f()
    {
        std::cout << this->m.str_arr() << std::endl;
    }
};
