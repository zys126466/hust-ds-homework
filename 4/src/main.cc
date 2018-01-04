#include <general_ui.hpp>
#include <parser.hpp>
#include <gc.h>

reflected_impl impl;
GCThread gc;

int main() {
    fake_terminal::go(parser::parse);
}