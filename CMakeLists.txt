cmake_minimum_required(VERSION 3.18)
project(BeatBox VERSION 1.0 DESCRIPTION "BeatBox" LANGUAGES C)

# Compiler Options
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -Wpedantic -Wextra -pg -lasound)
add_compile_options(-g -Wall -Werror -Wpedantic -Wextra)
#add_compile_options(-fdiagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

add_subdirectory(hal)
add_subdirectory(app)

# Comment out Lines 8, 13, and 14
# Uncomment Line 7