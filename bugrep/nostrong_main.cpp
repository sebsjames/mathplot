// Attempt to reduce code down to reveal bug seen when compiling sm::mat<>::str_arr() in VisualOwnable
// error message:
// /opt/gcc16/include/c++/16.2.1/format:5016:32: error: 'strong_ordering' is not a member of 'std'

import nostrong.b;

int main()
{
    B b;
    b.f();
    b.me();
}
